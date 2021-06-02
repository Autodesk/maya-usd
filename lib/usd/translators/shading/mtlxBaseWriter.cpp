//
// Copyright 2021 Autodesk
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
#include "mtlxBaseWriter.h"

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
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <cmath>

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((conversionName, "MaterialX"))
    ((contextName, "mtlx"))
    ((niceName, "MaterialX shading"))
    ((exportDescription, "Exports bound shaders as a MaterialX UsdShade network."))

    ((nodeGraphPrefix, "MayaNG"))

    // Prefix for conversion nodes:
    ((SwizzlePrefix, "MayaSwizzle"))
    ((LuminancePrefix, "MayaLuminance"))

    // Conversion nodes:
    (ND_luminance_color3)
    (ND_luminance_color4)

    // Conversion ports:
    (in)
    (out)
    (channels)
);
// clang-format on

REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION(
    _tokens->conversionName,
    _tokens->contextName,
    _tokens->niceName,
    _tokens->exportDescription);

UsdMayaShaderWriter::ContextSupport
MaterialXTranslators_BaseWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    return exportArgs.convertMaterialsTo == _tokens->conversionName ? ContextSupport::Supported
                                                                    : ContextSupport::Unsupported;
}

MaterialXTranslators_BaseWriter::MaterialXTranslators_BaseWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
}

UsdPrim MaterialXTranslators_BaseWriter::GetNodeGraph()
{
    SdfPath materialPath = GetUsdPath().GetParentPath();
    TfToken ngName(TfStringPrintf(
        "%s_%s", _tokens->nodeGraphPrefix.GetText(), materialPath.GetName().c_str()));
    SdfPath ngPath = materialPath.AppendChild(ngName);
    return UsdShadeNodeGraph::Define(GetUsdStage(), ngPath).GetPrim();
}

UsdAttribute
MaterialXTranslators_BaseWriter::AddSwizzle(const std::string& channels, int numChannels)
{
    UsdAttribute nodeOutput = UsdShadeShader(_usdPrim).GetOutput(_tokens->out);
    return AddSwizzle(channels, numChannels, nodeOutput);
}

UsdAttribute MaterialXTranslators_BaseWriter::AddSwizzle(
    const std::string& channels,
    int                numChannels,
    UsdAttribute       nodeOutput)
{
    if (numChannels == channels.size()) {
        // No swizzle actually needed:
        return nodeOutput;
    }

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    SdfPath           nodegraphPath = nodegraphSchema.GetPath();

    SdfPath outputPath = nodeOutput.GetPath().GetParentPath();

    TfToken        swizzleName(TfStringPrintf(
        "%s_%s_%s",
        _tokens->SwizzlePrefix.GetText(),
        outputPath.GetName().c_str(),
        channels.c_str()));
    const SdfPath  swizzlePath = nodegraphPath.AppendChild(swizzleName);
    UsdShadeShader swizzleSchema = UsdShadeShader::Define(GetUsdStage(), swizzlePath);

    UsdAttribute swizzleOutput = swizzleSchema.GetOutput(_tokens->out);
    if (swizzleOutput) {
        // Reusing existing node:
        return swizzleOutput;
    }

    // The swizzle name varies according to source and destination channel sizes:
    std::string srcType, dstType;

    switch (numChannels) {
    case 1:
        srcType = "float";
        swizzleSchema.CreateInput(_tokens->in, SdfValueTypeNames->Float)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    case 2:
        srcType = "vector2";
        swizzleSchema.CreateInput(_tokens->in, SdfValueTypeNames->Float2)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    case 3:
        srcType = "color3";
        swizzleSchema.CreateInput(_tokens->in, SdfValueTypeNames->Color3f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    case 4:
        srcType = "color4";
        swizzleSchema.CreateInput(_tokens->in, SdfValueTypeNames->Color4f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    default: TF_CODING_ERROR("Unsupported format for swizzle"); return UsdAttribute();
    }

    swizzleSchema.CreateInput(_tokens->channels, SdfValueTypeNames->String)
        .Set(channels, UsdTimeCode::Default());

    switch (channels.size()) {
    case 1:
        dstType = "float";
        swizzleOutput = swizzleSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Float);
        break;
    case 2:
        dstType = "vector2";
        swizzleOutput = swizzleSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Float2);
        break;
    case 3:
        dstType = "color3";
        swizzleOutput = swizzleSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Color3f);
        break;
    case 4:
        dstType = "color4";
        swizzleOutput = swizzleSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Color4f);
        break;
    }

    TfToken swizzleID(TfStringPrintf("ND_swizzle_%s_%s", srcType.c_str(), dstType.c_str()));

    swizzleSchema.CreateIdAttr(VtValue(swizzleID));

    return swizzleOutput;
}

UsdAttribute MaterialXTranslators_BaseWriter::AddLuminance(int numChannels)
{
    UsdAttribute nodeOutput = UsdShadeShader(_usdPrim).GetOutput(_tokens->out);
    if (numChannels < 3) {
        // Not enough channels:
        return nodeOutput;
    }

    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return UsdAttribute();
    }

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    SdfPath           nodegraphPath = nodegraphSchema.GetPath();

    TfToken luminanceName(
        TfStringPrintf("%s_%s", _tokens->LuminancePrefix.GetText(), depNodeFn.name().asChar()));
    const SdfPath  luminancePath = nodegraphPath.AppendChild(luminanceName);
    UsdShadeShader luminanceSchema = UsdShadeShader::Define(GetUsdStage(), luminancePath);

    UsdAttribute luminanceOutput = luminanceSchema.GetOutput(_tokens->out);
    if (luminanceOutput) {
        // Reusing existing node:
        return luminanceOutput;
    }

    switch (numChannels) {
    case 3:
        luminanceSchema.CreateIdAttr(VtValue(_tokens->ND_luminance_color3));
        luminanceSchema.CreateInput(_tokens->in, SdfValueTypeNames->Color3f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        luminanceOutput = luminanceSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Color3f);
        break;
    case 4:
        luminanceSchema.CreateIdAttr(VtValue(_tokens->ND_luminance_color4));
        luminanceSchema.CreateInput(_tokens->in, SdfValueTypeNames->Color4f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        luminanceOutput = luminanceSchema.CreateOutput(_tokens->out, SdfValueTypeNames->Color4f);
        break;
    default: TF_CODING_ERROR("Unsupported format for luminance"); return UsdAttribute();
    }

    return AddSwizzle("r", numChannels, luminanceOutput);
}

PXR_NAMESPACE_CLOSE_SCOPE
