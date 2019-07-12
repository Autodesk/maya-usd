// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "ProxyShapeHierarchyHandler.h"
#include "Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

ProxyShapeHierarchyHandler::ProxyShapeHierarchyHandler(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler)
	: Ufe::HierarchyHandler()
	, fMayaHierarchyHandler(mayaHierarchyHandler)
{
	fProxyShapeHierarchy = ProxyShapeHierarchy::create(mayaHierarchyHandler);
}

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
		fProxyShapeHierarchy->setItem(item);
		return fProxyShapeHierarchy;
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
