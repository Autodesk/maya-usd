// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdRootChildHierarchy.h"

#include "ufe/runTimeMgr.h"

#include <cassert>

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
extern Ufe::Rtid g_MayaRtid;
const std::string kNotGatewayNodePath = "Tail of path %s is not a gateway node.";

UsdRootChildHierarchy::UsdRootChildHierarchy()
	: UsdHierarchy()
{
}

UsdRootChildHierarchy::~UsdRootChildHierarchy()
{
}

/*static*/
UsdRootChildHierarchy::Ptr UsdRootChildHierarchy::create()
{
	return std::make_shared<UsdRootChildHierarchy>();
}

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdRootChildHierarchy::parent() const
{
	// If we're a child of the root, our parent node in the path is a Maya
	// node.  Ask the Maya hierarchy interface to create a selection item
	// for that path.
	auto parentPath = path().pop();
#if !defined(NDEBUG)
	assert(parentPath.runTimeId() == g_MayaRtid);
#endif
	if (parentPath.runTimeId() != g_MayaRtid)
		TF_WARN(kNotGatewayNodePath.c_str(), path().string().c_str());

	auto mayaHierarchyHandler = Ufe::RunTimeMgr::instance().hierarchyHandler(g_MayaRtid);
	return mayaHierarchyHandler->createItem(parentPath);
}

} // namespace ufe
} // namespace MayaUsd
