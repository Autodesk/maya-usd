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

#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>

#include <maya/MPlug.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

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

    materialOutput.ConnectToSource(shaderOutput);

    return shaderOutput;
}

MObject UsdMayaShadingUtil::CreatePlace2dTextureAndConnectTexture(MObject textureNode)
{
    MStatus           status;
    MObject           uvObj;
    MFnDependencyNode uvDepFn;
    MFnDependencyNode depFn(textureNode);
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
              _tokens->place2dTexture.GetText(),
              _tokens->place2dTexture.GetText(),
              UsdMayaShadingNodeType::Utility,
              &status,
              &uvObj)
          && uvDepFn.setObject(uvObj))) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
            "Could not create place2dTexture for texture '%s'.\n", depFn.name().asChar());
        return MObject();
    }

    // Connect manually (fileTexturePlacementConnect is not available in batch):
    {
        MPlug uvPlug = uvDepFn.findPlug(_tokens->outUV.GetText(), true, &status);
        MPlug filePlug = depFn.findPlug(_tokens->uvCoord.GetText(), true, &status);
        UsdMayaUtil::Connect(uvPlug, filePlug, false);
    }
    {
        MPlug uvPlug = uvDepFn.findPlug(_tokens->outUvFilterSize.GetText(), true, &status);
        MPlug filePlug = depFn.findPlug(_tokens->uvFilterSize.GetText(), true, &status);
        UsdMayaUtil::Connect(uvPlug, filePlug, false);
    }
    MString connectCmd;
    for (const TfToken& uvName : _Place2dTextureConnections) {
        MPlug uvPlug = uvDepFn.findPlug(uvName.GetText(), true, &status);
        MPlug filePlug = depFn.findPlug(uvName.GetText(), true, &status);
        UsdMayaUtil::Connect(uvPlug, filePlug, false);
    }

    return uvObj;
}
