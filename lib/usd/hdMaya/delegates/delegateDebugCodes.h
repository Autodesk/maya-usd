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
#ifndef HDMAYA_DELEGATE_DEBUG_CODES_H
#define HDMAYA_DELEGATE_DEBUG_CODES_H

#include <pxr/base/tf/debug.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEBUG_CODES(
    HDMAYA_DELEGATE_GET,
    HDMAYA_DELEGATE_GET_CULL_STYLE,
    HDMAYA_DELEGATE_GET_CURVE_TOPOLOGY,
    HDMAYA_DELEGATE_GET_DISPLAY_STYLE,
    HDMAYA_DELEGATE_GET_DOUBLE_SIDED,
    HDMAYA_DELEGATE_GET_EXTENT,
    HDMAYA_DELEGATE_GET_INSTANCER_ID,
    HDMAYA_DELEGATE_GET_INSTANCE_INDICES,
    HDMAYA_DELEGATE_GET_LIGHT_PARAM_VALUE,
    HDMAYA_DELEGATE_GET_MATERIAL_ID,
    HDMAYA_DELEGATE_GET_MATERIAL_RESOURCE,
    HDMAYA_DELEGATE_GET_MESH_TOPOLOGY,
    HDMAYA_DELEGATE_GET_PRIMVAR_DESCRIPTORS,
    HDMAYA_DELEGATE_GET_RENDER_TAG,
    HDMAYA_DELEGATE_GET_SUBDIV_TAGS,
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

// Debug codes for Hydra API that was deprecated with USD 20.11.
// These are declared in a separate block to avoid using a preprocessor
// directive inside the TF_DEBUG_CODES() macro invocation, which breaks
// compilation on Windows.
#if USD_VERSION_NUM < 2011
// clang-format off
TF_DEBUG_CODES(
    HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE,
    HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE_ID);
// clang-format on
#endif // USD_VERSION_NUM < 2011

// Debug codes for Hydra API that was deprecated after USD 19.11.
// These are declared in a separate block to avoid using a preprocessor
// directive inside the TF_DEBUG_CODES() macro invocation, which breaks
// compilation on Windows.
#if USD_VERSION_NUM <= 1911
// clang-format off
TF_DEBUG_CODES(
    HDMAYA_DELEGATE_GET_DISPLACEMENT_SHADER_SOURCE,
    HDMAYA_DELEGATE_GET_MATERIAL_METADATA,
    HDMAYA_DELEGATE_GET_MATERIAL_PARAM_VALUE,
    HDMAYA_DELEGATE_GET_MATERIAL_PARAMS,
    HDMAYA_DELEGATE_GET_SURFACE_SHADER_SOURCE);
// clang-format on
#endif // USD_VERSION_NUM <= 1911

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_DELEGATE_DEBUG_CODES_H
