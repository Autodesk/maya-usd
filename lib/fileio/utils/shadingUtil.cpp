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
#include "pxr/pxr.h"
#include "shadingUtil.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"


PXR_NAMESPACE_USING_DIRECTIVE


UsdShadeInput
UsdMayaShadingUtil::CreateMaterialInputAndConnectShader(
        UsdShadeMaterial& material,
        const TfToken& materialInputName,
        const SdfValueTypeName& inputTypeName,
        UsdShadeShader& shader,
        const TfToken& shaderInputName)
{
    if (!material || !shader) {
        return UsdShadeInput();
    }

    UsdShadeInput materialInput =
        material.CreateInput(materialInputName, inputTypeName);

    UsdShadeInput shaderInput =
        shader.CreateInput(shaderInputName, inputTypeName);

    shaderInput.ConnectToSource(materialInput);

    return materialInput;
}

UsdShadeOutput
UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
        UsdShadeShader& shader,
        const TfToken& shaderOutputName,
        const SdfValueTypeName& outputTypeName,
        UsdShadeMaterial& material,
        const TfToken& materialOutputName)
{
    if (!shader || !material) {
        return UsdShadeOutput();
    }

    UsdShadeOutput shaderOutput =
        shader.CreateOutput(shaderOutputName, outputTypeName);

    UsdShadeOutput materialOutput =
        material.CreateOutput(materialOutputName, outputTypeName);

    materialOutput.ConnectToSource(shaderOutput);

    return shaderOutput;
}
