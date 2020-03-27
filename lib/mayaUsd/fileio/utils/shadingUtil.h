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
#ifndef PXRUSDMAYA_SHADING_UTIL_H
#define PXRUSDMAYA_SHADING_UTIL_H

/// \file usdMaya/shadingUtil.h

#include "pxr/pxr.h"
#include "../../base/api.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"


PXR_NAMESPACE_OPEN_SCOPE


namespace UsdMayaShadingUtil
{


/// Create an input on the given material and shader and create a connection
/// between them.
///
/// This creates an interface input on \p material with name
/// \p materialInputName and type \p inputTypeName. An input named
/// \p shaderInputName is created on \p shader, also with type
/// \p inputTypeName. A connection is then created between the two such that
/// the input on the material drives the input on the shader.
///
/// Returns the material input on success, or an invalid UsdShadeInput
/// otherwise.
MAYAUSD_CORE_PUBLIC
UsdShadeInput CreateMaterialInputAndConnectShader(
        UsdShadeMaterial& material,
        const TfToken& materialInputName,
        const SdfValueTypeName& inputTypeName,
        UsdShadeShader& shader,
        const TfToken& shaderInputName);

/// Create an output on the given shader and material and create a connection
/// between them.
///
/// This creates an output on \p shader with name \p shaderOutputName and type
/// \p inputTypeName. An output named \p materialOutputName is created on
/// \p material, also with type \p outputTypeName. A connection is then created
/// between the two such that the output of the shader propagates to the output
/// of the material.
///
/// Returns the shader output on success, or an invalid UsdShadeOutput
/// otherwise.
MAYAUSD_CORE_PUBLIC
UsdShadeOutput CreateShaderOutputAndConnectMaterial(
        UsdShadeShader& shader,
        const TfToken& shaderOutputName,
        const SdfValueTypeName& outputTypeName,
        UsdShadeMaterial& material,
        const TfToken& materialOutputName);


} // namespace UsdMayaShadingUtil


PXR_NAMESPACE_CLOSE_SCOPE


#endif
