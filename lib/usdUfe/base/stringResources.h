//
// Copyright 2026 Autodesk
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
#ifndef USDUFE_STRING_RESOURCES_H
#define USDUFE_STRING_RESOURCES_H

#include <stringResourceBase.h>

#include <usdUfe/base/api.h>

#include <string>

namespace USDUFE_NS_DEF {
namespace StringResources {

using StringResourceBase::Resource;
using StringResourceBase::ResourceMap;

// Register all UsdUfe strings.
USDUFE_PUBLIC bool registerStrings();

// Register every string of a map with the DCC. Also used by the layer editor for its own strings.
// Keeps going after a failure and returns false if any string failed to register.
USDUFE_PUBLIC bool registerResources(const ResourceMap& resources);

// Retrieve a string resource, translated for the running DCC.
USDUFE_PUBLIC std::string getAsString(const Resource& strResID);

// Every resource below, keyed by Resource::_key; a DCC walks this to populate its own catalog.
USDUFE_PUBLIC const ResourceMap& allResources();

// Shorthands taking the bare resource name, e.g. GET_USDUFE_STR(kCancel).
#define GET_USDUFE_STR(key) UsdUfe::StringResources::getAsString(UsdUfe::StringResources::key)

// -------------------------------------------------------------
//
// clang-format off

// The list of all resources. Add new strings here only: the variable name matches its key.
// These are defaults; a DCC that registers the key in its own catalog overrides what is shown.
//
// Each is declared extern below and defined once in stringResources.cpp, so a Resource is built
// once per string rather than once per translation unit including this header (C++14: no inline
// variables).

#define USDUFE_STRING_LIST(X) \
    /* UsdContextOps: */ \
    X(kUSDLoadLabel, "Load") \
    X(kUSDLoadWithDescendantsLabel, "Load with Descendants") \
    X(kUSDUnloadLabel, "Unload") \
    X(kUSDVariantSetsLabel, "Variant Sets") \
    X(kUSDMakeVisibleLabel, "Make Visible") \
    X(kUSDMakeInvisibleLabel, "Make Invisible") \
    X(kUSDActivatePrimLabel, "Activate Prim") \
    X(kUSDDeactivatePrimLabel, "Deactivate Prim") \
    X(kUSDMarkAsInstanceableLabel, "Mark as Instanceable") \
    X(kUSDUnmarkAsInstanceableLabel, "Unmark as Instanceable") \
    X(kUSDSetAsDefaultLabel, "Set as Default Prim") \
    X(kUSDClearDefaultLabel, "Clear Default Prim") \
    X(kUSDAddNewPrimLabel, "Add New Prim") \
    X(kAllRegisteredTypesLabel, "All Registered") \
    X(kAssignExistingMaterialLabel, "Assign Existing Material") \
    X(kAssignNewMaterialLabel, "Assign New Material") \
    X(kBulkEditMixedTypeLabel, "^1s Prims Selected") \
    X(kBulkEditSameTypeLabel, "^1s ^2s Prims Selected")

// Only valid inside namespace StringResources; undef to avoid polluting the global namespace.
#define DECLARE_USDUFE_STR(key, value) USDUFE_PUBLIC extern const Resource key;
USDUFE_STRING_LIST(DECLARE_USDUFE_STR)
#undef DECLARE_USDUFE_STR

// clang-format on
//
// -------------------------------------------------------------

} // namespace StringResources
} // namespace USDUFE_NS_DEF

#endif // USDUFE_STRING_RESOURCES_H
