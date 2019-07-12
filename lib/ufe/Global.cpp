// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "Global.h"
#include "ProxyShapeHandler.h"
#include "StagesSubject.h"
#include "private/InPathChange.h"
#include "UsdHierarchyHandler.h"
#include "UsdTransform3dHandler.h"
#include "UsdSceneItemOpsHandler.h"

#include "ufe/rtid.h"
#include "ufe/runTimeMgr.h"
#include "ufe/hierarchyHandler.h"
#include "ufe/ProxyShapeHierarchyHandler.h"

#include <string>
#include <cassert>

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

// Maya's UFE run-time name and ID.
static const std::string kMayaRunTimeName("Maya-DG");
Ufe::Rtid g_MayaRtid = 0;

// Register this run-time with UFE under the following name.
static const std::string kUSDRunTimeName("USD");

// Our run-time ID, allocated by UFE at registration time.  Initialize it
// with illegal 0 value.
Ufe::Rtid g_USDRtid = 0;

// The normal Maya hierarchy handler, which we decorate for ProxyShape support.
// Keep a reference to it to restore on finalization.
Ufe::HierarchyHandler::Ptr g_MayaHierarchyHandler;

// Subject singleton for observation of all USD stages.
StagesSubject::Ptr g_StagesSubject;

bool InPathChange::fInPathChange = false;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Only intended to be called by the plugin initialization, to
// initialize the stage model.
MStatus initialize()
{
	// Replace the Maya hierarchy handler with ours.
	g_MayaRtid = Ufe::RunTimeMgr::instance().getId(kMayaRunTimeName);
#if !defined(NDEBUG)
	assert(g_MayaRtid != 0);
#endif
	if (g_MayaRtid == 0)
		return MS::kFailure;

	g_MayaHierarchyHandler = Ufe::RunTimeMgr::instance().hierarchyHandler(g_MayaRtid);
	auto proxyShapeHandler = ProxyShapeHierarchyHandler::create(g_MayaHierarchyHandler);
	Ufe::RunTimeMgr::instance().setHierarchyHandler(g_MayaRtid, proxyShapeHandler);

	auto usdHierHandler = UsdHierarchyHandler::create();
	auto usdTrans3dHandler = UsdTransform3dHandler::create();
	auto usdSceneHandler = UsdSceneItemOpsHandler::create();
	g_USDRtid = Ufe::RunTimeMgr::instance().register_(
		kUSDRunTimeName, usdHierHandler, usdTrans3dHandler, usdSceneHandler);
#if !defined(NDEBUG)
	assert(g_USDRtid != 0);
#endif
	if (g_USDRtid == 0)
		return MS::kFailure;

	g_StagesSubject = StagesSubject::create();

	return MS::kSuccess;
}

MStatus finalize()
{
	// Restore the normal Maya hierarchy handler, and unregister.
	Ufe::RunTimeMgr::instance().setHierarchyHandler(g_MayaRtid, g_MayaHierarchyHandler);
	Ufe::RunTimeMgr::instance().unregister(g_USDRtid);
	g_MayaHierarchyHandler.reset();

	g_StagesSubject.Reset();

	return MS::kSuccess;
}

} // namespace ufe
} // namespace MayaUsd
