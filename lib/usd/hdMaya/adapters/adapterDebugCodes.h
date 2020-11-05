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
#ifndef HDMAYA_ADAPTER_DEBUG_CODES_H
#define HDMAYA_ADAPTER_DEBUG_CODES_H

#include <pxr/base/tf/debug.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEBUG_CODES(
    HDMAYA_ADAPTER_CALLBACKS,
    HDMAYA_ADAPTER_CURVE_PLUG_DIRTY,
    HDMAYA_ADAPTER_CURVE_UNHANDLED_PLUG_DIRTY,
    HDMAYA_ADAPTER_DAG_HIERARCHY,
    HDMAYA_ADAPTER_DAG_PLUG_DIRTY,
    HDMAYA_ADAPTER_GET,
    HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE,
    HDMAYA_ADAPTER_IMAGEPLANES,
    HDMAYA_ADAPTER_LIGHT_SHADOWS,
    HDMAYA_ADAPTER_MATERIALS,
    HDMAYA_ADAPTER_MESH_PLUG_DIRTY,
    HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY);
// clang-format on

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_ADAPTER_DEBUG_CODES_H
