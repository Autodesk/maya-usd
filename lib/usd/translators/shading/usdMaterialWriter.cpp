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

#include "shadingTokens.h"

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <cmath>

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaShaderWriter::ContextSupport PxrUsdTranslators_MaterialWriter::CanExport(
    const UsdMayaJobExportArgs& exportArgs,
    const TfToken&              currentMaterialConversion)
{
    if (currentMaterialConversion == UsdImagingTokens->UsdPreviewSurface) {
        return ContextSupport::Supported;
    }
    // Only report as fallback if UsdPreviewSurface was not explicitly requested:
    if (exportArgs.convertMaterialsTo.count(UsdImagingTokens->UsdPreviewSurface) == 0) {
        ContextSupport::Fallback;
    }
    return ContextSupport::Unsupported;
}

PxrUsdTranslators_MaterialWriter::PxrUsdTranslators_MaterialWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
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

bool PxrUsdTranslators_MaterialWriter::AuthorShaderInputFromShadingNodeAttr(
    const MFnDependencyNode& depNodeFn,
    const TfToken&           shadingNodeAttrName,
    UsdShadeShader&          shaderSchema,
    const TfToken&           shaderInputName,
    const UsdTimeCode        usdTime,
    bool                     ignoreIfUnauthored,
    const SdfValueTypeName&  inputTypeName)
{
    return AuthorShaderInputFromScaledShadingNodeAttr(
        depNodeFn,
        shadingNodeAttrName,
        shaderSchema,
        shaderInputName,
        usdTime,
        TfToken(),
        ignoreIfUnauthored,
        inputTypeName);
}

bool PxrUsdTranslators_MaterialWriter::AuthorShaderInputFromScaledShadingNodeAttr(
    const MFnDependencyNode& depNodeFn,
    const TfToken&           shadingNodeAttrName,
    UsdShadeShader&          shaderSchema,
    const TfToken&           shaderInputName,
    const UsdTimeCode        usdTime,
    const TfToken&           scalingAttrName,
    bool                     ignoreIfUnauthored,
    const SdfValueTypeName&  inputTypeName)
{
    MStatus status;

    MPlug shadingNodePlug = depNodeFn.findPlug(
        depNodeFn.attribute(shadingNodeAttrName.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (ignoreIfUnauthored && !UsdMayaUtil::IsAuthored(shadingNodePlug)) {
        // Ignore this unauthored Maya attribute and return success.
        return true;
    }

    const bool isDestination = shadingNodePlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    const SdfValueTypeName shaderInputTypeName
        = bool(inputTypeName) ? inputTypeName : Converter::getUsdTypeName(shadingNodePlug);

    // Are color values are all linear on the shader?
    // Do we need to re-linearize them?
    VtValue value = UsdMayaWriteUtil::GetVtValue(
        shadingNodePlug,
        shaderInputTypeName,
        /* linearizeColors = */ false);

    if (value.IsEmpty()) {
        return false;
    }

    UsdShadeInput shaderInput = shaderSchema.CreateInput(shaderInputName, shaderInputTypeName);

    // For attributes that are the destination of a connection, we create
    // the input on the shader but we do *not* author a value for it. We
    // expect its actual value to come from the source of its connection.
    // We'll leave it to the shading export to handle creating
    // the connections in USD.
    if (!isDestination) {
        if (scalingAttrName != TfToken() && value.IsHolding<GfVec3f>()) {
            float colorScale = 1.0f;

            MPlug scalingPlug = depNodeFn.findPlug(
                depNodeFn.attribute(scalingAttrName.GetText()),
                /* wantNetworkedPlug = */ true,
                &status);
            if (status == MS::kSuccess) {
                colorScale = scalingPlug.asFloat();
            }

            value = value.UncheckedGet<GfVec3f>() * colorScale;
        }

        shaderInput.Set(value, usdTime);
    }

    return true;
}

/* virtual */
TfToken PxrUsdTranslators_MaterialWriter::GetShadingAttributeNameForMayaAttrName(
    const TfToken& mayaAttrName)
{
    if (mayaAttrName == TrMayaTokens->outColor) {
        return UsdShadeUtils::GetFullName(UsdShadeTokens->surface, UsdShadeAttributeType::Output);
    }

    TF_CODING_ERROR("Unsupported Maya attribute '%s'\n", mayaAttrName.GetText());
    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE
