//
// Copyright 2023 Autodesk
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

#ifndef PXRUSDMAYA_UTIL_DISPLAY_NAME_H
#define PXRUSDMAYA_UTIL_DISPLAY_NAME_H

#include <mayaUsd/base/api.h>

#include <string>

namespace MAYAUSD_NS_DEF {

// Load the attribute mappings.
//
// The attribute mappings are kept in a JSON file named 'attribute_mappings.json'.
// The JSON format is:
//      {
//          "version": 1.0,
//          "removed_prefixes": [ "abc", "def" ],
//          "attribute_mappings": {
//              "example-attribute-name": "example-display-name",
//              "foo": "bar",
//          },
//      }
MAYAUSD_CORE_PUBLIC
void loadAttributeNameMappings(const std::string& pluginFilePath);

// Convert the attribute name into a nice display name.
std::string getAttributeDisplayName(const std::string& attrName);

} // namespace MAYAUSD_NS_DEF

#endif
