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

#include "variantFallbacks.h"

#include <pxr/base/js/json.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

namespace MAYAUSD_NS_DEF {

namespace {

const char variantFallbacksAttrName[] = "variantFallbacks";

} // namespace

PXR_NS::PcpVariantFallbackMap convertVariantFallbackFromStr(const MString& fallbacksStr)
{
    if (!fallbacksStr.length()) {
        return {};
    }

    JsParseError parseError;
    JsValue      jsValue(JsParseString(fallbacksStr.asChar(), &parseError));
    if (parseError.line || !jsValue.IsObject()) {
        TF_WARN(
            "Incorrect variant fallbacks, value must be a string form of JSON data: \"%s\"",
            fallbacksStr.asChar());
        return {};
    }

    JsObject              jsObject(jsValue.GetJsObject());
    PcpVariantFallbackMap result;
    for (const auto& variantSet : jsObject) {
        const std::string& variantName = variantSet.first;
        if (!variantSet.second.IsArray()) {
            TF_WARN(
                "Unexpected data: variant value for \"%s\" must be an array. ",
                variantName.c_str());
            continue;
        }
        result[variantName] = variantSet.second.GetArrayOf<std::string>();
    }
    return result;
}

PXR_NS::PcpVariantFallbackMap updateVariantFallbacks(
    PXR_NS::PcpVariantFallbackMap&       defaultVariantFallbacks,
    const PXR_NS::MayaUsdProxyShapeBase& proxyShape)
{
    MStatus status;
    MString fallbackString {};

    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull()) {
        return {};
    }

    MFnDependencyNode depNode(proxyObj);
    MPlug             attr = depNode.findPlug(variantFallbacksAttrName, &status);

    if (status) {
        attr.getValue(fallbackString);
    } else {
        TF_CODING_ERROR(
            "Unable to get attribute \"%s\" of type MString. - %s",
            variantFallbacksAttrName,
            status.errorString().asChar());
    }
    auto fallbacks(convertVariantFallbackFromStr(fallbackString));
    if (!fallbacks.empty()) {
        defaultVariantFallbacks = PXR_NS::UsdStage::GetGlobalVariantFallbacks();
        PXR_NS::UsdStage::SetGlobalVariantFallbacks(fallbacks);
        return fallbacks;
    }
    return {};
}

MString convertVariantFallbacksToStr(const PXR_NS::PcpVariantFallbackMap& fallbacks)
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

void saveVariantFallbacks(
    const PXR_NS::PcpVariantFallbackMap& fallbacks,
    const PXR_NS::MayaUsdProxyShapeBase& proxyShape)
{
    MString fallbacksStr = convertVariantFallbacksToStr(fallbacks);
    MStatus status;
    MString fallbackString {};

    MObject proxyObj = proxyShape.thisMObject();
    if (proxyObj.isNull()) {
        return;
    }

    MFnDependencyNode depNode(proxyObj);
    MPlug             attr = depNode.findPlug(variantFallbacksAttrName, &status);

    if (status) {
        attr.getValue(fallbackString);
    } else {
        TF_CODING_ERROR(
            "Unable to get attribute \"%s\" of type MString. - %s",
            variantFallbacksAttrName,
            status.errorString().asChar());
    }
    if (fallbacksStr != fallbackString) {
        status = attr.setString(fallbacksStr);
        if (!status) {
            TF_CODING_ERROR(
                "Unable to set attribute \"%s\" of type MString - %s",
                variantFallbacksAttrName,
                status.errorString().asChar());
        }
    }
}

} // namespace MAYAUSD_NS_DEF