//
// Copyright 2019 Autodesk
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
#include "UsdHierarchyHandler.h"

#include <usdUfe/ufe/UsdHierarchy.h>
#include <usdUfe/ufe/UsdRootChildHierarchy.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/diagnostic.h>

namespace USDUFE_NS_DEF {

UsdHierarchyHandler::UsdHierarchyHandler()
    : Ufe::HierarchyHandler()
{
}

UsdHierarchyHandler::~UsdHierarchyHandler() { }

/*static*/
UsdHierarchyHandler::Ptr UsdHierarchyHandler::create()
{
    return std::make_shared<UsdHierarchyHandler>();
}

//------------------------------------------------------------------------------
// UsdHierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr UsdHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    if (!TF_VERIFY(usdItem)) {
        return nullptr;
    }
    return isRootChild(usdItem->path()) ? UsdRootChildHierarchy::create(usdItem)
                                        : UsdHierarchy::create(usdItem);
}

Ufe::SceneItem::Ptr UsdHierarchyHandler::createItem(const Ufe::Path& path) const
{
    PXR_NS::UsdPrim prim;
    const int       instanceIndex = ufePathToInstanceIndex(path, &prim);
    return prim.IsValid() ? UsdSceneItem::create(path, prim, instanceIndex) : nullptr;
}

Ufe::Hierarchy::ChildFilter UsdHierarchyHandler::childFilter() const
{
    Ufe::Hierarchy::ChildFilter childFilters;
    childFilters.emplace_back("InactivePrims", "Inactive Prims", true);
    return childFilters;
}

} // namespace USDUFE_NS_DEF
