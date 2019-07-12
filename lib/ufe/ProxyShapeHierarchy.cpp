// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "ProxyShapeHierarchy.h"
#include "Utils.h"

#include "ufe/pathComponent.h"
#include "ufe/pathSegment.h"
#include "ufe/rtid.h"

#include "pxr/usd/usd/stage.h"

#include <stdexcept>

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
extern Ufe::Rtid g_USDRtid;

//------------------------------------------------------------------------------
// ProxyShapeHierarchy
//------------------------------------------------------------------------------

ProxyShapeHierarchy::ProxyShapeHierarchy(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler)
	: Ufe::Hierarchy()
	, fMayaHierarchyHandler(mayaHierarchyHandler)
{
}

ProxyShapeHierarchy::~ProxyShapeHierarchy()
{
}

/*static*/
ProxyShapeHierarchy::Ptr ProxyShapeHierarchy::create(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler)
{
	return std::make_shared<ProxyShapeHierarchy>(mayaHierarchyHandler);
}

void ProxyShapeHierarchy::setItem(const Ufe::SceneItem::Ptr& item)
{
	fItem = item;
	fMayaHierarchy = fMayaHierarchyHandler->hierarchy(item);
}

const UsdPrim& ProxyShapeHierarchy::getUsdRootPrim() const
{
	if (!fUsdRootPrim.IsValid())
	{
		// FIXME During AL_usdmaya_ProxyShapeImport, nodes (both Maya
		// and USD) are being added (e.g. the proxy shape itself), but
		// there is no stage yet, and there is no way to detect that a
		// proxy shape import command is under way.  PPT, 28-Sep-2018.
		UsdStageWeakPtr stage = getStage(fItem->path());
		if (stage)
		{
			fUsdRootPrim = stage->GetPrimAtPath(SdfPath("/"));
		}
	}
	return fUsdRootPrim;
}

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr ProxyShapeHierarchy::sceneItem() const
{
	return fItem;
}

bool ProxyShapeHierarchy::hasChildren() const
{
	const UsdPrim& rootPrim = getUsdRootPrim();
	if (!rootPrim.IsValid())
		return false;
	return !rootPrim.GetChildren().empty();
}

Ufe::SceneItemList ProxyShapeHierarchy::children() const
{
	// Return children of the USD root.
	const UsdPrim& rootPrim = getUsdRootPrim();
	if (!rootPrim.IsValid())
		return Ufe::SceneItemList();

	auto usdChildren = rootPrim.GetChildren();
	auto parentPath = fItem->path();

	// We must create selection items for our children.  These will have as
	// path the path of the proxy shape, with a single path segment of a
	// single component appended to it.
	Ufe::SceneItemList children;
	for (const auto& child : usdChildren)
	{
		children.push_back(UsdSceneItem::create(parentPath + Ufe::PathSegment(
			Ufe::PathComponent(child.GetName().GetString()), g_USDRtid, '/'), child));
	}
	return children;
}

Ufe::SceneItem::Ptr ProxyShapeHierarchy::parent() const
{
	return fMayaHierarchy->parent();
}

Ufe::AppendedChild ProxyShapeHierarchy::appendChild(const Ufe::SceneItem::Ptr& child)
{
	throw std::runtime_error("ProxyShapeHierarchy::appendChild() not implemented");
}

Ufe::SceneItem::Ptr ProxyShapeHierarchy::createGroup(const Ufe::PathComponent& name) const
{
	throw std::runtime_error("ProxyShapeHierarchy::createGroup() not implemented");
}

Ufe::Group ProxyShapeHierarchy::createGroupCmd(const Ufe::PathComponent& name) const
{
	throw std::runtime_error("ProxyShapeHierarchy::createGroupCmd not implemented");
}

} // namespace ufe
} // namespace MayaUsd
