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

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdTranslators_StandardSurfaceWriter : public PxrUsdTranslators_MaterialWriter
{
    typedef PxrUsdTranslators_MaterialWriter BaseClass;

public:
    PxrUsdTranslators_StandardSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(standardSurface, PxrUsdTranslators_StandardSurfaceWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (base)
    (baseColor)
    (emission)
    (emissionColor)
    (metalness)
    (specular)
    (specularColor)
    (specularIOR)
    (specularRoughness)
    (coat)
    (coatRoughness)
    (transmission)
    (normalCamera)
);
// clang-format on

PxrUsdTranslators_StandardSurfaceWriter::PxrUsdTranslators_StandardSurfaceWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : BaseClass(depNodeFn, usdPath, jobCtx)
{
}

/* virtual */
void PxrUsdTranslators_StandardSurfaceWriter::Write(const UsdTimeCode& usdTime)
{
    BaseClass::Write(usdTime);

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
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName,
        usdTime,
        _tokens->base);

    // Emission is modulated from emission weight:
    AuthorShaderInputFromScaledShadingNodeAttr(
        depNodeFn,
        _tokens->emissionColor,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName,
        usdTime,
        _tokens->emission);

    MPlug metalnessPlug = depNodeFn.findPlug(
        depNodeFn.attribute(_tokens->metalness.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status == MS::kSuccess && UsdMayaUtil::IsAuthored(metalnessPlug)) {
        AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->metalness,
            shaderSchema,
            PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName,
            usdTime);

        // IOR value from Gold USDPreviewSurface preset
        shaderSchema
            .CreateInput(PxrMayaUsdPreviewSurfaceTokens->IorAttrName, SdfValueTypeNames->Float)
            .Set(50.0f, usdTime);
    } else {
        shaderSchema
            .CreateInput(
                PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName, SdfValueTypeNames->Int)
            .Set(1, usdTime);

        AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->specularColor,
            shaderSchema,
            PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName,
            usdTime);

        AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            _tokens->specularIOR,
            shaderSchema,
            PxrMayaUsdPreviewSurfaceTokens->IorAttrName,
            usdTime);
    }

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->specularRoughness,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName,
        usdTime);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->coat,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName,
        usdTime);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->coatRoughness,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName,
        usdTime);

    MPlug transmissionPlug = depNodeFn.findPlug(
        depNodeFn.attribute(_tokens->transmission.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status == MS::kSuccess && UsdMayaUtil::IsAuthored(transmissionPlug)) {
        // Need a solution if the transmission is textured, but in the
        // meantime, we go 1 - transmission.
        float transmissionValue = transmissionPlug.asFloat();

        shaderSchema
            .CreateInput(PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName, SdfValueTypeNames->Float)
            .Set(1.0f - transmissionValue, usdTime);
    }

    // Exported, but unsupported in hdStorm.
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        _tokens->normalCamera,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName,
        usdTime,
        /* ignoreIfUnauthored = */ false,
        /* inputTypeName = */ SdfValueTypeNames->Normal3f);
}

/* virtual */
TfToken PxrUsdTranslators_StandardSurfaceWriter::GetShadingAttributeNameForMayaAttrName(
    const TfToken& mayaAttrName)
{
    TfToken usdAttrName;

    if (mayaAttrName == _tokens->baseColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName;
    } else if (mayaAttrName == _tokens->emissionColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName;
    } else if (mayaAttrName == _tokens->metalness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName;
    } else if (mayaAttrName == _tokens->specularColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName;
    } else if (mayaAttrName == _tokens->specularIOR) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->IorAttrName;
    } else if (mayaAttrName == _tokens->specularRoughness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName;
    } else if (mayaAttrName == _tokens->coat) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName;
    } else if (mayaAttrName == _tokens->coatRoughness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName;
    } else if (mayaAttrName == _tokens->normalCamera) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->NormalAttrName;
    } else {
        return BaseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }

    return UsdShadeUtils::GetFullName(usdAttrName, UsdShadeAttributeType::Input);
}

PXR_NAMESPACE_CLOSE_SCOPE
