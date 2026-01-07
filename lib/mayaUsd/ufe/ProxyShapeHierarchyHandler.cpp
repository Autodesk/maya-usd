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
#include "ProxyShapeHierarchyHandler.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHierarchy.h>
#include <mayaUsd/ufe/Utils.h>

#include <ufe/runTimeMgr.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::HierarchyHandler, ProxyShapeHierarchyHandler);

ProxyShapeHierarchyHandler::ProxyShapeHierarchyHandler(
    const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
    : Ufe::HierarchyHandler()
    , _mayaHierarchyHandler(mayaHierarchyHandler)
{
}

/*static*/
ProxyShapeHierarchyHandler::Ptr
ProxyShapeHierarchyHandler::create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
{
    return std::make_shared<ProxyShapeHierarchyHandler>(mayaHierarchyHandler);
}

//------------------------------------------------------------------------------
// Ufe::HierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr ProxyShapeHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    // Check gateway type only if it is a Maya scene item
    if (!downcast(item) && isAGatewayType(item->nodeType())) {
        return ProxyShapeHierarchy::create(_mayaHierarchyHandler, item);
    } else {
        return _mayaHierarchyHandler->hierarchy(item);
    }
}

Ufe::SceneItem::Ptr ProxyShapeHierarchyHandler::createItem(const Ufe::Path& path) const
{
    return _mayaHierarchyHandler->createItem(path);
}

Ufe::Hierarchy::ChildFilter ProxyShapeHierarchyHandler::childFilter() const
{
    // Use the same child filter as the USD hierarchy handler.
    auto usdHierHand = Ufe::RunTimeMgr::instance().hierarchyHandler(getUsdRunTimeId());
    return usdHierHand->childFilter();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
