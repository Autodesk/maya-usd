//
// Copyright 2024 Animal Logic
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

#ifndef MAYAUSD_VARIANTFALLBACKS_H
#define MAYAUSD_VARIANTFALLBACKS_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/usd/pcp/types.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MApiNamespace.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

MAYAUSD_CORE_PUBLIC
PXR_NS::PcpVariantFallbackMap convertVariantFallbackFromStr(const MString& fallbacksStr);

// Set global variant fallbacks with a custom variant fallbacks.
MAYAUSD_CORE_PUBLIC
PXR_NS::PcpVariantFallbackMap updateVariantFallbacks(
    PXR_NS::PcpVariantFallbackMap&       defaultVariantFallbacks,
    const PXR_NS::MayaUsdProxyShapeBase& proxyShape);

// Save variant fallbacks string for proxy shape.
MAYAUSD_CORE_PUBLIC
void saveVariantFallbacks(
    const PXR_NS::PcpVariantFallbackMap& fallbacks,
    const PXR_NS::MayaUsdProxyShapeBase& proxyShape);

MAYAUSD_CORE_PUBLIC
MString convertVariantFallbacksToStr(const PXR_NS::PcpVariantFallbackMap& fallbacks);

} // namespace MAYAUSD_NS_DEF

#endif