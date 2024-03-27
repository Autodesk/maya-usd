//
// Copyright 2024 Autodesk
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

#ifndef MAYAUSD_UTILS_JSON_DICT_H
#define MAYAUSD_UTILS_JSON_DICT_H

#include <mayaUsd/base/api.h>

#include <pxr/base/js/json.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

////////////////////////////////////////////////////////////////////////////
//
// Convert a VtDictionary to JsValue.
// Modeled after PXR_NS::JsValueTypeConverter

class VtDictionaryToJsValueConverter
{
public:
    static PXR_NS::JsObject convertToDictionary(const PXR_NS::VtDictionary& dict)
    {
        PXR_NS::JsObject object;

        for (const auto& keyAndValue : dict) {
            PXR_NS::JsValue sub;
            if (canConvertToValue(keyAndValue.second, sub))
                object[keyAndValue.first] = sub;
        }

        return object;
    }

    static bool canConvertToDictionary(const PXR_NS::VtValue& value, PXR_NS::JsValue& into)
    {
        if (!value.IsHolding<PXR_NS::VtDictionary>())
            return false;

        into = convertToDictionary(value.UncheckedGet<PXR_NS::VtDictionary>());
        return true;
    }

    static PXR_NS::JsArray convertToArrayOfValues(const std::vector<PXR_NS::VtValue>& vec)
    {
        PXR_NS::JsArray array;

        for (const PXR_NS::VtValue& element : vec) {
            PXR_NS::JsValue sub;
            if (canConvertToValue(element, sub))
                array.push_back(sub);
        }

        return array;
    }

    static bool canConvertToArrayOfValues(const PXR_NS::VtValue& value, PXR_NS::JsValue& into)
    {
        if (!value.IsHolding<std::vector<PXR_NS::VtValue>>())
            return false;

        into = convertToArrayOfValues(value.UncheckedGet<std::vector<PXR_NS::VtValue>>());
        return true;
    }

    template <typename T, typename TARGET = T>
    static PXR_NS::JsArray convertToArrayOf(const std::vector<T>& vec)
    {
        PXR_NS::JsArray array;

        for (const auto element : vec)
            array.push_back(PXR_NS::JsValue(TARGET(element)));

        return array;
    }

    template <typename T, typename TARGET = T>
    static bool canConvertToArrayOf(const PXR_NS::VtValue& value, PXR_NS::JsValue& into)
    {
        if (!value.IsHolding<std::vector<T>>())
            return false;

        into = convertToArrayOf<T, TARGET>(value.UncheckedGet<std::vector<T>>());
        return true;
    }

    static bool canConvertToArray(const PXR_NS::VtValue& value, PXR_NS::JsValue& into)
    {
        if (canConvertToArrayOf<bool>(value, into))
            return true;
        if (canConvertToArrayOf<int>(value, into))
            return true;
        if (canConvertToArrayOf<unsigned int, int>(value, into))
            return true;
        if (canConvertToArrayOf<short, int>(value, into))
            return true;
        if (canConvertToArrayOf<unsigned short, int>(value, into))
            return true;
        if (canConvertToArrayOf<long, int64_t>(value, into))
            return true;
        if (canConvertToArrayOf<unsigned long, int64_t>(value, into))
            return true;
        if (canConvertToArrayOf<int8_t, int>(value, into))
            return true;
        if (canConvertToArrayOf<uint8_t, int>(value, into))
            return true;
        if (canConvertToArrayOf<int16_t, int>(value, into))
            return true;
        if (canConvertToArrayOf<uint16_t, int>(value, into))
            return true;
        if (canConvertToArrayOf<int32_t, int>(value, into))
            return true;
        if (canConvertToArrayOf<uint32_t, int>(value, into))
            return true;
        if (canConvertToArrayOf<int64_t>(value, into))
            return true;
        if (canConvertToArrayOf<uint64_t, int64_t>(value, into))
            return true;
        if (canConvertToArrayOf<float>(value, into))
            return true;
        if (canConvertToArrayOf<double>(value, into))
            return true;
        if (canConvertToArrayOf<std::string>(value, into))
            return true;
        if (canConvertToArrayOfValues(value, into))
            return true;

        // TODO: we don't support arrays or arrays or arrays of dictionaries.
        //       We never need that for our purpose.

        return false;
    }

    template <typename T, typename TARGET = T>
    static bool canConvertTo(const PXR_NS::VtValue& value, PXR_NS::JsValue& into)
    {
        if (!value.IsHolding<T>())
            return false;
        into = PXR_NS::JsValue(TARGET(value.UncheckedGet<T>()));
        return true;
    }

    static bool canConvertToValue(const PXR_NS::VtValue& value, PXR_NS::JsValue& into)
    {
        if (canConvertTo<bool>(value, into))
            return true;
        if (canConvertTo<int>(value, into))
            return true;
        if (canConvertTo<unsigned int, int>(value, into))
            return true;
        if (canConvertTo<short, int>(value, into))
            return true;
        if (canConvertTo<unsigned short, int>(value, into))
            return true;
        if (canConvertTo<long, int64_t>(value, into))
            return true;
        if (canConvertTo<unsigned long, int64_t>(value, into))
            return true;
        if (canConvertTo<int8_t, int>(value, into))
            return true;
        if (canConvertTo<uint8_t, int>(value, into))
            return true;
        if (canConvertTo<int16_t, int>(value, into))
            return true;
        if (canConvertTo<uint16_t, int>(value, into))
            return true;
        if (canConvertTo<int32_t, int>(value, into))
            return true;
        if (canConvertTo<uint32_t, int>(value, into))
            return true;
        if (canConvertTo<int64_t>(value, into))
            return true;
        if (canConvertTo<uint64_t, int64_t>(value, into))
            return true;
        if (canConvertTo<float, double>(value, into))
            return true;
        if (canConvertTo<double>(value, into))
            return true;
        if (canConvertTo<std::string>(value, into))
            return true;

        if (canConvertToDictionary(value, into))
            return true;

        if (canConvertToArray(value, into))
            return true;

        return true;
    }
};

} // namespace MAYAUSD_NS_DEF

#endif
