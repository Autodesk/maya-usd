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

#include "private/UfeNotifGuard.h"

#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/ProxyShapeHierarchyHandler.h>
#include <mayaUsd/ufe/StagesSubject.h>
#include <mayaUsd/ufe/UfeVersionCompat.h>
#include <mayaUsd/ufe/UsdHierarchyHandler.h>
#include <mayaUsd/ufe/UsdSceneItemOpsHandler.h>
#include <mayaUsd/ufe/UsdTransform3dHandler.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/ProxyShapeContextOpsHandler.h>
#include <mayaUsd/ufe/UsdAttributesHandler.h>
#include <mayaUsd/ufe/UsdContextOpsHandler.h>
#include <mayaUsd/ufe/UsdObject3dHandler.h>
#include <mayaUsd/ufe/UsdUIInfoHandler.h>
#endif

#if UFE_PREVIEW_VERSION_NUM >= 2031
#include <mayaUsd/ufe/UsdCameraHandler.h>
#endif

#include <ufe/hierarchyHandler.h>
#include <ufe/runTimeMgr.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/pathString.h>
#endif

#include <cassert>
#include <string>

namespace {
int gRegistrationCount = 0;
}

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

// Maya's UFE run-time name and ID.
static const std::string kMayaRunTimeName("Maya-DG");
Ufe::Rtid                g_MayaRtid = 0;

// Register this run-time with UFE under the following name.
static const std::string kUSDRunTimeName("USD");

// Our run-time ID, allocated by UFE at registration time.  Initialize it
// with illegal 0 value.
Ufe::Rtid g_USDRtid = 0;

// The normal Maya hierarchy handler, which we decorate for ProxyShape support.
// Keep a reference to it to restore on finalization.
Ufe::HierarchyHandler::Ptr g_MayaHierarchyHandler;

#ifdef UFE_V2_FEATURES_AVAILABLE
// The normal Maya context ops handler, which we decorate for ProxyShape support.
// Keep a reference to it to restore on finalization.
Ufe::ContextOpsHandler::Ptr g_MayaContextOpsHandler;
#endif

// Subject singleton for observation of all USD stages.
StagesSubject::Ptr g_StagesSubject;

bool InPathChange::inGuard = false;
bool InAddOrDeleteOperation::inGuard = false;

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
    auto proxyShapeHierHandler = ProxyShapeHierarchyHandler::create(g_MayaHierarchyHandler);
    Ufe::RunTimeMgr::instance().setHierarchyHandler(g_MayaRtid, proxyShapeHierHandler);

#ifdef UFE_V2_FEATURES_AVAILABLE
    g_MayaContextOpsHandler = Ufe::RunTimeMgr::instance().contextOpsHandler(g_MayaRtid);
    auto proxyShapeContextOpsHandler = ProxyShapeContextOpsHandler::create(g_MayaContextOpsHandler);
    Ufe::RunTimeMgr::instance().setContextOpsHandler(g_MayaRtid, proxyShapeContextOpsHandler);
#endif

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::RunTimeMgr::Handlers handlers;
    handlers.hierarchyHandler = UsdHierarchyHandler::create();
    handlers.transform3dHandler = UsdTransform3dHandler::create();
    handlers.sceneItemOpsHandler = UsdSceneItemOpsHandler::create();
    handlers.attributesHandler = UsdAttributesHandler::create();
    handlers.object3dHandler = UsdObject3dHandler::create();
    handlers.contextOpsHandler = UsdContextOpsHandler::create();
    handlers.uiInfoHandler = UsdUIInfoHandler::create();
#if UFE_PREVIEW_VERSION_NUM >= 2031
    handlers.cameraHandler = UsdCameraHandler::create();
#endif
    g_USDRtid = Ufe::RunTimeMgr::instance().register_(kUSDRunTimeName, handlers);
#else
    auto usdHierHandler = UsdHierarchyHandler::create();
    auto usdTrans3dHandler = UsdTransform3dHandler::create();
    auto usdSceneItemOpsHandler = UsdSceneItemOpsHandler::create();
    g_USDRtid = Ufe::RunTimeMgr::instance().register_(
        kUSDRunTimeName, usdHierHandler, usdTrans3dHandler, usdSceneItemOpsHandler);
#endif

#if !defined(NDEBUG)
    assert(g_USDRtid != 0);
#endif
    if (g_USDRtid == 0)
        return MS::kFailure;

    g_StagesSubject = StagesSubject::create();

    // Register for UFE string to path service using path component separator '/'
    UFE_V2(Ufe::PathString::registerPathComponentSeparator(g_USDRtid, '/');)

    return MS::kSuccess;
}

MStatus finalize()
{
    // If more than one plugin still has us registered, do nothing.
    if (gRegistrationCount-- > 1)
        return MS::kSuccess;

    // Restore the normal Maya hierarchy handler, and unregister.
    Ufe::RunTimeMgr::instance().setHierarchyHandler(g_MayaRtid, g_MayaHierarchyHandler);
#ifdef UFE_V2_FEATURES_AVAILABLE
    // Restore the normal Maya context ops handler (can be empty).
    if (g_MayaContextOpsHandler)
        Ufe::RunTimeMgr::instance().setContextOpsHandler(g_MayaRtid, g_MayaContextOpsHandler);
    g_MayaContextOpsHandler.reset();
#endif
    Ufe::RunTimeMgr::instance().unregister(g_USDRtid);
    g_MayaHierarchyHandler.reset();

    g_StagesSubject.Reset();

    return MS::kSuccess;
}

Ufe::Rtid getUsdRunTimeId() { return g_USDRtid; }

Ufe::Rtid getMayaRunTimeId() { return g_MayaRtid; }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
