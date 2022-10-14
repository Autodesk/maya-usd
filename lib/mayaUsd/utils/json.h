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

#include <mayaUsd/base/api.h>

#include <pxr/base/js/json.h>

#include <maya/MDagPath.h>
#include <maya/MString.h>
#include <ufe/pathString.h>

namespace MAYAUSD_NS_DEF {

////////////////////////////////////////////////////////////////////////////
//
// Conversion functions to and from JSON for C++, Maya and UFE types.
//
// All functions throw C++ exceptions on error.

MAYAUSD_CORE_PUBLIC
PXR_NS::JsValue convertToValue(const std::string& text);
MAYAUSD_CORE_PUBLIC
PXR_NS::JsValue convertToValue(const MString& text);
MAYAUSD_CORE_PUBLIC
PXR_NS::JsValue convertToValue(const Ufe::Path& path);
MAYAUSD_CORE_PUBLIC
PXR_NS::JsValue convertToValue(const MDagPath& path);

MAYAUSD_CORE_PUBLIC
std::string convertToString(const PXR_NS::JsValue& value);
MAYAUSD_CORE_PUBLIC
MString convertToMString(const PXR_NS::JsValue& value);
MAYAUSD_CORE_PUBLIC
Ufe::Path convertToUfePath(const PXR_NS::JsValue& value);
MAYAUSD_CORE_PUBLIC
MDagPath convertToDagPath(const PXR_NS::JsValue& value);
MAYAUSD_CORE_PUBLIC
PXR_NS::JsArray convertToArray(const PXR_NS::JsValue& value);
MAYAUSD_CORE_PUBLIC
PXR_NS::JsObject convertToObject(const PXR_NS::JsValue& value);
MAYAUSD_CORE_PUBLIC
PXR_NS::JsValue convertJsonKeyToValue(const PXR_NS::JsObject& object, const std::string& key);

} // namespace MAYAUSD_NS_DEF
