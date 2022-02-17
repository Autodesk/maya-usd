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

#include "shadingTokens.h"

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/shading/symmetricShaderReader.h>
#include <mayaUsd/fileio/shading/symmetricShaderWriter.h>
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

    ((nodeGraphPrefix, "MayaNG"))

    // Prefix for conversion nodes:
    ((ConverterPrefix, "MayaConvert"))
    ((SwizzlePrefix, "MayaSwizzle"))
    ((LuminancePrefix, "MayaLuminance"))
    ((NormalMapPrefix, "MayaNormalMap"))

    (lambert)
    (MayaND_lambert_surfaceshader)
    (phong)
    (MayaND_phong_surfaceshader)
    (blinn)
    (MayaND_blinn_surfaceshader)
);
// clang-format on

REGISTER_SHADING_MODE_EXPORT_MATERIAL_CONVERSION(
    TrMtlxTokens->conversionName,
    TrMtlxTokens->contextName,
    TrMtlxTokens->niceName,
    TrMtlxTokens->exportDescription);

// Register symmetric writers:
TF_REGISTRY_FUNCTION(UsdMayaShaderWriterRegistry)
{
    UsdMayaSymmetricShaderWriter::RegisterWriter(
        _tokens->lambert, _tokens->MayaND_lambert_surfaceshader, TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderWriter::RegisterWriter(
        _tokens->phong, _tokens->MayaND_phong_surfaceshader, TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderWriter::RegisterWriter(
        _tokens->blinn, _tokens->MayaND_blinn_surfaceshader, TrMtlxTokens->conversionName);
};

TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)
{
    UsdMayaSymmetricShaderReader::RegisterReader(
        _tokens->MayaND_lambert_surfaceshader, _tokens->lambert, TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderReader::RegisterReader(
        _tokens->MayaND_phong_surfaceshader, _tokens->phong, TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderReader::RegisterReader(
        _tokens->MayaND_blinn_surfaceshader, _tokens->blinn, TrMtlxTokens->conversionName);
};

UsdMayaShaderWriter::ContextSupport
MtlxUsd_BaseWriter::CanExport(const UsdMayaJobExportArgs& exportArgs)
{
    return exportArgs.convertMaterialsTo == TrMtlxTokens->conversionName
        ? ContextSupport::Supported
        : ContextSupport::Unsupported;
}

MtlxUsd_BaseWriter::MtlxUsd_BaseWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
}

UsdPrim MtlxUsd_BaseWriter::GetNodeGraph()
{
    SdfPath materialPath = GetUsdPath().GetParentPath();
    TfToken ngName(TfStringPrintf(
        "%s_%s", _tokens->nodeGraphPrefix.GetText(), materialPath.GetName().c_str()));
    SdfPath ngPath = materialPath.AppendChild(ngName);
    return UsdShadeNodeGraph::Define(GetUsdStage(), ngPath).GetPrim();
}

UsdAttribute MtlxUsd_BaseWriter::AddConversion(
    const SdfValueTypeName& fromType,
    const SdfValueTypeName& toType,
    UsdAttribute            nodeOutput)
{

    // We currently only support color3f to vector3f for normal maps:
    if (fromType == SdfValueTypeNames->Color3f && toType == SdfValueTypeNames->Float3) {
        MStatus                 status;
        const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
        if (status != MS::kSuccess) {
            return UsdAttribute();
        }

        UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
        SdfPath           nodegraphPath = nodegraphSchema.GetPath();

        TfToken        converterName(TfStringPrintf(
            "%s_%s_%s_%s",
            _tokens->ConverterPrefix.GetText(),
            UsdMayaUtil::SanitizeName(depNodeFn.name().asChar()).c_str(),
            fromType.GetAsToken().GetText(),
            toType.GetAsToken().GetText()));
        const SdfPath  converterPath = nodegraphPath.AppendChild(converterName);
        UsdShadeShader converterSchema = UsdShadeShader::Define(GetUsdStage(), converterPath);

        UsdAttribute converterOutput = converterSchema.GetOutput(TrMtlxTokens->out);
        if (converterOutput) {
            // Reusing existing node:
            return converterOutput;
        }

        converterSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_convert_color3_vector3));
        converterSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Color3f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        converterOutput
            = converterSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float3);
        return converterOutput;
    }

    return UsdAttribute();
}

UsdAttribute MtlxUsd_BaseWriter::AddSwizzle(const std::string& channels, int numChannels)
{
    UsdAttribute nodeOutput = UsdShadeShader(_usdPrim).GetOutput(TrMtlxTokens->out);
    return AddSwizzle(channels, numChannels, nodeOutput);
}

UsdAttribute MtlxUsd_BaseWriter::AddSwizzle(
    const std::string& channels,
    int                numChannels,
    UsdAttribute       nodeOutput)
{
    if (numChannels == static_cast<int>(channels.size())) {
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

    UsdAttribute swizzleOutput = swizzleSchema.GetOutput(TrMtlxTokens->out);
    if (swizzleOutput) {
        // Reusing existing node:
        return swizzleOutput;
    }

    // The swizzle name varies according to source and destination channel sizes:
    std::string srcType, dstType;

    switch (numChannels) {
    case 1:
        srcType = "float";
        swizzleSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Float)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    case 2:
        srcType = "vector2";
        swizzleSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Float2)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    case 3:
        srcType = "color3";
        swizzleSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Color3f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    case 4:
        srcType = "color4";
        swizzleSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Color4f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        break;
    default: TF_CODING_ERROR("Unsupported format for swizzle"); return UsdAttribute();
    }

    swizzleSchema.CreateInput(TrMtlxTokens->channels, SdfValueTypeNames->String)
        .Set(channels, UsdTimeCode::Default());

    switch (channels.size()) {
    case 1:
        dstType = "float";
        swizzleOutput = swizzleSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float);
        break;
    case 2:
        dstType = "vector2";
        swizzleOutput = swizzleSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float2);
        break;
    case 3:
        dstType = "color3";
        swizzleOutput = swizzleSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Color3f);
        break;
    case 4:
        dstType = "color4";
        swizzleOutput = swizzleSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Color4f);
        break;
    }

    TfToken swizzleID(TfStringPrintf("ND_swizzle_%s_%s", srcType.c_str(), dstType.c_str()));

    swizzleSchema.CreateIdAttr(VtValue(swizzleID));

    return swizzleOutput;
}

UsdAttribute MtlxUsd_BaseWriter::AddLuminance(int numChannels)
{
    UsdAttribute nodeOutput = UsdShadeShader(_usdPrim).GetOutput(TrMtlxTokens->out);
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

    TfToken        luminanceName(TfStringPrintf(
        "%s_%s",
        _tokens->LuminancePrefix.GetText(),
        UsdMayaUtil::SanitizeName(depNodeFn.name().asChar()).c_str()));
    const SdfPath  luminancePath = nodegraphPath.AppendChild(luminanceName);
    UsdShadeShader luminanceSchema = UsdShadeShader::Define(GetUsdStage(), luminancePath);

    UsdAttribute luminanceOutput = luminanceSchema.GetOutput(TrMtlxTokens->out);
    if (luminanceOutput) {
        // Reusing existing node:
        return luminanceOutput;
    }

    switch (numChannels) {
    case 3:
        luminanceSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_luminance_color3));
        luminanceSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Color3f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        luminanceOutput
            = luminanceSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Color3f);
        break;
    case 4:
        luminanceSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_luminance_color4));
        luminanceSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Color4f)
            .ConnectToSource(UsdShadeOutput(nodeOutput));
        luminanceOutput
            = luminanceSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Color4f);
        break;
    default: TF_CODING_ERROR("Unsupported format for luminance"); return UsdAttribute();
    }

    return AddSwizzle("r", numChannels, luminanceOutput);
}

UsdAttribute MtlxUsd_BaseWriter::AddNormalMapping(UsdAttribute normalInput)
{
    // For standard surface (and not preview surface)

    // We are starting at the NodeGraph boundary and building a chain that will
    // eventually reach an image node
    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return UsdAttribute();
    }

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    SdfPath           nodegraphPath = nodegraphSchema.GetPath();

    // Normal map:
    TfToken        nodeName(TfStringPrintf(
        "%s_%s_%s",
        _tokens->NormalMapPrefix.GetText(),
        UsdMayaUtil::SanitizeName(depNodeFn.name().asChar()).c_str(),
        normalInput.GetBaseName().GetText()));
    SdfPath        nodePath = nodegraphPath.AppendChild(nodeName);
    UsdShadeShader nodeSchema = UsdShadeShader::Define(GetUsdStage(), nodePath);
    nodeSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_normalmap));
    UsdShadeInput  mapInput = nodeSchema.CreateInput(TrMtlxTokens->in, SdfValueTypeNames->Float3);
    UsdShadeOutput mapOutput
        = nodeSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float3);
    UsdShadeOutput(normalInput).ConnectToSource(UsdShadeOutput(mapOutput));

    return mapInput;
}

PXR_NAMESPACE_CLOSE_SCOPE
