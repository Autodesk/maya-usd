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
#include "GatewayHierarchyHandler.h"

#include <mayaUsd/ufe/GatewayHierarchy.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <ufe/runTimeMgr.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::HierarchyHandler, GatewayHierarchyHandler);

GatewayHierarchyHandler::GatewayHierarchyHandler(
    const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
    : Ufe::HierarchyHandler()
    , _mayaHierarchyHandler(mayaHierarchyHandler)
{
}

/*static*/
GatewayHierarchyHandler::Ptr
GatewayHierarchyHandler::create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
{
    return std::make_shared<GatewayHierarchyHandler>(mayaHierarchyHandler);
}

//------------------------------------------------------------------------------
// Ufe::HierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr GatewayHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    auto nodeType = UsdUfe::getSceneItemNodeType(item);
    if (isAGatewayType(nodeType) && !isReferencedUsdSettingsNode(nodeType, item->path())) {
        return GatewayHierarchy::create(_mayaHierarchyHandler, item);
    } else {
        return _mayaHierarchyHandler->hierarchy(item);
    }
}

Ufe::SceneItem::Ptr GatewayHierarchyHandler::createItem(const Ufe::Path& path) const
{
    // The gateway distinction is decided in hierarchy() based on the item's
    // nodeType. Item construction itself does not depend on that distinction,
    // so the wrapped Maya hierarchy handler can produce the SceneItem directly.
    return _mayaHierarchyHandler->createItem(path);
}

Ufe::Hierarchy::ChildFilter GatewayHierarchyHandler::childFilter() const
{
    // Use the same child filter as the USD hierarchy handler.
    auto usdHierHand = Ufe::RunTimeMgr::instance().hierarchyHandler(getUsdRunTimeId());
    return usdHierHand->childFilter();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
