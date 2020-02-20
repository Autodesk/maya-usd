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

#include "Global.h"
#include "ProxyShapeHandler.h"
#include "StagesSubject.h"
#include "private/InPathChange.h"
#include "UsdHierarchyHandler.h"
#include "UsdTransform3dHandler.h"
#include "UsdSceneItemOpsHandler.h"

#include <ufe/runTimeMgr.h>
#include <ufe/hierarchyHandler.h>
#include <ufe/ProxyShapeHierarchyHandler.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
// Note: must come after include of ufe files so we have the define.
#include "UsdAttributesHandler.h"
#include "UsdObject3dHandler.h"
#if UFE_PREVIEW_VERSION_NUM >= 2009
#include "UsdContextOpsHandler.h"
#endif
#if UFE_PREVIEW_VERSION_NUM >= 2011
#include <ufe/pathString.h>
#endif
#else
#include "UfeVersionCompat.h"
#endif

#include <string>
#include <cassert>

namespace {
	int gRegistrationCount = 0;
}

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
	// If we're already registered, do nothing.
	if (gRegistrationCount++ > 0)
		return MS::kSuccess;

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
	auto usdSceneItemOpsHandler = UsdSceneItemOpsHandler::create();
	UFE_V2(auto usdAttributesHandler = UsdAttributesHandler::create();)
	UFE_V2(auto usdObject3dHandler = UsdObject3dHandler::create();)
#if UFE_PREVIEW_VERSION_NUM >= 2009
	UFE_V2(auto usdContextOpsHandler = UsdContextOpsHandler::create();)
#endif
	g_USDRtid = Ufe::RunTimeMgr::instance().register_(
		kUSDRunTimeName, usdHierHandler, usdTrans3dHandler, 
        usdSceneItemOpsHandler
        UFE_V2(, usdAttributesHandler, usdObject3dHandler)
#if UFE_PREVIEW_VERSION_NUM >= 2009
        UFE_V2(, usdContextOpsHandler)
#endif
    );

#if !defined(NDEBUG)
	assert(g_USDRtid != 0);
#endif
	if (g_USDRtid == 0)
		return MS::kFailure;

	g_StagesSubject = StagesSubject::create();

    // Register for UFE string to path service using path component separator '/'
#if UFE_PREVIEW_VERSION_NUM >= 2011
    UFE_V2(Ufe::PathString::registerPathComponentSeparator(g_USDRtid, '/');)
#endif

	return MS::kSuccess;
}

MStatus finalize()
{
	// If more than one plugin still has us registered, do nothing.
	if (gRegistrationCount-- > 1)
		return MS::kSuccess;

	// Restore the normal Maya hierarchy handler, and unregister.
	Ufe::RunTimeMgr::instance().setHierarchyHandler(g_MayaRtid, g_MayaHierarchyHandler);
	Ufe::RunTimeMgr::instance().unregister(g_USDRtid);
	g_MayaHierarchyHandler.reset();

	g_StagesSubject.Reset();

	return MS::kSuccess;
}

Ufe::Rtid getUsdRunTimeId()
{
    return g_USDRtid;
}

} // namespace ufe
} // namespace MayaUsd
