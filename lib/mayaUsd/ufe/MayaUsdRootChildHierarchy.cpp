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
#include "MayaUsdRootChildHierarchy.h"

#include "Utils.h"

#include <mayaUsd/ufe/MayaUsdHierarchy.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdRootChildHierarchy, MayaUsdRootChildHierarchy);

MayaUsdRootChildHierarchy::MayaUsdRootChildHierarchy(const UsdUfe::UsdSceneItem::Ptr& item)
    : UsdRootChildHierarchy(item)
{
}

/*static*/
MayaUsdRootChildHierarchy::Ptr
MayaUsdRootChildHierarchy::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdRootChildHierarchy>(item);
}

//------------------------------------------------------------------------------
// UsdHierarchy overrides
//------------------------------------------------------------------------------

bool MayaUsdRootChildHierarchy::childrenHook(
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive) const
{
    return mayaUsdHierarchyChildrenHook(
        getProxyShapePrimPath(sceneItem()->path()), child, children, filterInactive);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
