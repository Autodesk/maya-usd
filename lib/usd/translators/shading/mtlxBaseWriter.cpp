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
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
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

UsdAttribute MtlxUsd_BaseWriter::ExtractChannel(size_t channelIndex, UsdAttribute nodeOutput)
{
    // TODO: MaterialX 1.39 will remove swizzle nodes. Must use extract nodes instead. Note that
    //       in 1.38 the extract is 1) implemented using a swizzle 2) not inlined, so we will wait
    //       until 1.39 to implement that change.
    const SdfValueTypeName& sourceType = nodeOutput.GetTypeName();
    if (sourceType == SdfValueTypeNames->Float) {
        // No swizzle actually needed:
        return nodeOutput;
    }

    std::string materialXType;
    std::string channelNames;
    if (sourceType == SdfValueTypeNames->Float2) {
        materialXType = "vector2";
        channelNames = "xy";
    } else if (sourceType == SdfValueTypeNames->Float3) {
        materialXType = "vector3";
        channelNames = "xyz";
    } else if (sourceType == SdfValueTypeNames->Float4) {
        materialXType = "vector4";
        channelNames = "xyzw";
    } else if (sourceType == SdfValueTypeNames->Color3f) {
        materialXType = "color3";
        channelNames = "rgb";
    } else if (sourceType == SdfValueTypeNames->Color4f) {
        materialXType = "color4";
        channelNames = "rgba";
    } else {
        return {};
    }

    if (channelIndex >= channelNames.size()) {
        return {};
    }

    const std::string channel(1, channelNames[channelIndex]);

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    SdfPath           nodegraphPath = nodegraphSchema.GetPath();

    SdfPath outputPath = nodeOutput.GetPath().GetParentPath();

    TfToken        swizzleName(TfStringPrintf(
        "%s_%s_%s",
        _tokens->SwizzlePrefix.GetText(),
        outputPath.GetName().c_str(),
        channel.c_str()));
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
        .Set(channel, UsdTimeCode::Default());

    swizzleOutput = swizzleSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float);

    TfToken swizzleID(TfStringPrintf("ND_swizzle_%s_float", materialXType.c_str()));

    swizzleSchema.CreateIdAttr(VtValue(swizzleID));

    return swizzleOutput;
}

UsdAttribute
MtlxUsd_BaseWriter::AddConstructor(UsdAttribute nodeInput, size_t channelIndex, MPlug inputPlug)
{
    const SdfValueTypeName& sourceType = nodeInput.GetTypeName();
    if (sourceType == SdfValueTypeNames->Float) {
        // No constructor actually needed:
        return nodeInput;
    }

    std::string materialXType;
    size_t      maxChannels = 0;
    if (sourceType == SdfValueTypeNames->Float2) {
        materialXType = "vector2";
        maxChannels = 2;
    } else if (sourceType == SdfValueTypeNames->Float3) {
        materialXType = "vector3";
        maxChannels = 3;
    } else if (sourceType == SdfValueTypeNames->Float4) {
        materialXType = "vector4";
        maxChannels = 4;
    } else if (sourceType == SdfValueTypeNames->Color3f) {
        materialXType = "color3";
        maxChannels = 3;
    } else if (sourceType == SdfValueTypeNames->Color4f) {
        materialXType = "color4";
        maxChannels = 4;
    } else {
        return {};
    }

    if (channelIndex >= maxChannels) {
        // Can happen for monochrome file texture with an RGB default color plugged at the
        // subcomponent level. Extremely rare.
        channelIndex = maxChannels - 1;
    }

    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    SdfPath           nodegraphPath = nodegraphSchema.GetPath();

    SdfPath outputPath = nodeInput.GetPath().GetParentPath();

    TfToken        ctorName(TfStringPrintf(
        "%s_%s_%s",
        TrMtlxTokens->ConstructorPrefix.GetText(),
        outputPath.GetName().c_str(),
        nodeInput.GetBaseName().GetText()));
    const SdfPath  ctorPath = nodegraphPath.AppendChild(ctorName);
    UsdShadeShader ctorSchema = UsdShadeShader::Define(GetUsdStage(), ctorPath);

    TfToken inputName(TfStringPrintf("in%zu", channelIndex + 1));

    UsdAttribute ctorInput = ctorSchema.GetInput(inputName);
    if (ctorInput) {
        // Reusing existing input:
        ctorInput.Clear();
        return ctorInput;
    }

    // Here we must initialize the CTOR from the provided plug:
    if (outputPath == nodegraphPath) {
        UsdShadeOutput(nodeInput).ConnectToSource(
            ctorSchema.CreateOutput(TrMtlxTokens->out, sourceType));
    } else {
        UsdShadeInput(nodeInput).ConnectToSource(
            ctorSchema.CreateOutput(TrMtlxTokens->out, sourceType));
    }

    for (size_t i = 0; i < maxChannels; ++i) {
        TfToken inputName(TfStringPrintf("in%zu", i + 1));

        MPlug childPlug = inputPlug.child(static_cast<unsigned int>(i));

        UsdAttribute childAttr = ctorSchema.CreateInput(inputName, SdfValueTypeNames->Float);

        if (i == channelIndex) {
            ctorInput = childAttr;
        } else {
            float channelValue = childPlug.asFloat();
            childAttr.Set(channelValue, UsdTimeCode::Default());
        }
    }

    TfToken ctorID(TfStringPrintf("ND_combine%zu_%s", maxChannels, materialXType.c_str()));

    ctorSchema.CreateIdAttr(VtValue(ctorID));

    return ctorInput;
}

UsdAttribute
MtlxUsd_BaseWriter::AddConversion(const SdfValueTypeName& destType, UsdAttribute nodeOutput)
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
    //
    // An empty swizzle signals the use a "convert" node.
    //
    // TODO: MaterialX 1.39 will no longer support swizzles, requiring use of two nodes (separate +
    //       combine) to do the work of one.
    // NOTE: in 1.38 the separate node is 1) implemented using a swizzle 2) not inlined, so we will
    //       wait until 1.39 to implement that change.
    // This requires making sure that both USD and MaterialX work correctly with multi-output nodes.
    //   Requires at least the upcoming USD 22.05 and MaterialX 1.38.4:
    //     https://github.com/PixarAnimationStudios/USD/commit/9bcc8baa
    //     https://github.com/AcademySoftwareFoundation/MaterialX/pull/853
    //
    typedef std::vector<_SwizzleData> _SwizzleMap;
    static const _SwizzleMap          _swizzleMap = _SwizzleMap {
        { SdfValueTypeNames->Float, SdfValueTypeNames->Float2, "float", "vector2", "" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Color3f, "float", "color3", "" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Color4f, "float", "color4", "xxx1" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Float3, "float", "vector3", "" },
        { SdfValueTypeNames->Float, SdfValueTypeNames->Float4, "float", "vector4", "" },

        { SdfValueTypeNames->Float2, SdfValueTypeNames->Float, "vector2", "float", "x" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Color3f, "vector2", "color3", "xyy" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Color4f, "vector2", "color4", "xyy1" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Float3, "vector2", "vector3", "xyy" },
        { SdfValueTypeNames->Float2, SdfValueTypeNames->Float4, "vector2", "vector4", "xyyy" },

        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float, "color3", "float", "r" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float2, "color3", "vector2", "rg" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Color4f, "color3", "color4", "" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float3, "color3", "vector3", "" },
        { SdfValueTypeNames->Color3f, SdfValueTypeNames->Float4, "color3", "vector4", "rgb1" },

        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float, "color4", "float", "r" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float2, "color4", "vector2", "rg" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Color3f, "color4", "color3", "" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float3, "color4", "vector3", "rgb" },
        { SdfValueTypeNames->Color4f, SdfValueTypeNames->Float4, "color4", "vector4", "" },

        { SdfValueTypeNames->Float3, SdfValueTypeNames->Float, "vector3", "float", "x" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Float2, "vector3", "vector2", "" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Color3f, "vector3", "color3", "" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Color4f, "vector3", "color4", "xyz1" },
        { SdfValueTypeNames->Float3, SdfValueTypeNames->Float4, "vector3", "vector4", "" },

        { SdfValueTypeNames->Float4, SdfValueTypeNames->Float, "vector4", "float", "x" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Float2, "vector4", "vector2", "xy" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Color3f, "vector4", "color3", "xyz" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Color4f, "vector4", "color4", "" },
        { SdfValueTypeNames->Float4, SdfValueTypeNames->Float3, "vector4", "vector3", "" },
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

    TfToken swizzleName;
    if (channels->empty()) {
        swizzleName = TfToken(TfStringPrintf(
            "%s_%s", _tokens->ConverterPrefix.GetText(), outputPath.GetName().c_str()));
    } else {
        swizzleName = TfToken(TfStringPrintf(
            "%s_%s_%s",
            _tokens->SwizzlePrefix.GetText(),
            outputPath.GetName().c_str(),
            channels->c_str()));
    }
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

    if (!channels->empty()) {
        swizzleSchema.CreateInput(TrMtlxTokens->channels, SdfValueTypeNames->String)
            .Set(*channels, UsdTimeCode::Default());
    }

    swizzleOutput = swizzleSchema.CreateOutput(TrMtlxTokens->out, destType);

    TfToken swizzleID(TfStringPrintf(
        (channels->empty() ? "ND_convert_%s_%s" : "ND_swizzle_%s_%s"),
        srcType->c_str(),
        dstType->c_str()));

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

    return ExtractChannel(0, luminanceOutput);
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

UsdAttribute MtlxUsd_BaseWriter::PreserveNodegraphBoundaries(UsdAttribute input)
{
    UsdShadeNodeGraph nodeGraph(GetNodeGraph());
    SdfPath           nodeGraphPath(nodeGraph.GetPath().GetParentPath());

    SdfPath inputPath = input.GetPrimPath().GetParentPath();

    // If both are at the same level, we need to create a nodegraph boundary:
    if (nodeGraphPath == inputPath) {
        UsdShadeOutput ngOutput = nodeGraph.CreateOutput(input.GetBaseName(), input.GetTypeName());
        UsdShadeInput(input).ConnectToSource(ngOutput);

        return ngOutput;
    }

    return input;
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
