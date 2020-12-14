//
// Copyright 2019 Luma Pictures
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
#include "delegateDebugCodes.h"

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET, "Print information about 'Get' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_CULL_STYLE,
        "Print information about 'GetCullStyle' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_CURVE_TOPOLOGY,
        "Print information about 'GetCurveTopology' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_DISPLAY_STYLE,
        "Print information about 'GetDisplayStyle' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_DOUBLE_SIDED,
        "Print information about 'GetDoubleSided' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_EXTENT, "Print information about 'GetExtent' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_INSTANCER_ID,
        "Print information about 'GetInstancerId' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_INSTANCE_INDICES,
        "Print information about GetInstanceIndices calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_LIGHT_PARAM_VALUE,
        "Print information about 'GetLightParamValue' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_ID,
        "Print information about 'GetMaterialId' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_RESOURCE,
        "Print information about 'GetMaterialResource' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MESH_TOPOLOGY,
        "Print information about 'GetMeshTopology' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_PRIMVAR_DESCRIPTORS,
        "Print information about 'GetPrimvarDescriptors' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_RENDER_TAG,
        "Print information about 'GetRenderTag' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_SUBDIV_TAGS,
        "Print information about 'GetSubdivTags' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_TRANSFORM,
        "Print information about 'GetTransform' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_VISIBLE,
        "Print information about 'GetVisible' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_INSERTDAG, "Print information about 'InsertDag' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_IS_ENABLED, "Print information about 'IsEnabled' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_RECREATE_ADAPTER,
        "Print information when the delegate recreates adapters.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_REGISTRY, "Print information about registration of HdMayaDelegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_SAMPLE_PRIMVAR,
        "Print information about 'SamplePrimvar' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_SAMPLE_TRANSFORM,
        "Print information about 'SampleTransform' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_SELECTION, "Print information about hdMaya delegate selection.");

#if USD_VERSION_NUM < 2011

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE,
        "Print information about 'GetTextureResource' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE_ID,
        "Print information about 'GetTextureResourceID' calls to the "
        "delegates.");

#endif // USD_VERSION_NUM < 2011

#if USD_VERSION_NUM <= 1911

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_DISPLACEMENT_SHADER_SOURCE,
        "Print information about 'GetDisplacementShaderSource' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_METADATA,
        "Print information about 'GetMaterialMetadata' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_PARAM_VALUE,
        "Print information about 'GetMaterialParamValue' calls to the "
        "delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_MATERIAL_PARAMS,
        "Print information about 'GetMaterialParams' calls to the delegates.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_DELEGATE_GET_SURFACE_SHADER_SOURCE,
        "Print information about 'GetSurfaceShaderSource' calls to the "
        "delegates.");

#endif // USD_VERSION_NUM <= 1911
}

PXR_NAMESPACE_CLOSE_SCOPE
