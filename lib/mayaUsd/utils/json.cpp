//
// Copyright 2022 Autodesk
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

#include "json.h"

#include <mayaUsd/utils/util.h>

#include <ufe/pathString.h>

namespace MAYAUSD_NS_DEF {

static const char* invalidJson = "Invalid JSON";

PXR_NS::JsValue convertToValue(const std::string& text)
{
    // Provided for call consistency and in case we need to do some filtering
    // in the future.
    return PXR_NS::JsValue(text);
}

std::string convertToString(const PXR_NS::JsValue& value)
{
    if (!value.IsString())
        throw std::runtime_error(invalidJson);

    return value.GetString();
}

PXR_NS::JsValue convertToValue(const MString& text)
{
    // Provided for call consistency and in case we need to do some filtering
    // in the future.
    return PXR_NS::JsValue(text.asChar());
}

MString convertToMString(const PXR_NS::JsValue& value)
{
    return MString(convertToString(value).c_str());
}

PXR_NS::JsValue convertToValue(double value) { return PXR_NS::JsValue(value); }

double convertToDouble(const PXR_NS::JsValue& value)
{
    if (!value.IsReal())
        throw std::runtime_error(invalidJson);

    return value.GetReal();
}

PXR_NS::JsValue convertToValue(const Ufe::Path& path)
{
    return convertToValue(Ufe::PathString::string(path));
}

Ufe::Path convertToUfePath(const PXR_NS::JsValue& pathJson)
{
    return Ufe::PathString::path(convertToString(pathJson));
}

PXR_NS::JsValue convertToValue(const MDagPath& path) { return convertToValue(path.fullPathName()); }

MDagPath convertToDagPath(const PXR_NS::JsValue& value)
{
    return PXR_NS::UsdMayaUtil::nameToDagPath(convertToString(value));
}

PXR_NS::JsArray convertToArray(const PXR_NS::JsValue& value)
{
    if (!value.IsArray())
        throw std::runtime_error(invalidJson);

    return value.GetJsArray();
}

PXR_NS::JsObject convertToObject(const PXR_NS::JsValue& value)
{
    if (!value.IsObject())
        throw std::runtime_error(invalidJson);

    return value.GetJsObject();
}

PXR_NS::JsValue convertJsonKeyToValue(const PXR_NS::JsObject& object, const std::string& key)
{
    const auto pos = object.find(key);
    if (pos == object.end())
        throw std::runtime_error(invalidJson);

    return pos->second;
}

} // namespace MAYAUSD_NS_DEF
