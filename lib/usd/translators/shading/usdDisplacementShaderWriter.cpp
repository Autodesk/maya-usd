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
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

using namespace MAYAUSD_NS;

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdTranslators_DisplacementShaderWriter : public UsdMayaShaderWriter
{
    using BaseClass = UsdMayaShaderWriter;

public:
    PxrUsdTranslators_DisplacementShaderWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    static ContextSupport CanExport(const UsdMayaJobExportArgs&);

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(displacementShader, PxrUsdTranslators_DisplacementShaderWriter);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya material nodes attribute names
    (displacement));

PxrUsdTranslators_DisplacementShaderWriter::PxrUsdTranslators_DisplacementShaderWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : BaseClass(depNodeFn, usdPath, jobCtx)
{
    // This node will search for the surface output of the material and if it
    // finds an existing UsdPreviewSurface, it will merge the displacement in.
    UsdShadeMaterial shadeMaterial = UsdShadeMaterial::Get(GetUsdStage(), usdPath.GetParentPath());
    if (shadeMaterial) {
        UsdShadeShader surfaceShader = shadeMaterial.ComputeSurfaceSource();
        if (surfaceShader) {
            TfToken shaderId;
            surfaceShader.GetIdAttr().Get(&shaderId);
            if (shaderId == UsdImagingTokens->UsdPreviewSurface) {
                _usdPrim = surfaceShader.GetPrim();
                return;
            }
        }
    }

    // Existing UsdPreviewSurface not found. Create one.
    UsdShadeShader shaderSchema = UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    UsdAttribute idAttr = shaderSchema.CreateIdAttr(VtValue(UsdImagingTokens->UsdPreviewSurface));

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }

    // Surface Output
    shaderSchema.CreateOutput(UsdShadeTokens->surface, SdfValueTypeNames->Token);

    // Displacement Output
    shaderSchema.CreateOutput(UsdShadeTokens->displacement, SdfValueTypeNames->Token);
}

/* virtual */
void PxrUsdTranslators_DisplacementShaderWriter::Write(const UsdTimeCode& usdTime)
{
    BaseClass::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!shaderSchema) {
        return;
    }

    MPlug displacementPlug
        = depNodeFn.findPlug(depNodeFn.attribute(_tokens->displacement.GetText()), true, &status);
    if (status != MS::kSuccess) {
        return;
    }

    const bool isDestination = displacementPlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return;
    }

    auto shaderInputTypeName = Converter::getUsdTypeName(displacementPlug);

    VtValue value = UsdMayaWriteUtil::GetVtValue(displacementPlug, shaderInputTypeName, false);

    if (value.IsEmpty()) {
        return;
    }

    UsdShadeInput shaderInput = shaderSchema.CreateInput(
        PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName, shaderInputTypeName);

    if (!isDestination) {
        shaderInput.Set(value, usdTime);
    }
}

/* static */
UsdMayaShaderWriter::ContextSupport
PxrUsdTranslators_DisplacementShaderWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    return exportArgs.convertMaterialsTo == UsdImagingTokens->UsdPreviewSurface
        ? ContextSupport::Supported
        : ContextSupport::Fallback;
}

/* virtual */
TfToken PxrUsdTranslators_DisplacementShaderWriter::GetShadingAttributeNameForMayaAttrName(
    const TfToken& mayaAttrName)
{
    if (mayaAttrName == _tokens->displacement) {
        return UsdShadeUtils::GetFullName(
            PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName, UsdShadeAttributeType::Input);
    }

    // Not returning an output for this exporter. The displacement output got
    // connected when the surface got exported.
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
