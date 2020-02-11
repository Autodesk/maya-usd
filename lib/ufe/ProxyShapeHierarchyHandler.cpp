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
#include "Utils.h"

#include <ufe/ProxyShapeHierarchy.h>

MAYAUSD_NS_DEF {
namespace ufe {

ProxyShapeHierarchyHandler::ProxyShapeHierarchyHandler(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler)
	: Ufe::HierarchyHandler()
	, fMayaHierarchyHandler(mayaHierarchyHandler)
{}

ProxyShapeHierarchyHandler::~ProxyShapeHierarchyHandler()
{
}

/*static*/
ProxyShapeHierarchyHandler::Ptr ProxyShapeHierarchyHandler::create(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler)
{
	return std::make_shared<ProxyShapeHierarchyHandler>(mayaHierarchyHandler);
}

//------------------------------------------------------------------------------
// Ufe::HierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr ProxyShapeHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
	if (isAGatewayType(item->nodeType()))
	{
		return ProxyShapeHierarchy::create(fMayaHierarchyHandler, item);
	}
	else
	{
		return fMayaHierarchyHandler->hierarchy(item);
	}
}

Ufe::SceneItem::Ptr ProxyShapeHierarchyHandler::createItem(const Ufe::Path& path) const
{
	return fMayaHierarchyHandler->createItem(path);
}

} // namespace ufe
} // namespace MayaUsd
