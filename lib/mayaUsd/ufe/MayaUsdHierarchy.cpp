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
#include "MayaUsdHierarchy.h"

#include <mayaUsd/fileio/primUpdaterManager.h>

#include <ufe/pathString.h>

#include <string>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MayaUsdHierarchy::MayaUsdHierarchy(const UsdSceneItem::Ptr& item)
    : UsdUfe::UsdHierarchy(item)
{
}

MayaUsdHierarchy::~MayaUsdHierarchy() { }

/*static*/
MayaUsdHierarchy::Ptr MayaUsdHierarchy::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdHierarchy>(item);
}

//------------------------------------------------------------------------------
// UsdHierarchy overrides
//------------------------------------------------------------------------------

bool MayaUsdHierarchy::childrenHook(
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive) const
{
    return mayaUsdHierarchyChildrenHook(child, children, filterInactive);
}

bool mayaUsdHierarchyChildrenHook(
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive)
{
    std::string dagPathStr;
    if (MayaUsd::readPullInformation(child, dagPathStr)) {
        auto item = Ufe::Hierarchy::createItem(Ufe::PathString::path(dagPathStr));
        // if we mapped to a valid object, insert it. it's possible that we got stale object
        // so in this case simply fallback to the usual processing of items
        if (item) {
            children.emplace_back(item);
            return true;
        }
    }
    return false;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
