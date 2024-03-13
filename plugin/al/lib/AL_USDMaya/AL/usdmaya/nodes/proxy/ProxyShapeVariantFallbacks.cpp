//
// Copyright 2017 Animal Logic
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

#include "AL/usdmaya/nodes/ProxyShape.h"

#include <pxr/base/js/json.h>
#include <pxr/usd/pcp/types.h>

#include <maya/MProfiler.h>
namespace {
const int _proxyShapeVariantFallbacksProfilerCategory = MProfiler::addCategory(
    "AL_usdmaya_ProxyShape_variant_fallbacks",
    "AL_usdmaya_ProxyShape_variant_fallbacks");
} // namespace

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
MString ProxyShape::getVariantFallbacksFromLayer(const SdfLayerRefPtr& layer) const
{
    MProfilingScope profilerScope(
        _proxyShapeVariantFallbacksProfilerCategory,
        MProfiler::kColorE_L3,
        "Get variant fallbacks from layer");

    if (!layer) {
        return {};
    }

    static const std::string kVariantFallbacksToken = "variant_fallbacks";
    const VtDictionary&      data(layer->GetCustomLayerData());
    const auto&              valIt(data.find(kVariantFallbacksToken));
    if (valIt == data.end()) {
        return {};
    }

    const VtValue& customFallbacksVal(valIt->second);
    if (!customFallbacksVal.IsHolding<std::string>()) {
        TF_WARN(
            "Session layer has wrong \"%s\" data type, value must be a string.",
            kVariantFallbacksToken.c_str());
        return {};
    }
    auto result = customFallbacksVal.Get<std::string>();
    if (result.empty()) {
        return {};
    }
    return result.c_str();
}

} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
