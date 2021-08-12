//
// Copyright 2021 Autodesk
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
#include "selectability.h"

#include <pxr/base/tf/hashmap.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  The tokens used in the selectability metadata.
 */
TfToken Selectability::MetadataToken("maya_selectability");
TfToken Selectability::InheritToken("inherit");
TfToken Selectability::OnToken("on");
TfToken Selectability::OffToken("off");

namespace {
// Very simple selectability cache for prims to avoid rechecking the metadata.
using Data = bool;
using SelectabilityCache = TfHashMap<UsdPrim, Data, boost::hash<UsdPrim>>;

// Use a function to retrieve the cache, as this exploits the C++ guaranteed
// initialization of static in funtions.
SelectabilityCache& getCache()
{
    static SelectabilityCache cache;
    return cache;
}

void clearCache() { getCache().clear(); }

// Check selectability for a prim and recurse to parent if inheriting.
bool isSelectableUncached(UsdPrim prim)
{
    const Selectability::State state = Selectability::getLocalState(prim);
    switch (state) {
    case Selectability::kOn: return true;
    case Selectability::kOff: return false;
    default: return Selectability::isSelectable(prim.GetParent());
    }
}
} // namespace

/*! \brief  Do any internal preparation for selection needed.
 */
void Selectability::prepareForSelection() { clearCache(); }

/*! \brief  Compute the selectability of a prim, considering inheritence.
 */
bool Selectability::isSelectable(UsdPrim prim)
{
    // The reason we treat invalid prim as selectable is two-fold:
    //
    // - We don't want to influence selectability of things that are not prim that are being
    //   tested by accident.
    // - We loop inheritence until we reach an invalid parent prim, and prim are selectable
    //   by default.
    if (!prim.IsValid())
        return true;

    auto&      cache = getCache();
    const auto pos = cache.find(prim);
    if (pos != cache.end())
        return pos->second;

    const bool selectable = isSelectableUncached(prim);
    cache[prim] = selectable;
    return selectable;
}

/*! \brief  Retrieve the local selectability state of a prim, without any inheritence.
 */
Selectability::State Selectability::getLocalState(const UsdPrim& prim)
{
    TfToken selectability;
    if (!prim.GetMetadata(MetadataToken, &selectability))
        return kInherit;

    if (selectability == OffToken) {
        return kOff;
    } else if (selectability == OnToken) {
        return kOn;
    } else {
        // Invalid values will be treated as inherit.
        return kInherit;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
