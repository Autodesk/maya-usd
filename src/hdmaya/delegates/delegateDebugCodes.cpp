//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <hdmaya/delegates/delegateDebugCodes.h>

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug) {
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MESH_TOPOLOGY,
        "Print information about 'GetMeshTopology' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_EXTENT,
        "Print information about 'GetExtent' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_TRANSFORM,
        "Print information about 'GetTransform' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_IS_ENABLED,
        "Print information about 'IsEnabled' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET,
        "Print information about 'Get' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_PRIMVAR_DESCRIPTORS,
        "Print information about 'GetPrimvarDescriptors' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_LIGHT_PARAM_VALUE,
        "Print information about 'GetLightParamValue' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_VISIBLE,
        "Print information about 'GetVisible' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_DOUBLE_SIDED,
        "Print information about 'GetDoubleSided' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_CULL_STYLE,
        "Print information about 'GetCullStyle' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_DISPLAY_STYLE,
        "Print information about 'GetDisplayStyle' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_ID,
        "Print information about 'GetMaterialId' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_SURFACE_SHADER_SOURCE,
        "Print information about 'GetSurfaceShaderSource' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_DISPLACEMENT_SHADER_SOURCE,
        "Print information about 'GetDisplacementShaderSource' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_PARAM_VALUE,
        "Print information about 'GetMaterialParamValue' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_PARAMS,
        "Print information about 'GetMaterialParams' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_RESOURCE,
        "Print information about 'GetMaterialResource' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_PRIMVARS,
        "Print information about 'GetMaterialPrimvars' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE_ID,
        "Print information about 'GetTextureResourceID' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE,
        "Print information about 'GetTextureResource' calls to the delegates.");
}

PXR_NAMESPACE_CLOSE_SCOPE
