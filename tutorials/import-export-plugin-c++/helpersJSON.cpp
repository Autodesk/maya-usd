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

#include "helpers.h"

#include <mayaUsd/utils/jsonDict.h>

#include <pxr/base/js/converter.h>

#include <maya/MGlobal.h>

namespace Helpers {

PXR_NAMESPACE_USING_DIRECTIVE

// Convert a JSON-encoded text string into a USD dictionary.
Dict jsonToDictionary(const std::string& json)
{
    JsValue jsValue = JsParseString(json.c_str());

    // Note: we pass false to the template so that it uses int instead of int64.
    //       This is more compatible with the way MayaUSD encodes integers in
    //       its settings.
    VtValue value = JsValueTypeConverter<VtValue, Dict, false>::Convert(jsValue);

    if (!value.IsHolding<Dict>())
        return {};

    return value.Get<Dict>();
}

// Convert a Maya option var containing a JSON-encoded text string into a USD dictionary.
Dict jsonOptionVarToDictionary(const char* optionVarName)
{
    if (!MGlobal::optionVarExists(optionVarName))
        return {};

    MString encodedValue = MGlobal::optionVarStringValue(optionVarName);
    return jsonToDictionary(encodedValue.asChar());
}

// Convert a USD JSON object into a text string saved into a Maya option var.
void jsonToOptionVar(const char* optionVarName, const Json& jsSettings)
{
    const std::string encodedSettings = JsWriteToString(jsSettings);
    MGlobal::setOptionVarValue(optionVarName, encodedSettings.c_str());
}

// Convert a USD dictionary to a USD JSON object.
// Note: only data types supported by JSON will be converted.
Json dictionaryToJSON(const Dict& dict)
{
    return MayaUsd::VtDictionaryToJsValueConverter::convertToDictionary(dict);
}

} // namespace Helpers
