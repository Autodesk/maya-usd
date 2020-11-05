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
