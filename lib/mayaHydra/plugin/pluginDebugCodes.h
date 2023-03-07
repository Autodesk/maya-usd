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
#ifndef HDMAYA_PLUGIN_DEBUG_CODES_H
#define HDMAYA_PLUGIN_DEBUG_CODES_H

#include <pxr/base/tf/debug.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEBUG_CODES(
    HDMAYA_RENDEROVERRIDE_DEFAULT_LIGHTING,
    HDMAYA_RENDEROVERRIDE_RENDER,
    HDMAYA_RENDEROVERRIDE_RESOURCES,
    HDMAYA_RENDEROVERRIDE_SELECTION
);
// clang-format on

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_PLUGIN_DEBUG_CODES_H
