//
// Copyright 2019 Luma Pictures
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
#ifndef __HDMAYA_DELEGATE_DEBUG_CODES_H__
#define __HDMAYA_DELEGATE_DEBUG_CODES_H__

#include <pxr/pxr.h>

#include <pxr/base/tf/debug.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEBUG_CODES(
    HDMAYA_DELEGATE_GET,
    HDMAYA_DELEGATE_GET_CULL_STYLE,
    HDMAYA_DELEGATE_GET_CURVE_TOPOLOGY,
    HDMAYA_DELEGATE_GET_DISPLACEMENT_SHADER_SOURCE,
    HDMAYA_DELEGATE_GET_DISPLAY_STYLE,
    HDMAYA_DELEGATE_GET_DOUBLE_SIDED,
    HDMAYA_DELEGATE_GET_EXTENT,
    HDMAYA_DELEGATE_GET_INSTANCE_INDICES,
    HDMAYA_DELEGATE_GET_LIGHT_PARAM_VALUE,
    HDMAYA_DELEGATE_GET_MATERIAL_ID,
    HDMAYA_DELEGATE_GET_MATERIAL_METADATA,
    HDMAYA_DELEGATE_GET_MATERIAL_PARAM_VALUE,
    HDMAYA_DELEGATE_GET_MATERIAL_PARAMS,
    HDMAYA_DELEGATE_GET_MATERIAL_PRIMVARS,
    HDMAYA_DELEGATE_GET_MATERIAL_RESOURCE,
    HDMAYA_DELEGATE_GET_MESH_TOPOLOGY,
    HDMAYA_DELEGATE_GET_PRIMVAR_DESCRIPTORS,
    HDMAYA_DELEGATE_GET_RENDER_TAG,
    HDMAYA_DELEGATE_GET_SUBDIV_TAGS,
    HDMAYA_DELEGATE_GET_SURFACE_SHADER_SOURCE,
    HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE,
    HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE_ID,
    HDMAYA_DELEGATE_GET_TRANSFORM,
    HDMAYA_DELEGATE_GET_VISIBLE,
    HDMAYA_DELEGATE_INSERTDAG,
    HDMAYA_DELEGATE_IS_ENABLED,
    HDMAYA_DELEGATE_RECREATE_ADAPTER,
    HDMAYA_DELEGATE_REGISTRY,
    HDMAYA_DELEGATE_SAMPLE_PRIMVAR,
    HDMAYA_DELEGATE_SAMPLE_TRANSFORM,
    HDMAYA_DELEGATE_SELECTION);
// clang-format on

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_DEBUG_CODES_H__
