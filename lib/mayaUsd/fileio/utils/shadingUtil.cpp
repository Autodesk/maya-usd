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
#include "shadingUtil.h"

#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#if PXR_VERSION >= 2102
#include <pxr/imaging/hio/image.h>
#else
#include <pxr/imaging/glf/image.h>
#endif

#include <maya/MPlug.h>
#include <maya/MString.h>

#include <ghc/filesystem.hpp>

#include <regex>
#include <string>
#include <system_error>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((UDIMTag, "<UDIM>"))

    (place2dTexture)
    (coverage)
    (translateFrame)
    (rotateFrame) 
    (mirrorU)
    (mirrorV)
    (stagger)
    (wrapU)
    (wrapV)
    (repeatUV)
    (offset)
    (rotateUV)
    (noiseUV)
    (vertexUvOne)
    (vertexUvTwo)
    (vertexUvThree)
    (vertexCameraOne)
    (outUvFilterSize)
    (uvFilterSize)
    (outUV)
    (uvCoord)
);
// clang-format on

static const TfTokenVector _Place2dTextureConnections = {
    _tokens->coverage,    _tokens->translateFrame, _tokens->rotateFrame,   _tokens->mirrorU,
    _tokens->mirrorV,     _tokens->stagger,        _tokens->wrapU,         _tokens->wrapV,
    _tokens->repeatUV,    _tokens->offset,         _tokens->rotateUV,      _tokens->noiseUV,
    _tokens->vertexUvOne, _tokens->vertexUvTwo,    _tokens->vertexUvThree, _tokens->vertexCameraOne
};

} // namespace

std::string
UsdMayaShadingUtil::GetStandardAttrName(const MPlug& attrPlug, bool allowMultiElementArrays)
{
    if (!attrPlug.isElement()) {
        const MString mayaPlugName = attrPlug.partialName(false, false, false, false, false, true);
        return mayaPlugName.asChar();
    }

    const MString mayaPlugName
        = attrPlug.array().partialName(false, false, false, false, false, true);
    const unsigned int logicalIndex = attrPlug.logicalIndex();

    if (allowMultiElementArrays) {
        return TfStringPrintf("%s_%d", mayaPlugName.asChar(), logicalIndex);
    } else if (logicalIndex == 0) {
        return mayaPlugName.asChar();
    }

    return std::string();
}

UsdShadeInput UsdMayaShadingUtil::CreateMaterialInputAndConnectShader(
    UsdShadeMaterial&       material,
    const TfToken&          materialInputName,
    const SdfValueTypeName& inputTypeName,
    UsdShadeShader&         shader,
    const TfToken&          shaderInputName)
{
    if (!material || !shader) {
        return UsdShadeInput();
    }

    UsdShadeInput materialInput = material.CreateInput(materialInputName, inputTypeName);

    UsdShadeInput shaderInput = shader.CreateInput(shaderInputName, inputTypeName);

    shaderInput.ConnectToSource(materialInput);

    return materialInput;
}

UsdShadeOutput UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
    UsdShadeShader&   shader,
    UsdShadeMaterial& material,
    const TfToken&    terminalName,
    const TfToken&    renderContext)
{
    if (!shader || !material) {
        return UsdShadeOutput();
    }

    UsdShadeOutput materialOutput;
    if (terminalName == UsdShadeTokens->surface) {
        materialOutput = material.CreateSurfaceOutput(renderContext);
    } else if (terminalName == UsdShadeTokens->volume) {
        materialOutput = material.CreateVolumeOutput(renderContext);
    } else if (terminalName == UsdShadeTokens->displacement) {
        materialOutput = material.CreateDisplacementOutput(renderContext);
    } else {
        return UsdShadeOutput();
    }

    UsdShadeOutput shaderOutput = shader.CreateOutput(terminalName, materialOutput.GetTypeName());

    UsdPrim parentPrim = shader.GetPrim().GetParent();
    if (parentPrim == material.GetPrim()) {
        materialOutput.ConnectToSource(shaderOutput);
    } else {
        // If the surface is inside a multi-material node graph, then we must create an intermediate
        // output on the NodeGraph
        UsdShadeNodeGraph parentNodeGraph(parentPrim);
        UsdShadeOutput    parentOutput
            = parentNodeGraph.CreateOutput(terminalName, materialOutput.GetTypeName());
        parentOutput.ConnectToSource(shaderOutput);
        materialOutput.ConnectToSource(parentOutput);
    }

    return shaderOutput;
}

void UsdMayaShadingUtil::ConnectPlace2dTexture(MObject textureNode, MObject uvNode)
{
    MStatus           status;
    MFnDependencyNode uvDepFn(uvNode, &status);
    MFnDependencyNode depFn(textureNode, &status);
    {
        MPlug filePlug = depFn.findPlug(_tokens->uvCoord.GetText(), true, &status);
        if (!filePlug.isDestination()) {
            MPlug uvPlug = uvDepFn.findPlug(_tokens->outUV.GetText(), true, &status);
            UsdMayaUtil::Connect(uvPlug, filePlug, false);
        }
    }
    {
        MPlug filePlug = depFn.findPlug(_tokens->uvFilterSize.GetText(), true, &status);
        if (!filePlug.isDestination()) {
            MPlug uvPlug = uvDepFn.findPlug(_tokens->outUvFilterSize.GetText(), true, &status);
            UsdMayaUtil::Connect(uvPlug, filePlug, false);
        }
    }
    for (const TfToken& uvName : _Place2dTextureConnections) {
        MPlug filePlug = depFn.findPlug(uvName.GetText(), true, &status);
        if (!filePlug.isDestination()) {
            MPlug uvPlug = uvDepFn.findPlug(uvName.GetText(), true, &status);
            UsdMayaUtil::Connect(uvPlug, filePlug, false);
        }
    }
}

MObject UsdMayaShadingUtil::CreatePlace2dTextureAndConnectTexture(MObject textureNode)
{
    MStatus status;
    MObject uvObj;
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
            _tokens->place2dTexture.GetText(),
            _tokens->place2dTexture.GetText(),
            UsdMayaShadingNodeType::Utility,
            &status,
            &uvObj))) {
        // we need to make sure those types are loaded..
        MFnDependencyNode depFn(textureNode);
        TF_RUNTIME_ERROR(
            "Could not create place2dTexture for texture '%s'.\n", depFn.name().asChar());
        return MObject();
    }

    // Connect manually (fileTexturePlacementConnect is not available in batch):
    ConnectPlace2dTexture(textureNode, uvObj);

    return uvObj;
}

namespace {
// Match UDIM pattern, from 1001 to 1999
const std::regex
    _udimRegex(".*[^\\d](1(?:[0-9][0-9][1-9]|[1-9][1-9]0|0[1-9]0|[1-9]00))(?:[^\\d].*|$)");
} // namespace

void UsdMayaShadingUtil::ResolveUsdTextureFileName(
    std::string&       fileTextureName,
    const std::string& usdFileName,
    bool               isUDIM)
{
    // WARNING: This extremely minimal attempt at making the file path relative
    //          to the USD stage is a stopgap measure intended to provide
    //          minimal interop. It will be replaced by proper use of Maya and
    //          USD asset resolvers. For package files, the exporter needs full
    //          paths.

    TfToken fileExt(TfGetExtension(usdFileName));
    if (fileExt != UsdMayaTranslatorTokens->UsdFileExtensionPackage) {
        ghc::filesystem::path usdDir(usdFileName);
        usdDir = usdDir.parent_path();
        std::error_code       ec;
        ghc::filesystem::path relativePath = ghc::filesystem::relative(fileTextureName, usdDir, ec);
        if (!ec && !relativePath.empty()) {
            fileTextureName = relativePath.generic_string();
        }
    }

    // Update filename in case of UDIM
    if (isUDIM) {
        std::smatch match;
        if (std::regex_search(fileTextureName, match, _udimRegex) && match.size() == 2) {
            fileTextureName = std::string(match[0].first, match[1].first)
                + _tokens->UDIMTag.GetString() + std::string(match[1].second, match[0].second);
        }
    }
}

#if PXR_VERSION >= 2102
int UsdMayaShadingUtil::GetNumberOfChannels(const std::string& fileTextureName)
{
    // Using Hio because the Maya texture node does not provide the information:
    HioImageSharedPtr image = HioImage::OpenForReading(fileTextureName.c_str());
    HioFormat         imageFormat = image ? image->GetFormat() : HioFormat::HioFormatUNorm8Vec4;

    // In case of unknown, use 4 channel image:
    if (imageFormat == HioFormat::HioFormatInvalid) {
        imageFormat = HioFormat::HioFormatUNorm8Vec4;
    }

    return HioGetComponentCount(imageFormat);
}
#elif PXR_VERSION >= 2011
int UsdMayaShadingUtil::GetNumberOfChannels(const std::string& fileTextureName)
{
    GlfImageSharedPtr image = GlfImage::OpenForReading(fileTextureName.c_str());

    if (!image) {
        return 4;
    }

    // Inlined HioGetComponentCount from USD 21.02:
    switch (image->GetHioFormat()) {
    case HioFormatUNorm8:
    case HioFormatSNorm8:
    case HioFormatFloat16:
    case HioFormatFloat32:
    case HioFormatDouble64:
    case HioFormatUInt16:
    case HioFormatInt16:
    case HioFormatUInt32:
    case HioFormatInt32:
    case HioFormatUNorm8srgb: return 1;
    case HioFormatUNorm8Vec2:
    case HioFormatSNorm8Vec2:
    case HioFormatFloat16Vec2:
    case HioFormatFloat32Vec2:
    case HioFormatDouble64Vec2:
    case HioFormatUInt16Vec2:
    case HioFormatInt16Vec2:
    case HioFormatUInt32Vec2:
    case HioFormatInt32Vec2:
    case HioFormatUNorm8Vec2srgb: return 2;
    case HioFormatUNorm8Vec3:
    case HioFormatSNorm8Vec3:
    case HioFormatFloat16Vec3:
    case HioFormatFloat32Vec3:
    case HioFormatDouble64Vec3:
    case HioFormatUInt16Vec3:
    case HioFormatInt16Vec3:
    case HioFormatUInt32Vec3:
    case HioFormatInt32Vec3:
    case HioFormatUNorm8Vec3srgb:
    case HioFormatBC6FloatVec3:
    case HioFormatBC6UFloatVec3: return 3;
    default: return 4;
    }
}
#else // 20.08
// Not including the OpenGL headers just for 3 constants, especially since this code
// is going to be retired soon.

// From glcorearb.h:
#define GL_RED 0x1903
#define GL_RG  0x8227
#define GL_RGB 0x1907

int UsdMayaShadingUtil::GetNumberOfChannels(const std::string& fileTextureName)
{
    // Using Glf because the Maya texture node does not provide the information:
    GlfImageSharedPtr image = GlfImage::OpenForReading(fileTextureName.c_str());

    if (!image) {
        return 4;
    }

    switch (image->GetFormat()) {
    case GL_RED: return 1;
    case GL_RG: return 2;
    case GL_RGB: return 3;
    default: return 4;
    }
}
#endif
