// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdHierarchy.h"
#include "UsdRootChildHierarchy.h"

#include "ufe/hierarchyHandler.h"

//PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy handler.
/*!
	This hierarchy handler is the standard USD run-time hierarchy handler.  Its
	only special behavior is to return a UsdRootChildHierarchy interface object
	if it is asked for a hierarchy interface for a child of the USD root prim.
	These prims are special because we define their parent to be the Maya USD
	gateway node, which the UsdRootChildHierarchy interface implements.
 */
class MAYAUSD_CORE_PUBLIC UsdHierarchyHandler : public Ufe::HierarchyHandler
{
public:
	typedef std::shared_ptr<UsdHierarchyHandler> Ptr;

	UsdHierarchyHandler();
	~UsdHierarchyHandler() override;

	// Delete the copy/move constructors assignment operators.
	UsdHierarchyHandler(const UsdHierarchyHandler&) = delete;
	UsdHierarchyHandler& operator=(const UsdHierarchyHandler&) = delete;
	UsdHierarchyHandler(UsdHierarchyHandler&&) = delete;
	UsdHierarchyHandler& operator=(UsdHierarchyHandler&&) = delete;

	//! Create a UsdHierarchyHandler.
	static UsdHierarchyHandler::Ptr create();

	// UsdHierarchyHandler overrides
	Ufe::Hierarchy::Ptr hierarchy(const Ufe::SceneItem::Ptr& item) const override;
	Ufe::SceneItem::Ptr createItem(const Ufe::Path& path) const override;

private:
	UsdRootChildHierarchy::Ptr fUsdRootChildHierarchy;
	UsdHierarchy::Ptr fUsdHierarchy;

}; // UsdHierarchyHandler

} // namespace ufe
} // namespace MayaUsd
