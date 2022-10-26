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
#ifndef MTOH_TOKENS_H
#define MTOH_TOKENS_H

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define MTOH_TOKENS \
    (HdStormRendererPlugin) \
    (HdMayaRenderItemRendererPlugin) \
    (mtohMaximumShadowMapResolution)
// clang-format on

// This is not an exported API.
TF_DECLARE_PUBLIC_TOKENS(MtohTokens, , MTOH_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_TOKENS_H
