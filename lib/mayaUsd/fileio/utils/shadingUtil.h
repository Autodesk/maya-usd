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

/// \file

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>

#include <maya/MPlug.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdMayaShadingUtil {
/// Get a "standard" USD attribute name for \p attrPlug.
///
/// If \p attrPlug is not an element in a Maya array attribute, then its name
/// is simply returned.
///
/// If \p attrPlug is an element in an array and if \p allowMultiElementArrays
/// is true, this will return a name of the form "<attrName>_<index>". If
/// \p allowMultiElementArrays is false, this will return <attrName> if it's
/// the 0-th logical element. Otherwise it will return an empty string.
MAYAUSD_CORE_PUBLIC
std::string GetStandardAttrName(const MPlug& attrPlug, bool allowMultiElementArrays);

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
    UsdShadeMaterial&       material,
    const TfToken&          materialInputName,
    const SdfValueTypeName& inputTypeName,
    UsdShadeShader&         shader,
    const TfToken&          shaderInputName);

/// Create an output on the given shader and material and create a connection
/// between them.
///
/// This creates an output on \p shader with name \p terminalName. An output specific to the \p
/// terminalName is created on \p material, using the provided \p renderContext. A connection is
/// then created between the two such that the output of the shader propagates to the output of the
/// material.
///
/// Returns the shader output on success, or an invalid UsdShadeOutput
/// otherwise.
MAYAUSD_CORE_PUBLIC
UsdShadeOutput CreateShaderOutputAndConnectMaterial(
    UsdShadeShader&   shader,
    UsdShadeMaterial& material,
    const TfToken&    terminalName,
    const TfToken&    renderContext);

// Connects a place2dTexture to a texture node
MAYAUSD_CORE_PUBLIC
void ConnectPlace2dTexture(MObject textureNode, MObject uvNode);

// Creates and connects a place2dTexture to a texture node
MAYAUSD_CORE_PUBLIC
MObject CreatePlace2dTextureAndConnectTexture(MObject textureNode);

/// Computes a USD file name from a Maya file name.
///
/// Makes path relative to \p usdFileName and resolves issues with UDIM naming.
MAYAUSD_CORE_PUBLIC void ResolveUsdTextureFileName(
    std::string&       fileTextureName,
    const std::string& usdFileName,
    bool               isUDIM);

/// Computes how many channels a texture file has by loading its header from disk
MAYAUSD_CORE_PUBLIC int GetNumberOfChannels(const std::string& fileTextureName);

} // namespace UsdMayaShadingUtil

PXR_NAMESPACE_CLOSE_SCOPE

#endif
