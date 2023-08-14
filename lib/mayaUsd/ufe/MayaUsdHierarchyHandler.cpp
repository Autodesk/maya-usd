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
#include "MayaUsdHierarchyHandler.h"

#include <mayaUsd/ufe/MayaUsdHierarchy.h>
#include <mayaUsd/ufe/MayaUsdRootChildHierarchy.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdHierarchyHandler, MayaUsdHierarchyHandler);

/*static*/
MayaUsdHierarchyHandler::Ptr MayaUsdHierarchyHandler::create()
{
    return std::make_shared<MayaUsdHierarchyHandler>();
}

//------------------------------------------------------------------------------
// UsdHierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr MayaUsdHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    PXR_NAMESPACE_USING_DIRECTIVE
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    if (!TF_VERIFY(usdItem)) {
        return nullptr;
    }
    if (UsdUfe::isRootChild(usdItem->path())) {
        return MayaUsdRootChildHierarchy::create(usdItem);
    }
    return MayaUsdHierarchy::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
