//
// Copyright 2020 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "materialWriter.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(
    lambert,
    PxrUsdTranslators_MaterialWriter);

PXRUSDMAYA_REGISTER_WRITER(
    blinn,
    PxrUsdTranslators_MaterialWriter);

PXRUSDMAYA_REGISTER_WRITER(
    phong,
    PxrUsdTranslators_MaterialWriter);

PXRUSDMAYA_REGISTER_WRITER(
    phongE,
    PxrUsdTranslators_MaterialWriter);

PXRUSDMAYA_REGISTER_WRITER(
    standardSurface,
    PxrUsdTranslators_MaterialWriter);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (color)
    (diffuse)
    (incandescence)
    (eccentricity)
    (cosinePower)
    (outColor)
    (specularRollOff)
    (normalCamera)

    // Sub-selection of standard surface attributes:
    (base)
    (baseColor)
    (emission)
    (emissionColor)
    (specular)
    (metalness)
    (specularRoughness)
    (coat)
    (coatRoughness)
    (specularIOR)
    (transmission)

    // XXX: We duplicate these tokens here rather than create a dependency on
    // usdImaging in case the plugin is being built with imaging disabled.
    // If/when they move out of usdImaging to a place that is always available,
    // they should be pulled from there instead.
    (UsdPreviewSurface)

    // UsdPreviewSurface:
    (inputs)
    (diffuseColor)
    (emissiveColor)
    (useSpecularWorkflow)
    (specularColor)
    (metallic)
    (roughness)
    (clearcoat)
    (clearcoatRoughness)
    (opacity)
    (ior)
    (normal)
    (displacement)

    // Roundtrip memory storage:
    (Maya)
    (nodeName)
);

PxrUsdTranslators_MaterialWriter::PxrUsdTranslators_MaterialWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
    UsdShadeShader shaderSchema =
        UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    UsdAttribute idAttr = 
        shaderSchema.CreateIdAttr(VtValue(_tokens->UsdPreviewSurface));

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }

    // Surface Output
    shaderSchema.CreateOutput(
        UsdShadeTokens->surface,
        SdfValueTypeNames->Token);

    // Displacement Output
    shaderSchema.CreateOutput(
        UsdShadeTokens->displacement,
        SdfValueTypeNames->Token);
}

static
bool
_AuthorShaderInputFromShadingNodeAttr(
        const MFnDependencyNode& depNodeFn,
        const TfToken& shadingNodeAttrName,
        UsdShadeShader& shaderSchema,
        const TfToken& shaderInputName,
        const SdfValueTypeName& shaderInputTypeName,
        const UsdTimeCode usdTime,
        const TfToken& scalingAttrName = TfToken())
{
    MStatus status;

    MPlug shadingNodePlug =
        depNodeFn.findPlug(
            depNodeFn.attribute(shadingNodeAttrName.GetText()),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return false;
    }

    const bool isDestination = shadingNodePlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Are color values are all linear on the shader?
    // Do we need to re-linearize them?
    VtValue value =
        UsdMayaWriteUtil::GetVtValue(
            shadingNodePlug,
            shaderInputTypeName,
            /* linearizeColors = */ false);

    if (value.IsEmpty()) {
        return false;
    }

    UsdShadeInput shaderInput =
        shaderSchema.CreateInput(shaderInputName, shaderInputTypeName);

    // For attributes that are the destination of a connection, we create
    // the input on the shader but we do *not* author a value for it. We
    // expect its actual value to come from the source of its connection.
    // We'll leave it to the shading export to handle creating
    // the connections in USD.
    if (!isDestination) {
        if (scalingAttrName != TfToken() && value.IsHolding<GfVec3f>()) {
            float colorScale = 1.0f;

            MPlug scalingPlug =
                depNodeFn.findPlug(
                    depNodeFn.attribute(scalingAttrName.GetText()),
                    /* wantNetworkedPlug = */ true,
                    &status);
            if (status == MS::kSuccess) {
                VtValue vtScale =
                    UsdMayaWriteUtil::GetVtValue(
                        scalingPlug,
                        SdfValueTypeNames->Float);

                if (vtScale.IsHolding<float>()) {
                    colorScale = vtScale.UncheckedGet<float>();
                }
            }

            value = value.UncheckedGet<GfVec3f>() * colorScale;
        }

        shaderInput.Set(value, usdTime);
    }

    return true;
}

/* virtual */
void
PxrUsdTranslators_MaterialWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    if (GetMayaObject().hasFn(MFn::Type::kBlinn)) {
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->eccentricity,
            shaderSchema,
            _tokens->roughness,
            SdfValueTypeNames->Float,
            usdTime);
    }

    if (GetMayaObject().hasFn(MFn::Type::kPhong)) {
        MPlug cosinePowerPlug =
            depNodeFn.findPlug(
                depNodeFn.attribute(_tokens->cosinePower.GetText()),
                /* wantNetworkedPlug = */ true,
                &status);
        if (status == MS::kSuccess) {
            VtValue cosinePower =
                UsdMayaWriteUtil::GetVtValue(
                    cosinePowerPlug,
                    SdfValueTypeNames->Float,
                    /* linearizeColors = */ false);

            // In the maya UI, cosinePower goes from 2.0 to 100.0
            // this does not map directly to specular roughness
            float roughnessFloat =
                std::sqrt(1.0f / (0.454f * cosinePower.Get<float>() + 3.357f));

            shaderSchema.CreateInput(
                _tokens->roughness,
                SdfValueTypeNames->Float).Set(roughnessFloat, usdTime);
        }
    }

    if (GetMayaObject().hasFn(MFn::Type::kPhongExplorer)) {
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->roughness,
            shaderSchema,
            _tokens->roughness,
            SdfValueTypeNames->Float,
            usdTime);
    }

    if (GetMayaObject().hasFn(MFn::Type::kReflect)) {
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->specularColor,
            shaderSchema,
            _tokens->specularColor,
            SdfValueTypeNames->Color3f,
            usdTime,
            GetMayaObject().hasFn(MFn::Type::kBlinn) ? _tokens->specularRollOff
                                                     : TfToken());

        shaderSchema.CreateInput(
            _tokens->useSpecularWorkflow,
            SdfValueTypeNames->Int).Set(1, usdTime);
    } else {
        shaderSchema.CreateInput(
            _tokens->roughness,
            SdfValueTypeNames->Float).Set(1.0f, usdTime);

        shaderSchema.CreateInput(
            _tokens->useSpecularWorkflow,
            SdfValueTypeNames->Int).Set(0, usdTime);
    }

    if (GetMayaObject().hasFn(MFn::Type::kLambert)) {
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->color,
            shaderSchema,
            _tokens->diffuseColor,
            SdfValueTypeNames->Color3f,
            usdTime,
            _tokens->diffuse);

        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->incandescence,
            shaderSchema,
            _tokens->emissiveColor,
            SdfValueTypeNames->Color3f,
            usdTime);

        // Exported, but unsupported in hdStorm.
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->normalCamera,
            shaderSchema,
            _tokens->normal,
            SdfValueTypeNames->Normal3f,
            usdTime);
    }

#if MAYA_API_VERSION >= 20200000
    if (GetMayaObject().hasFn(MFn::Type::kStandardSurface)) {
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->baseColor,
            shaderSchema,
            _tokens->diffuseColor,
            SdfValueTypeNames->Color3f,
            usdTime,
            _tokens->base);

        // Emission is modulated from emission weight:
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->emissionColor,
            shaderSchema,
            _tokens->emissiveColor,
            SdfValueTypeNames->Color3f,
            usdTime,
            _tokens->emission);

        MPlug metalnessPlug =
            depNodeFn.findPlug(
                depNodeFn.attribute(_tokens->metalness.GetText()),
                /* wantNetworkedPlug = */ true,
                &status);
        if (status == MS::kSuccess && UsdMayaUtil::IsAuthored(metalnessPlug)) {
            _AuthorShaderInputFromShadingNodeAttr(
                depNodeFn,
                _tokens->metalness,
                shaderSchema,
                _tokens->metallic,
                SdfValueTypeNames->Float,
                usdTime);

            // IOR value from Gold USDPreviewSurface preset
            shaderSchema.CreateInput(
                _tokens->ior,
                SdfValueTypeNames->Float).Set(50.0f, usdTime);
        } else {
            shaderSchema.CreateInput(
                _tokens->useSpecularWorkflow,
                SdfValueTypeNames->Int).Set(1, usdTime);

            _AuthorShaderInputFromShadingNodeAttr(
                depNodeFn,
                _tokens->specularColor,
                shaderSchema,
                _tokens->specularColor,
                SdfValueTypeNames->Color3f,
                usdTime,
                _tokens->specular);

            _AuthorShaderInputFromShadingNodeAttr(
                depNodeFn,
                _tokens->specularIOR,
                shaderSchema,
                _tokens->ior,
                SdfValueTypeNames->Float,
                usdTime);
        }

        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->specularRoughness,
            shaderSchema,
            _tokens->roughness,
            SdfValueTypeNames->Float,
            usdTime);

        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->coat,
            shaderSchema,
            _tokens->clearcoat,
            SdfValueTypeNames->Float,
            usdTime);

        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->coatRoughness,
            shaderSchema,
            _tokens->clearcoatRoughness,
            SdfValueTypeNames->Float,
            usdTime);

        MPlug transmissionPlug =
            depNodeFn.findPlug(
                depNodeFn.attribute(_tokens->transmission.GetText()),
                /* wantNetworkedPlug = */ true,
                &status);
        if (status == MS::kSuccess && UsdMayaUtil::IsAuthored(transmissionPlug)) {
            // Need a solution if the transmission is textured, but in the
            // meantime, we go 1 - transmission.
            VtValue value =
                UsdMayaWriteUtil::GetVtValue(
                    transmissionPlug,
                    SdfValueTypeNames->Float,
                    /* linearizeColors = */ false);

            shaderSchema.CreateInput(
                _tokens->opacity,
                SdfValueTypeNames->Float).Set(1.0f - value.Get<float>(),
                                              usdTime);
        }

        // Exported, but unsupported in hdStorm.
        _AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->normalCamera,
            shaderSchema,
            _tokens->normal,
            SdfValueTypeNames->Normal3f,
            usdTime);
    }
#endif
}

/* virtual */
TfToken
PxrUsdTranslators_MaterialWriter::GetShadingAttributeNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    if (!_usdPrim) {
        return TfToken();
    }

    TfToken usdIoDirection;
    TfToken usdPortName;

    if (mayaAttrName == _tokens->outColor) {
        usdIoDirection = UsdShadeTokens->outputs;
        usdPortName = UsdShadeTokens->surface;
    } else {
        usdIoDirection = UsdShadeTokens->inputs;
    }

    if (mayaAttrName == _tokens->color) {
        usdPortName =_tokens->diffuseColor;
    } else if (mayaAttrName == _tokens->specularColor) {
        usdPortName =_tokens->specularColor;
    } else if (mayaAttrName == _tokens->incandescence) {
        usdPortName =_tokens->emissiveColor;
    } else if (mayaAttrName == _tokens->eccentricity) {
        usdPortName =_tokens->roughness;
    } else if (mayaAttrName == _tokens->baseColor) {
        usdPortName =_tokens->diffuseColor;
    } else if (mayaAttrName == _tokens->emissionColor) {
        usdPortName =_tokens->emissiveColor;
    } else if (mayaAttrName == _tokens->metalness) {
        usdPortName =_tokens->metallic;
    } else if (mayaAttrName == _tokens->specularRoughness) {
        usdPortName =_tokens->roughness;
    } else if (mayaAttrName == _tokens->coat) {
        usdPortName =_tokens->clearcoat;
    } else if (mayaAttrName == _tokens->coatRoughness) {
        usdPortName =_tokens->clearcoatRoughness;
    } else if (mayaAttrName == _tokens->opacity) {
        usdPortName =_tokens->opacity;
    } else if (mayaAttrName == _tokens->specularIOR) {
        usdPortName =_tokens->specularIOR;
    } else if (mayaAttrName == _tokens->normalCamera) {
        usdPortName =_tokens->normal;
    } else {
        TF_VERIFY(
            false,
            "Unsupported Maya attribute '%s'\n",
            mayaAttrName.GetText());
        return TfToken();
    }

    return TfToken(
                TfStringPrintf(
                    "%s%s",
                    usdIoDirection.GetText(),
                    usdPortName.GetText()).c_str());
}


PXR_NAMESPACE_CLOSE_SCOPE
