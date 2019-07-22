// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdHierarchyHandler.h"
#include "UsdSceneItem.h"
#include "Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdHierarchyHandler::UsdHierarchyHandler()
	: Ufe::HierarchyHandler()
{
	fUsdRootChildHierarchy = UsdRootChildHierarchy::create();
	fUsdHierarchy = UsdHierarchy::create();
}

UsdHierarchyHandler::~UsdHierarchyHandler()
{
}

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
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
	if(isRootChild(usdItem->path()))
	{
		fUsdRootChildHierarchy->setItem(usdItem);
		return fUsdRootChildHierarchy;
	}
	else
	{
		fUsdHierarchy->setItem(usdItem);
		return fUsdHierarchy;
	}
}

Ufe::SceneItem::Ptr UsdHierarchyHandler::createItem(const Ufe::Path& path) const
{
	const UsdPrim prim = ufePathToPrim(path);
	return prim.IsValid() ? UsdSceneItem::create(path, prim) : nullptr;
}

} // namespace ufe
} // namespace MayaUsd
