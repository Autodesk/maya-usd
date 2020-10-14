//
// Copyright 2018 Pixar
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
#include "usdPreviewSurfaceWriter.h"

#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

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
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION(
    UsdImagingTokens->UsdPreviewSurface,
    UsdShadeTokens->universalRenderContext,
    PxrMayaUsdPreviewSurfaceTokens->niceName,
    PxrMayaUsdPreviewSurfaceTokens->exportDescription);

UsdMayaShaderWriter::ContextSupport
PxrMayaUsdPreviewSurface_Writer::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    return exportArgs.convertMaterialsTo == UsdImagingTokens->UsdPreviewSurface
        ? ContextSupport::Supported
        : ContextSupport::Fallback;
}

PxrMayaUsdPreviewSurface_Writer::PxrMayaUsdPreviewSurface_Writer(
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

    shaderSchema.CreateIdAttr(VtValue(UsdImagingTokens->UsdPreviewSurface));

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
        UsdShadeShader& shaderSchema,
        const TfToken& shaderInputName,
        const SdfValueTypeName& shaderInputTypeName,
        const UsdTimeCode usdTime,
        const bool mayaBoolAsUsdInt = false)
{
    MStatus status;

    // If the USD shader input type is int but the Maya attribute type is bool,
    // we do a conversion (e.g. for "useSpecularWorkflow").
    const bool convertBoolToInt = (mayaBoolAsUsdInt &&
        (shaderInputTypeName == SdfValueTypeNames->Int));

    MPlug shadingNodePlug =
        depNodeFn.findPlug(
            shaderInputName.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return false;
    }

    const bool isDestination = shadingNodePlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(shadingNodePlug)) {
        // Color values are all linear on the shader, so do not re-linearize
        // them.
        VtValue value =
            UsdMayaWriteUtil::GetVtValue(
                shadingNodePlug,
                convertBoolToInt ?
                    SdfValueTypeNames->Bool :
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
            if (convertBoolToInt) {
                if (value.UncheckedGet<bool>()) {
                    value = 1;
                } else {
                    value = 0;
                }
            }

            shaderInput.Set(value, usdTime);
        }
    }

    return true;
}

/* virtual */
void
PxrMayaUsdPreviewSurface_Writer::Write(const UsdTimeCode& usdTime)
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

    // Clearcoat
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Clearcoat Roughness
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Diffuse Color
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName,
        SdfValueTypeNames->Color3f,
        usdTime);

    // Displacement
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Emissive Color
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName,
        SdfValueTypeNames->Color3f,
        usdTime);

    // Ior
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->IorAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Metallic
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Normal
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName,
        SdfValueTypeNames->Normal3f,
        usdTime);

    // Occlusion
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Opacity
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Roughness
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Specular Color
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName,
        SdfValueTypeNames->Color3f,
        usdTime);

    // Use Specular Workflow
    // The Maya attribute is bool-typed, while the USD attribute is int-typed.
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName,
        SdfValueTypeNames->Int,
        usdTime,
        /* mayaBoolAsUsdInt = */ true);
}

/* virtual */
TfToken
PxrMayaUsdPreviewSurface_Writer::GetShadingAttributeNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    if (!_usdPrim) {
        return TfToken();
    }

    TfToken usdAttrName;

    if (mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName) {
        usdAttrName =
            TfToken(
                TfStringPrintf(
                    "%s%s",
                    UsdShadeTokens->outputs.GetText(),
                    UsdShadeTokens->surface.GetText()).c_str());
    }
    else if (mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->IorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName) {
        usdAttrName =
            TfToken(
                TfStringPrintf(
                    "%s%s",
                    UsdShadeTokens->inputs.GetText(),
                    mayaAttrName.GetText()).c_str());
    }

    return usdAttrName;
}


PXR_NAMESPACE_CLOSE_SCOPE
