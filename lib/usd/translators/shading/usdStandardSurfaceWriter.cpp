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
#include "usdMaterialWriter.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MStatus.h>

#include <pxr/pxr.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdTranslators_StandardSurfaceWriter
    : public PxrUsdTranslators_MaterialWriter
{
    typedef PxrUsdTranslators_MaterialWriter baseClass;

    public:
        PxrUsdTranslators_StandardSurfaceWriter(
                const MFnDependencyNode& depNodeFn,
                const SdfPath& usdPath,
                UsdMayaWriteJobContext& jobCtx);

        void Write(const UsdTimeCode& usdTime) override;

        TfToken GetShadingAttributeNameForMayaAttrName(
                const TfToken& mayaAttrName) override;
};

PXRUSDMAYA_REGISTER_WRITER(
    standardSurface,
    PxrUsdTranslators_StandardSurfaceWriter);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (base)
    (baseColor)
    (emission)
    (emissionColor)
    (metalness)
    (specular)
    (specularIOR)
    (specularRoughness)
    (coat)
    (coatRoughness)
    (transmission)
    (normalCamera)

    // UsdPreviewSurface
    (diffuseColor)
    (emissiveColor)
    (metallic)
    (ior)
    (useSpecularWorkflow)
    (specularColor)
    (roughness)
    (clearcoat)
    (clearcoatRoughness)
    (opacity)
    (normal)
);

PxrUsdTranslators_StandardSurfaceWriter::PxrUsdTranslators_StandardSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx)
    : baseClass(depNodeFn, usdPath, jobCtx) {}

/* virtual */
void
PxrUsdTranslators_StandardSurfaceWriter::Write(const UsdTimeCode& usdTime)
{
    baseClass::Write(usdTime);

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

    AuthorShaderInputFromScaledShadingNodeAttr(
        depNodeFn,
        _tokens->baseColor,
        shaderSchema,
        _tokens->diffuseColor,
        usdTime,
        _tokens->base);

    // Emission is modulated from emission weight:
    AuthorShaderInputFromScaledShadingNodeAttr(
        depNodeFn,
        _tokens->emissionColor,
        shaderSchema,
        _tokens->emissiveColor,
        usdTime,
        _tokens->emission);

    MPlug metalnessPlug =
        depNodeFn.findPlug(
            depNodeFn.attribute(_tokens->metalness.GetText()),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status == MS::kSuccess && UsdMayaUtil::IsAuthored(metalnessPlug)) {
        AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->metalness,
            shaderSchema,
            _tokens->metallic,
            usdTime);

        // IOR value from Gold USDPreviewSurface preset
        shaderSchema.CreateInput(
            _tokens->ior,
            SdfValueTypeNames->Float).Set(50.0f, usdTime);
    } else {
        shaderSchema.CreateInput(
            _tokens->useSpecularWorkflow,
            SdfValueTypeNames->Int).Set(1, usdTime);

        AuthorShaderInputFromScaledShadingNodeAttr(
            depNodeFn,
            _tokens->specularColor,
            shaderSchema,
            _tokens->specularColor,
            usdTime,
            _tokens->specular);

        AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->specularIOR,
            shaderSchema,
            _tokens->ior,
            usdTime);
    }

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->specularRoughness,
        shaderSchema,
        _tokens->roughness,
        usdTime);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->coat,
        shaderSchema,
        _tokens->clearcoat,
        usdTime);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->coatRoughness,
        shaderSchema,
        _tokens->clearcoatRoughness,
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
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->normalCamera,
        shaderSchema,
        _tokens->normal,
        usdTime);

}

/* virtual */
TfToken
PxrUsdTranslators_StandardSurfaceWriter::GetShadingAttributeNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    if (!_usdPrim) {
        return TfToken();
    }

    TfToken usdPortName;

    if (mayaAttrName == _tokens->baseColor) {
        usdPortName =_tokens->diffuseColor;
    } else if (mayaAttrName == _tokens->emissionColor) {
        usdPortName =_tokens->emissiveColor;
    } else if (mayaAttrName == _tokens->metalness) {
        usdPortName =_tokens->metallic;
    } else if (mayaAttrName == _tokens->specularColor) {
        usdPortName =_tokens->specularColor;
    } else if (mayaAttrName == _tokens->specularIOR) {
        usdPortName =_tokens->ior;
    } else if (mayaAttrName == _tokens->specularRoughness) {
        usdPortName =_tokens->roughness;
    } else if (mayaAttrName == _tokens->coat) {
        usdPortName =_tokens->clearcoat;
    } else if (mayaAttrName == _tokens->coatRoughness) {
        usdPortName =_tokens->clearcoatRoughness;
    } else if (mayaAttrName == _tokens->normalCamera) {
        usdPortName =_tokens->normal;
    } else {
        return baseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }

    return TfToken(
                TfStringPrintf(
                    "%s%s",
                    UsdShadeTokens->inputs,
                    usdPortName.GetText()).c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
