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

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
PcpVariantFallbackMap ProxyShape::convertVariantFallbackFromStr(const MString& fallbacksStr) const
{
    if (!fallbacksStr.length()) {
        return {};
    }

    JsParseError parseError;
    JsValue      jsValue(JsParseString(fallbacksStr.asChar(), &parseError));
    if (parseError.line || !jsValue.IsObject()) {
        MGlobal::displayError(MString(parseError.reason.c_str()));
        MGlobal::displayError(
            MString("ProxyShape attribute \"") + name()
            + ".variantFallbacks\" "
              "contains incorrect variant fallbacks, value must be a string form of JSON data.");
        return {};
    }

    JsObject              jsObject(jsValue.GetJsObject());
    PcpVariantFallbackMap result;
    for (const auto& variantSet : jsObject) {
        const std::string& variantName = variantSet.first;
        if (!variantSet.second.IsArray()) {
            MGlobal::displayError(
                MString("ProxyShape attribute \"") + name()
                + ".variantFallbacks\" "
                  "contains unexpected data: variant value for \""
                + variantName.c_str() + "\" must be an array.");
            continue;
        }
        result[variantName] = variantSet.second.GetArrayOf<std::string>();
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
MString ProxyShape::convertVariantFallbacksToStr(const PcpVariantFallbackMap& fallbacks) const
{
    if (fallbacks.empty()) {
        return {};
    }

    JsObject jsObject;
    for (const auto& variantSet : fallbacks) {
        jsObject[variantSet.first] = JsArray(variantSet.second.cbegin(), variantSet.second.cend());
    }

    return JsWriteToString(jsObject).c_str();
}

//----------------------------------------------------------------------------------------------------------------------
MString ProxyShape::getVariantFallbacksFromLayer(const SdfLayerRefPtr& layer) const
{
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

//----------------------------------------------------------------------------------------------------------------------
PcpVariantFallbackMap ProxyShape::updateVariantFallbacks(
    PcpVariantFallbackMap& defaultVariantFallbacks,
    MDataBlock&            dataBlock) const
{
    auto fallbacks(convertVariantFallbackFromStr(inputStringValue(dataBlock, m_variantFallbacks)));
    if (!fallbacks.empty()) {
        defaultVariantFallbacks = UsdStage::GetGlobalVariantFallbacks();
        TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("Setting global variant fallback");
        UsdStage::SetGlobalVariantFallbacks(fallbacks);
        return fallbacks;
    }
    return {};
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::saveVariantFallbacks(const MString& fallbacksStr, MDataBlock& dataBlock) const
{
    if (fallbacksStr != inputStringValue(dataBlock, m_variantFallbacks)) {
        TF_DEBUG(ALUSDMAYA_EVALUATION)
            .Msg("Saving global variant fallbacks: \n\"%s\"\n", fallbacksStr.asChar());
        outputStringValue(dataBlock, m_variantFallbacks, fallbacksStr);
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
