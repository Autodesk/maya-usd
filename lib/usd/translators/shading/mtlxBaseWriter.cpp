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
        TrMayaTokens->lambert,
        TrMtlxTokens->MayaND_lambert_surfaceshader,
        TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->phong,
        TrMtlxTokens->MayaND_phong_surfaceshader,
        TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderWriter::RegisterWriter(
        TrMayaTokens->blinn,
        TrMtlxTokens->MayaND_blinn_surfaceshader,
        TrMtlxTokens->conversionName);
};

TF_REGISTRY_FUNCTION(UsdMayaShaderReaderRegistry)
{
    UsdMayaSymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_lambert_surfaceshader,
        TrMayaTokens->lambert,
        TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_phong_surfaceshader,
        TrMayaTokens->phong,
        TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_blinn_surfaceshader,
        TrMayaTokens->blinn,
        TrMtlxTokens->conversionName);
    UsdMayaSymmetricShaderReader::RegisterReader(
        TrMtlxTokens->MayaND_place2dTexture_vector2,
        TrMayaTokens->place2dTexture,
        TrMtlxTokens->conversionName);
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

UsdAttribute
MtlxUsd_BaseWriter::AddSwizzleConversion(const SdfValueTypeName& destType, UsdAttribute nodeOutput)
{
    const SdfValueTypeName& sourceType = nodeOutput.GetTypeName();
    if (sourceType == destType) {
        // No swizzle actually needed:
        return nodeOutput;
    }

    struct _SwizzleData
    {
        const SdfValueTypeName _fromName;
        const SdfValueTypeName _toName;
        const std::string      _fromType;
        const std::string      _toType;
        const std::string      _channels;
        _SwizzleData(
            const SdfValueTypeName fromName,
            const SdfValueTypeName toName,
            std::string&&          fromType,
            std::string&&          toType,
            std::string&&          channels)
            : _fromName(fromName)
            , _toName(toName)
            , _fromType(std::move(fromType))
            , _toType(std::move(toType))
            , _channels(std::move(channels))
        {
        }
    };

    // There are 6 types to handle. Not enough that a sorted container would help. Grouping them
    // will allow finding any item in maximum 10 iterations by skipping whole chunks. Putting Float3
    // and Float4 last since they are the least likely to be searched for.
    typedef std::vector<_SwizzleData> _SwizzleMap;
    static const _SwizzleMap          _swizzleMap = _SwizzleMap {
        { SdfValueTypeNames->Float, SdfValueTypeNames->Float2, "float", "vector2", "xx" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Color3f, "float", "color3", "xxx" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Color4f, "float", "color4", "xxx1" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Float3, "float", "vector3", "xxx" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Float4, "float", "vector4", "xxxx" },

        { SdfValueTypeNames->Float2, SdfValueTypeNames->Float, "vector2", "float", "x" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Color3f, "vector2", "color3", "xyy" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Color4f, "vector2", "color4", "xyyy" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Float3, "vector2", "vector3", "xyy" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Float4, "vector2", "vector4", "xxyy" },

        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float, "color3", "float", "r" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float2, "color3", "vector2", "rg" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Color4f, "color3", "color4", "rgb1" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float3, "color3", "vector3", "rgb" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float4, "color3", "vector4", "rgb1" },

        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float, "color4", "float", "r" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float2, "color4", "vector2", "rg" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Color3f, "color4", "color3", "rgb" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float3, "color4", "vector3", "rgb" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float4, "color4", "vector4", "rgba" },

        { SdfValueTypeNames->Float3, SdfValueTypeNames->Float, "vector3", "float", "x" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Float2, "vector3", "vector2", "xy" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Color3f, "vector3", "color3", "xyz" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Color4f, "vector3", "color4", "xyz1" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Float4, "vector3", "vector4", "xyz1" },

        { SdfValueTypeNames->Float4, SdfValueTypeNames->Float, "vector4", "float", "x" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Float2, "vector4", "vector2", "xy" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Color3f, "vector4", "color3", "xyz" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Color4f, "vector4", "color4", "xyzw" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Float3, "vector4", "vector3", "xyz" },
    };

    const std::string* srcType = nullptr;
    const std::string* dstType = nullptr;
    const std::string* channels = nullptr;
    // Search with stride 5 for sourceType
    for (size_t fromIdx = 0; !srcType && fromIdx < _swizzleMap.size(); fromIdx += 5) {
        if (_swizzleMap[fromIdx]._fromName == sourceType) {
            // Refine at stride 1 for destType
            for (size_t toIdx = 0; !srcType && toIdx < 5; ++toIdx) {
                if (_swizzleMap[fromIdx + toIdx]._toName == destType) {
                    srcType = &_swizzleMap[fromIdx + toIdx]._fromType;
                    dstType = &_swizzleMap[fromIdx + toIdx]._toType;
                    channels = &_swizzleMap[fromIdx + toIdx]._channels;
                }
            }
        }
    }

    if (!srcType) {
        TF_CODING_ERROR(
            "Could not swizzle from %s to %s",
            sourceType.GetAsToken().GetText(),
            destType.GetAsToken().GetText());
        return {};
    }

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    SdfPath           nodegraphPath = nodegraphSchema.GetPath();

    SdfPath outputPath = nodeOutput.GetPath().GetParentPath();

    TfToken        swizzleName(TfStringPrintf(
        "%s_%s_%s",
        _tokens->SwizzlePrefix.GetText(),
        outputPath.GetName().c_str(),
        channels->c_str()));
    const SdfPath  swizzlePath = nodegraphPath.AppendChild(swizzleName);
    UsdShadeShader swizzleSchema = UsdShadeShader::Define(GetUsdStage(), swizzlePath);

    UsdAttribute swizzleOutput = swizzleSchema.GetOutput(TrMtlxTokens->out);
    if (swizzleOutput) {
        // Reusing existing node:
        return swizzleOutput;
    }

    // The swizzle name varies according to source and destination channel sizes:
    swizzleSchema.CreateInput(TrMtlxTokens->in, sourceType)
        .ConnectToSource(UsdShadeOutput(nodeOutput));

    swizzleSchema.CreateInput(TrMtlxTokens->channels, SdfValueTypeNames->String)
        .Set(*channels, UsdTimeCode::Default());

    swizzleOutput = swizzleSchema.CreateOutput(TrMtlxTokens->out, destType);

    TfToken swizzleID(TfStringPrintf("ND_swizzle_%s_%s", srcType->c_str(), dstType->c_str()));

    swizzleSchema.CreateIdAttr(VtValue(swizzleID));

    return swizzleOutput;
}

UsdAttribute MtlxUsd_BaseWriter::AddLuminance(int numChannels, UsdAttribute nodeOutput)
{
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

bool MtlxUsd_BaseWriter::AuthorShaderInputFromShadingNodeAttr(
    const MFnDependencyNode& depNodeFn,
    const TfToken&           shadingNodeAttrName,
    UsdShadeShader&          shaderSchema,
    const UsdTimeCode        usdTime,
    bool                     ignoreIfUnauthored)
{
    MStatus status;

    MPlug shadingNodePlug = depNodeFn.findPlug(
        depNodeFn.attribute(shadingNodeAttrName.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return false;
    }

    SdfValueTypeName shaderInputTypeName = Converter::getUsdTypeName(shadingNodePlug);

    // We know exactly which types are supported by MaterialX, so we can adjust here:
    if (shaderInputTypeName == SdfValueTypeNames->Double) {
        shaderInputTypeName = SdfValueTypeNames->Float;
    }

    if (ignoreIfUnauthored && !UsdMayaUtil::IsAuthored(shadingNodePlug)) {
        // Ignore this unauthored Maya attribute and return success.
        return true;
    }

    const bool isDestination = shadingNodePlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Are color values are all linear on the shader?
    // Do we need to re-linearize them?
    VtValue value = UsdMayaWriteUtil::GetVtValue(
        shadingNodePlug,
        shaderInputTypeName,
        /* linearizeColors = */ false);

    if (value.IsEmpty()) {
        return false;
    }

    UsdShadeInput shaderInput = shaderSchema.CreateInput(shadingNodeAttrName, shaderInputTypeName);

    // For attributes that are the destination of a connection, we create
    // the input on the shader but we do *not* author a value for it. We
    // expect its actual value to come from the source of its connection.
    // We'll leave it to the shading export to handle creating
    // the connections in USD.
    if (!isDestination) {
        shaderInput.Set(value, usdTime);
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
