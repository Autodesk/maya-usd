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
#include "shadingTokens.h"
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
        TrMayaTokens->baseColor,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName,
        usdTime,
        TrMayaTokens->base);

    // Emission is modulated from emission weight:
    AuthorShaderInputFromScaledShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->emissionColor,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName,
        usdTime,
        TrMayaTokens->emission);

    MPlug metalnessPlug = depNodeFn.findPlug(
        depNodeFn.attribute(TrMayaTokens->metalness.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status == MS::kSuccess && UsdMayaUtil::IsAuthored(metalnessPlug)) {
        AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            TrMayaTokens->metalness,
            shaderSchema,
            PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName,
            usdTime);
    } else {
        shaderSchema
            .CreateInput(
                PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName, SdfValueTypeNames->Int)
            .Set(1, usdTime);

        AuthorShaderInputFromShadingNodeAttr(
            depNodeFn,
            TrMayaTokens->specularColor,
            shaderSchema,
            PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName,
            usdTime);
    }

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->specularIOR,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->IorAttrName,
        usdTime);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->specularRoughness,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName,
        usdTime);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->coat,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName,
        usdTime);

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->coatRoughness,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName,
        usdTime);

    MPlug transmissionPlug = depNodeFn.findPlug(
        depNodeFn.attribute(TrMayaTokens->transmission.GetText()),
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
        TrMayaTokens->normalCamera,
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

    if (mayaAttrName == TrMayaTokens->baseColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName;
    } else if (mayaAttrName == TrMayaTokens->emissionColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName;
    } else if (mayaAttrName == TrMayaTokens->metalness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName;
    } else if (mayaAttrName == TrMayaTokens->specularColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName;
    } else if (mayaAttrName == TrMayaTokens->specularIOR) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->IorAttrName;
    } else if (mayaAttrName == TrMayaTokens->specularRoughness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName;
    } else if (mayaAttrName == TrMayaTokens->coat) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName;
    } else if (mayaAttrName == TrMayaTokens->coatRoughness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName;
    } else if (mayaAttrName == TrMayaTokens->normalCamera) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->NormalAttrName;
    } else {
        return BaseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }

    return UsdShadeUtils::GetFullName(usdAttrName, UsdShadeAttributeType::Input);
}

PXR_NAMESPACE_CLOSE_SCOPE
