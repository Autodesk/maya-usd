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
#ifndef MAYAUSD_STRING_RESOURCES_H
#define MAYAUSD_STRING_RESOURCES_H

#include <mayaUsd/base/api.h>
#include <stringResourceBase.h>

#include <maya/MString.h>

#include <string>

namespace MAYAUSD_NS_DEF {
namespace StringResources {

using StringResourceBase::Resource;
using StringResourceBase::ResourceMap;

// Register all MayaUsd strings.
MAYAUSD_CORE_PUBLIC bool registerStrings();

// Register one string, or every string in a map, with Maya under kStringResourcePluginId. Also used
// by MayaUsdUI and by the UsdUfe hooks, so Maya's string API is only touched in one place.
MAYAUSD_CORE_PUBLIC bool registerString(const Resource& strResID);
MAYAUSD_CORE_PUBLIC bool registerResources(const ResourceMap& resources);

// Retrieve a string resource, translated by Maya. Falls back to the source text if Maya has no
// entry for the key (for example if registration failed or has not happened yet).
MAYAUSD_CORE_PUBLIC MString getAsMString(const Resource& strResID);
MAYAUSD_CORE_PUBLIC std::string getAsString(const Resource& strResID);

// Shorthands taking the bare resource name, e.g. GET_MAYAUSD_STR(kCancel).
#define GET_MAYAUSD_MSTR(key) MayaUsd::StringResources::getAsMString(MayaUsd::StringResources::key)
#define GET_MAYAUSD_STR(key)  MayaUsd::StringResources::getAsString(MayaUsd::StringResources::key)

// -------------------------------------------------------------
//
// clang-format off

// The list of all resources. Add new strings here only: the variable name matches its key.
// Each is declared extern below and defined once in stringResources.cpp, so a Resource is built
// once per string rather than once per translation unit including this header (C++14: no inline
// variables).
#define MAYAUSD_STRING_LIST(X) \
    /* MayaUsdContextOps: */ \
    X(kUSDLayerEditorLabel, "USD Layer Editor") \
    X(kAssetResolverDialogLabel, "USD Path Editor") \
    X(kUSDCompositionEditorLabel, "USD Composition Editor") \
    X(kEditAsMayaLabel, "Edit As Maya Data") \
    X(kEditAsMayaOptionsLabel, "Edit As Maya Data Options...") \
    X(kDuplicateAsMayaLabel, "Duplicate As Maya Data") \
    X(kAddMayaReferenceLabel, "Add Maya Reference...") \
    X(kBindMaterialToSelectionLabel, "Assign Material to Selection") \
    X(kUpgradeMaterialLabel, "Convert to current version of MaterialX") \
    X(kAddNewMaterialLabel, "Add New Material") \
    X(kAddRefOrPayloadLabel, "Add to Prim...") \
    X(kAddRefToNewPrimLabel, "Add...") \
    X(kClearAllRefsOrPayloadsLabel, "Clear...") \
    X(kReloadReferenceLabel, "Reload") \
    X(kUSDReferenceLabel, "Reference") \
    X(kAddReferenceLabel, "Add Reference...")

// Only valid inside namespace StringResources; undef to avoid polluting the global namespace.
#define DECLARE_MAYAUSD_STR(key, value) MAYAUSD_CORE_PUBLIC extern const Resource key;
MAYAUSD_STRING_LIST(DECLARE_MAYAUSD_STR)
#undef DECLARE_MAYAUSD_STR

// clang-format on
//
// -------------------------------------------------------------

} // namespace StringResources
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_STRING_RESOURCES_H
