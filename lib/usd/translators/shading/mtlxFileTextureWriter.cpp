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

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/converter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdUtils/pipeline.h>
#include <pxr/usdImaging/usdImaging/textureUtils.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <ghc/filesystem.hpp>

#include <regex>

using namespace MAYAUSD_NS_DEF;

PXR_NAMESPACE_OPEN_SCOPE

class MtlxUsd_FileWriter : public MtlxUsd_BaseWriter
{
public:
    MtlxUsd_FileWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    void PostExport() override;

    UsdAttribute GetShadingAttributeForMayaAttrName(
        const TfToken&          mayaAttrName,
        const SdfValueTypeName& typeName) override;

    bool AuthorSplitColor4InputFromShadingNodeAttr(
        const MFnDependencyNode& depNodeFn,
        const TfToken&           coorAttrName,
        const TfToken&           alphaAttrName,
        UsdShadeShader&          shaderSchema,
        const UsdTimeCode        usdTime);

private:
    int              _numChannels = 4;
    UsdPrim          _fileTexturePrim;
    SdfValueTypeName _outputDataType;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(file, MtlxUsd_FileWriter);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Shared primvar writer (when no place2dTexture found):
    ((PrimvarReaderShaderName, "shared_MayaGeomPropValue"))
    ((fileTextureSuffix, "_MayafileTexture"))
);
// clang-format on

MtlxUsd_FileWriter::MtlxUsd_FileWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : MtlxUsd_BaseWriter(depNodeFn, usdPath, jobCtx)
{
    // Everything must be added in the material node graph:
    UsdShadeNodeGraph nodegraphSchema(GetNodeGraph());
    if (!TF_VERIFY(
            nodegraphSchema,
            "Could not get UsdShadeNodeGraph at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    SdfPath nodegraphPath = nodegraphSchema.GetPath();
    SdfPath texPath
        = nodegraphPath.AppendChild(TfToken(UsdMayaUtil::SanitizeName(depNodeFn.name().asChar())));

    // Create a image shader as the "primary" shader for this writer.
    UsdShadeShader texSchema = UsdShadeShader::Define(GetUsdStage(), texPath);
    if (!TF_VERIFY(
            texSchema, "Could not define UsdShadeShader at path '%s'\n", texPath.GetText())) {
        return;
    }

    _usdPrim = texSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            texPath.GetText())) {
        return;
    }

    // We need to know how many channels the texture has:
    MPlug       texNamePlug = depNodeFn.findPlug("fileTextureName");
    std::string filename(texNamePlug.asString().asChar());

    // Not resolving UDIM tags. We want to actually open one of these files:
    UsdMayaShadingUtil::ResolveUsdTextureFileName(
        filename, _GetExportArgs().GetResolvedFileName(), false);

    _numChannels = UsdMayaShadingUtil::GetNumberOfChannels(filename);
    switch (_numChannels) {
    case 1:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_float));
        _outputDataType = SdfValueTypeNames->Float;
        break;
    case 2:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_vector2));
        _outputDataType = SdfValueTypeNames->Float2;
        break;
    case 3:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_color3));
        _outputDataType = SdfValueTypeNames->Color3f;
        break;
    case 4:
        texSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_image_color4));
        _outputDataType = SdfValueTypeNames->Color4f;
        break;
    default: TF_CODING_ERROR("Unsupported format"); return;
    }
    texSchema.CreateInput(TrMtlxTokens->filtertype, SdfValueTypeNames->String).Set("cubic");
    UsdShadeOutput colorOutput = texSchema.CreateOutput(TrMtlxTokens->out, _outputDataType);

    // The color correction section of a fileTexture node exists on a separate node that post
    // processes the values of the MaterialX image node, which is kept visible to allow DCC to
    // properly detect textures that need to be loaded.
    MString fileTextureName = depNodeFn.name();
    fileTextureName += _tokens->fileTextureSuffix.GetText();
    SdfPath fileTexturePath
        = nodegraphPath.AppendChild(TfToken(UsdMayaUtil::SanitizeName(fileTextureName.asChar())));

    UsdShadeShader fileTextureSchema = UsdShadeShader::Define(GetUsdStage(), fileTexturePath);
    _fileTexturePrim = fileTextureSchema.GetPrim();

    if (_outputDataType == SdfValueTypeNames->Float) {
        fileTextureSchema.CreateIdAttr(VtValue(TrMtlxTokens->MayaND_fileTexture_float));
    } else if (_outputDataType == SdfValueTypeNames->Color3f) {
        fileTextureSchema.CreateIdAttr(VtValue(TrMtlxTokens->MayaND_fileTexture_color3));
    } else if (_outputDataType == SdfValueTypeNames->Color4f) {
        fileTextureSchema.CreateIdAttr(VtValue(TrMtlxTokens->MayaND_fileTexture_color4));
    } else if (_outputDataType == SdfValueTypeNames->Float2) {
        fileTextureSchema.CreateIdAttr(VtValue(TrMtlxTokens->MayaND_fileTexture_vector2));
    } else if (_outputDataType == SdfValueTypeNames->Float3) {
        fileTextureSchema.CreateIdAttr(VtValue(TrMtlxTokens->MayaND_fileTexture_vector3));
    } else if (_outputDataType == SdfValueTypeNames->Float4) {
        fileTextureSchema.CreateIdAttr(VtValue(TrMtlxTokens->MayaND_fileTexture_vector4));
    }

    fileTextureSchema.CreateInput(TrMayaTokens->inColor, _outputDataType)
        .ConnectToSource(colorOutput);
    fileTextureSchema.CreateInput(TrMayaTokens->uvCoord, _outputDataType);
    fileTextureSchema.CreateOutput(TrMayaTokens->outColor, _outputDataType);

    const MPlug uvCoordPlug = depNodeFn.findPlug(
        TrMayaTokens->uvCoord.GetText(),
        /* wantNetworkedPlug = */ true);
    if (!uvCoordPlug.isNull() && uvCoordPlug.isDestination()) {
        // We have something driving the UV coordinates
        return;
    }

    // No place2dtexture connected: create a geompropvalue reader
    SdfPath primvarReaderPath
        = GetNodeGraph().GetPath().AppendChild(_tokens->PrimvarReaderShaderName);
    UsdShadeOutput primvarReaderOutput;

    if (!GetUsdStage()->GetPrimAtPath(primvarReaderPath)) {
        UsdShadeShader primvarReaderSchema
            = UsdShadeShader::Define(GetUsdStage(), primvarReaderPath);
        primvarReaderSchema.CreateIdAttr(VtValue(TrMtlxTokens->ND_geompropvalue_vector2));
        UsdShadeInput varnameInput
            = primvarReaderSchema.CreateInput(TrMtlxTokens->geomprop, SdfValueTypeNames->String);

        TfToken inputName(
            TfStringPrintf("%s:%s", depNodeFn.name().asChar(), TrMtlxTokens->varnameStr.GetText()));

        // We expose the primvar reader varnameStr attribute to the material to allow
        // easy specialization based on UV mappings to geometries:
        UsdPrim          materialPrim = primvarReaderSchema.GetPrim().GetParent();
        UsdShadeMaterial materialSchema(materialPrim);
        while (!materialSchema && materialPrim) {
            UsdShadeNodeGraph intermediateNodeGraph(materialPrim);
            if (intermediateNodeGraph) {
                UsdShadeInput intermediateInput
                    = intermediateNodeGraph.CreateInput(inputName, SdfValueTypeNames->String);
                varnameInput.ConnectToSource(intermediateInput);
                varnameInput = intermediateInput;
            }

            materialPrim = materialPrim.GetParent();
            materialSchema = UsdShadeMaterial(materialPrim);
        }

        if (materialSchema) {
            UsdShadeInput materialInput
                = materialSchema.CreateInput(inputName, SdfValueTypeNames->String);
            materialInput.Set(UsdUtilsGetPrimaryUVSetName().GetString());
            varnameInput.ConnectToSource(materialInput);
        } else {
            varnameInput.Set(UsdUtilsGetPrimaryUVSetName());
        }

        primvarReaderOutput
            = primvarReaderSchema.CreateOutput(TrMtlxTokens->out, SdfValueTypeNames->Float2);
    } else {
        // Re-using an existing primvar reader:
        UsdShadeShader primvarReaderShaderSchema(GetUsdStage()->GetPrimAtPath(primvarReaderPath));
        primvarReaderOutput = primvarReaderShaderSchema.GetOutput(TrMtlxTokens->out);
    }
    // Connect the output of the primvar reader to the texture coordinate
    // input of the UV texture.
    texSchema.CreateInput(TrMtlxTokens->texcoord, SdfValueTypeNames->Float2)
        .ConnectToSource(primvarReaderOutput);
}

/* virtual */
void MtlxUsd_FileWriter::Write(const UsdTimeCode& usdTime)
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

    // File
    const MPlug fileTextureNamePlug = depNodeFn.findPlug(
        TrMayaTokens->fileTextureName.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    std::string fileTextureName(fileTextureNamePlug.asString(&status).asChar());
    if (status != MS::kSuccess) {
        return;
    }

    const MPlug tilingAttr
        = depNodeFn.findPlug(TrMayaTokens->uvTilingMode.GetText(), true, &status);
    const bool isUDIM = (status == MS::kSuccess && tilingAttr.asInt() == 3);

    UsdMayaShadingUtil::ResolveUsdTextureFileName(
        fileTextureName, _GetExportArgs().GetResolvedFileName(), isUDIM);

    UsdShadeInput fileInput
        = shaderSchema.CreateInput(TrMtlxTokens->file, SdfValueTypeNames->Asset);
    fileInput.Set(SdfAssetPath(fileTextureName.c_str()), usdTime);

    // Default Color (which needs to have a matching number of channels)
    const MPlug defaultColorPlug = depNodeFn.findPlug(
        TrMayaTokens->defaultColor.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    // Adding inputs to the fileTexture post procesor node:
    auto fileTextureShader = UsdShadeShader(_fileTexturePrim);
    if (!fileTextureShader) {
        return;
    }

    if (!defaultColorPlug.isDestination()) {
        UsdShadeInput defaultValueInput
            = fileTextureShader.CreateInput(TrMayaTokens->defaultColor, _outputDataType);

        switch (_numChannels) {
        case 1: {
            float fallback = 0.0f;
            defaultColorPlug.child(0).getValue(fallback);
            defaultValueInput.Set(fallback, usdTime);

        } break;
        case 2: {
            GfVec2f fallback(0.0f, 0.0f);
            for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
                defaultColorPlug.child(i).getValue(fallback[i]);
            }
            defaultValueInput.Set(fallback, usdTime);
        } break;
        case 3: {
            GfVec3f fallback(0.0f, 0.0f, 0.0f);
            for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
                defaultColorPlug.child(i).getValue(fallback[i]);
            }
            defaultValueInput.Set(fallback, usdTime);
        } break;
        case 4: {
            GfVec4f fallback(0.0f, 0.0f, 0.0f, 1.0f);
            for (size_t i = 0u; i < 3; ++i) { // defaultColor is a 3Float
                defaultColorPlug.child(i).getValue(fallback[i]);
            }
            defaultValueInput.Set(fallback, usdTime);
        } break;
        default: TF_CODING_ERROR("Unsupported format for default"); return;
        }
    }

    // Source color space:
    MPlug colorSpacePlug = depNodeFn.findPlug(TrMayaTokens->colorSpace.GetText(), true, &status);
    if (status == MS::kSuccess) {
        MString colorRuleCmd;
        colorRuleCmd.format(
            "colorManagementFileRules -evaluate \"^1s\";", fileTextureNamePlug.asString());
        const MString colorSpaceByRule(MGlobal::executeCommandStringResult(colorRuleCmd));
        const MString colorSpace(colorSpacePlug.asString(&status));
        if (status == MS::kSuccess && colorSpace != colorSpaceByRule) {
            fileInput.GetAttr().SetColorSpace(TfToken(colorSpace.asChar()));
            // Also add it on the post-processor node for UsdMaya internal use:
            fileTextureShader.CreateInput(TrMayaTokens->colorSpace, SdfValueTypeNames->String)
                .Set(std::string(colorSpace.asChar()));
        }
    }

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn, TrMayaTokens->invert, fileTextureShader, usdTime);
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn, TrMayaTokens->exposure, fileTextureShader, usdTime);
    AuthorSplitColor4InputFromShadingNodeAttr(
        depNodeFn, TrMayaTokens->colorGain, TrMayaTokens->alphaGain, fileTextureShader, usdTime);
    AuthorSplitColor4InputFromShadingNodeAttr(
        depNodeFn,
        TrMayaTokens->colorOffset,
        TrMayaTokens->alphaOffset,
        fileTextureShader,
        usdTime);

    // Usdview has issues unless the u and v addressmodes are explicitly set
    shaderSchema.CreateInput(TrMtlxTokens->uaddressmode, SdfValueTypeNames->String)
        .Set(TrMtlxTokens->periodic.GetString());
    shaderSchema.CreateInput(TrMtlxTokens->vaddressmode, SdfValueTypeNames->String)
        .Set(TrMtlxTokens->periodic.GetString());

    // Filter type:
    //
    // A bit arbitrary. We will go with:
    //       Off(0) -> closest (rendered as kMinMagMipPoint in VP2)
    //    MipMap(1) -> linear (MaterialX default rendered as kMinMagMipLinear in VP2)
    // All other(-) -> cubic (rendered as kAnisotropic in VP2)
    MPlug filterTypePlug = depNodeFn.findPlug(TrMayaTokens->filterType.GetText(), true, &status);
    if (status == MS::kSuccess) {
        int filterTypeVal = filterTypePlug.asInt();
        if (filterTypeVal == 0) {
            shaderSchema.CreateInput(TrMtlxTokens->filtertype, SdfValueTypeNames->String)
                .Set(TrMtlxTokens->closest.GetString());
        } else if (filterTypeVal > 1) {
            shaderSchema.CreateInput(TrMtlxTokens->filtertype, SdfValueTypeNames->String)
                .Set(TrMtlxTokens->cubic.GetString());
        }
    }
}

bool MtlxUsd_FileWriter::AuthorSplitColor4InputFromShadingNodeAttr(
    const MFnDependencyNode& depNodeFn,
    const TfToken&           colorAttrName,
    const TfToken&           alphaAttrName,
    UsdShadeShader&          shaderSchema,
    const UsdTimeCode        usdTime)
{
    MStatus status;

    MPlug colorPlug = depNodeFn.findPlug(
        depNodeFn.attribute(colorAttrName.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return false;
    }

    MPlug alphaPlug = depNodeFn.findPlug(
        depNodeFn.attribute(alphaAttrName.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (!UsdMayaUtil::IsAuthored(colorPlug) && !UsdMayaUtil::IsAuthored(alphaPlug)) {
        // Ignore this unauthored Maya attribute and return success.
        return true;
    }

    const bool isDestination = colorPlug.isDestination(&status) || alphaPlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (isDestination) {
        // Destination inputs will be explicitly created on demand.
        return true;
    }

    // Here we need to consider the number of channels we export:
    UsdShadeInput shaderInput = shaderSchema.CreateInput(colorAttrName, _outputDataType);
    switch (_numChannels) {
    case 1: {
        float value = 0.0f;
        colorPlug.child(0).getValue(value);
        shaderInput.Set(value, usdTime);

    } break;
    case 2: {
        GfVec2f value(0.0f, 0.0f);
        for (size_t i = 0u; i < GfVec2f::dimension; ++i) {
            colorPlug.child(i).getValue(value[i]);
        }
        shaderInput.Set(value, usdTime);
    } break;
    case 3: {
        GfVec3f value(0.0f, 0.0f, 0.0f);
        for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
            colorPlug.child(i).getValue(value[i]);
        }
        shaderInput.Set(value, usdTime);
    } break;
    case 4: {
        GfVec4f value(0.0f, 0.0f, 0.0f, 1.0f);
        for (size_t i = 0u; i < 3; ++i) { // defaultColor is a 3Float
            colorPlug.child(i).getValue(value[i]);
        }
        alphaPlug.getValue(value[3]);
        shaderInput.Set(value, usdTime);
    } break;
    default: TF_CODING_ERROR("Unsupported format for default"); return false;
    }

    return true;
}

/* virtual */
UsdAttribute MtlxUsd_FileWriter::GetShadingAttributeForMayaAttrName(
    const TfToken&          mayaAttrName,
    const SdfValueTypeName& typeName)
{
    UsdShadeShader nodeSchema = UsdShadeShader(_fileTexturePrim);
    TfToken        outName = TrMayaTokens->outColor;
    if (!nodeSchema) {
        return UsdAttribute();
    }

    if (mayaAttrName == TrMayaTokens->outColor) {
        UsdShadeOutput mainOutput = nodeSchema.GetOutput(outName);

        if (mainOutput.GetTypeName() == typeName) {
            return mainOutput;
        }

        // If types differ, then we need to handle all possible conversions and
        // channel swizzling.
        return AddConversion(typeName, mainOutput);
    }

    // Starting here, we handle subcomponent requests:

    if (_numChannels == 2) {
        // This will be ND_image_vector2, so requires xyz swizzles:
        if (mayaAttrName == TrMayaTokens->outColorR || mayaAttrName == TrMayaTokens->outColorG
            || mayaAttrName == TrMayaTokens->outColorB) {
            return ExtractChannel(0, nodeSchema.GetOutput(outName));
        }
        if (mayaAttrName == TrMayaTokens->outAlpha) {
            return ExtractChannel(1, nodeSchema.GetOutput(outName));
        }
    }

    if (mayaAttrName == TrMayaTokens->outColorR) {
        return ExtractChannel(0, nodeSchema.GetOutput(outName));
    }

    if (mayaAttrName == TrMayaTokens->outColorG) {
        return ExtractChannel(1, nodeSchema.GetOutput(outName));
    }

    if (mayaAttrName == TrMayaTokens->outColorB) {
        return ExtractChannel(2, nodeSchema.GetOutput(outName));
    }

    if (mayaAttrName == TrMayaTokens->outAlpha) {
        bool                    alphaIsLuminance = false;
        MStatus                 status;
        const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
        if (status == MS::kSuccess) {
            MPlug plug = depNodeFn.findPlug(TrMayaTokens->alphaIsLuminance.GetText());
            plug.getValue(alphaIsLuminance);
        }

        if (alphaIsLuminance || _numChannels == 3) {
            return AddLuminance(_numChannels, nodeSchema.GetOutput(outName));
        } else {
            return ExtractChannel(3, nodeSchema.GetOutput(outName));
        }
    }

    if (mayaAttrName == TrMayaTokens->uvCoord) {
        return UsdShadeShader(_usdPrim).CreateInput(
            TrMtlxTokens->texcoord, SdfValueTypeNames->Float2);
    }

    if (mayaAttrName == TrMayaTokens->defaultColor) {
        return UsdShadeShader(_fileTexturePrim)
            .CreateInput(TrMayaTokens->defaultColor, _outputDataType);
    } else if (mayaAttrName == TrMayaTokens->colorGain) {
        // TODO: Merge colorGain and alphaGain in a color4/vector4 connection.
        return UsdShadeShader(_fileTexturePrim)
            .CreateInput(TrMayaTokens->colorGain, _outputDataType);
    } else if (mayaAttrName == TrMayaTokens->colorOffset) {
        // TODO: Merge colorOffset and alphaOffset in a color4/vector4 connection.
        return UsdShadeShader(_fileTexturePrim)
            .CreateInput(TrMayaTokens->colorOffset, _outputDataType);
    }

    // We did not find the attribute directly, but we might be dealing with a subcomponent
    // connection on a compound attribute:
    MStatus                 status;
    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);

    MPlug childPlug = depNodeFn.findPlug(mayaAttrName.GetText(), &status);
    if (!status || childPlug.isNull() || !childPlug.isChild()) {
        return {};
    }

    MPlug parentPlug = childPlug.parent();

    // We need the long name of the attribute:
    const TfToken parentAttrName(
        parentPlug.partialName(false, false, false, false, false, true).asChar());

    if (parentAttrName != TrMayaTokens->uvCoord && parentAttrName != TrMayaTokens->defaultColor
        && parentAttrName != TrMayaTokens->colorGain
        && parentAttrName != TrMayaTokens->colorOffset) {
        return {};
    }

    unsigned int       childIndex = 0;
    const unsigned int numChildren = parentPlug.numChildren();
    for (; childIndex < numChildren; ++childIndex) {
        if (childPlug.attribute() == parentPlug.child(childIndex).attribute()) {
            break;
        }
    }

    UsdShadeInput input
        = UsdShadeShader(_fileTexturePrim)
              .CreateInput(
                  parentAttrName,
                  (parentAttrName == TrMayaTokens->uvCoord ? SdfValueTypeNames->Float2
                                                           : _outputDataType));
    if (input) {
        return AddConstructor(input, static_cast<size_t>(childIndex), parentPlug);
    }

    return {};
}

void MtlxUsd_FileWriter::PostExport()
{
    // Connect the fileTexture node to the UVs of the image (generated by place2dTexture)
    UsdShadeShader fileTextureSchema(_fileTexturePrim);
    if (!fileTextureSchema) {
        return;
    }
    UsdShadeShader nodeSchema(_usdPrim);
    if (!nodeSchema) {
        return;
    }
    UsdShadeInput texcoordInput = nodeSchema.GetInput(TrMtlxTokens->texcoord);
    if (!texcoordInput) {
        return;
    }

    UsdShadeConnectableAPI sourceNode;
    TfToken                sourceOutputName;
    UsdShadeAttributeType  sourceType;

    if (!UsdShadeConnectableAPI::GetConnectedSource(
            texcoordInput, &sourceNode, &sourceOutputName, &sourceType)) {
        return;
    }

    fileTextureSchema.CreateInput(TrMayaTokens->uvCoord, SdfValueTypeNames->Float2)
        .ConnectToSource(sourceNode.GetOutput(sourceOutputName));
}

PXR_NAMESPACE_CLOSE_SCOPE
