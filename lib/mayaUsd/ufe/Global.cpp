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
#include <mayaUsd/ufe/UsdCameraHandler.h>
#include <mayaUsd/ufe/UsdContextOpsHandler.h>
#include <mayaUsd/ufe/UsdObject3dHandler.h>
#include <mayaUsd/ufe/UsdTransform3dCommonAPI.h>
#include <mayaUsd/ufe/UsdTransform3dFallbackMayaXformStack.h>
#include <mayaUsd/ufe/UsdTransform3dMatrixOp.h>
#include <mayaUsd/ufe/UsdTransform3dMayaXformStack.h>
#include <mayaUsd/ufe/UsdTransform3dPointInstance.h>
#include <mayaUsd/ufe/UsdUIInfoHandler.h>
#include <mayaUsd/ufe/UsdUIUfeObserver.h>
#endif
#ifdef UFE_V3_FEATURES_AVAILABLE
#define HAVE_PATH_MAPPING
#include <mayaUsd/ufe/MayaUIInfoHandler.h>
#include <mayaUsd/ufe/PulledObjectHierarchyHandler.h>
#include <mayaUsd/ufe/UsdPathMappingHandler.h>
#endif
#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4007)
#include <mayaUsd/ufe/UsdLightHandler.h>
#endif
#if (UFE_PREVIEW_VERSION_NUM >= 4001)
#include <mayaUsd/ufe/UsdShaderNodeDefHandler.h>
#endif
#endif
#include <mayaUsd/utils/editRouter.h>

#include <maya/MSceneMessage.h>
#include <ufe/hierarchyHandler.h>
#include <ufe/runTimeMgr.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/pathString.h>
#endif

#include <cassert>
#include <string>

namespace {

void exitingCallback(void* /* unusedData */)
{
    // Maya does not unload plugins on exit.  Make sure we perform an orderly
    // cleanup, otherwise on program exit UFE static data structures may be
    // cleaned up when this plugin is no longer alive.
    MayaUsd::ufe::finalize(/* exiting = */ true);
}

int gRegistrationCount = 0;

MCallbackId gExitingCbId = 0;
} // namespace

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

#ifdef HAVE_PATH_MAPPING
Ufe::PathMappingHandler::Ptr g_MayaPathMappingHandler;

Ufe::UIInfoHandler::Ptr g_MayaUIInfoHandler;
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
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    g_MayaRtid = runTimeMgr.getId(kMayaRunTimeName);
#if !defined(NDEBUG)
    assert(g_MayaRtid != 0);
#endif
    if (g_MayaRtid == 0)
        return MS::kFailure;

    g_MayaHierarchyHandler = runTimeMgr.hierarchyHandler(g_MayaRtid);
    auto proxyShapeHierHandler = ProxyShapeHierarchyHandler::create(g_MayaHierarchyHandler);
#ifdef UFE_V3_FEATURES_AVAILABLE
    auto pulledObjectHierHandler = PulledObjectHierarchyHandler::create(proxyShapeHierHandler);
    runTimeMgr.setHierarchyHandler(g_MayaRtid, pulledObjectHierHandler);
#else
    runTimeMgr.setHierarchyHandler(g_MayaRtid, proxyShapeHierHandler);
#endif

#ifdef UFE_V2_FEATURES_AVAILABLE
    g_MayaContextOpsHandler = runTimeMgr.contextOpsHandler(g_MayaRtid);
    auto proxyShapeContextOpsHandler = ProxyShapeContextOpsHandler::create(g_MayaContextOpsHandler);
    runTimeMgr.setContextOpsHandler(g_MayaRtid, proxyShapeContextOpsHandler);
#endif

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::RunTimeMgr::Handlers handlers;
    handlers.hierarchyHandler = UsdHierarchyHandler::create();
    handlers.sceneItemOpsHandler = UsdSceneItemOpsHandler::create();
    handlers.attributesHandler = UsdAttributesHandler::create();
    handlers.object3dHandler = UsdObject3dHandler::create();
    handlers.contextOpsHandler = UsdContextOpsHandler::create();
    handlers.uiInfoHandler = UsdUIInfoHandler::create();
    handlers.cameraHandler = UsdCameraHandler::create();
#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4007)
    handlers.lightHandler = UsdLightHandler::create();
#endif
#if (UFE_PREVIEW_VERSION_NUM >= 4001)
    handlers.nodeDefHandler = UsdShaderNodeDefHandler::create();
#endif
#endif

    // USD has a very flexible data model to support 3d transformations --- see
    // https://graphics.pixar.com/usd/docs/api/class_usd_geom_xformable.html
    //
    // To map this flexibility into a UFE Transform3d handler, we set up a
    // chain of responsibility
    // https://en.wikipedia.org/wiki/Chain-of-responsibility_pattern
    // for Transform3d interface creation, from least important to most
    // important:
    // - Perform operations on a Maya transform stack appended to the existing
    //   transform stack (fallback).
    // - Perform operations on a 4x4 matrix transform op.
    // - Perform operations using the USD common transform API.
    // - Perform operations using a Maya transform stack.
    // - If the object is a point instance, use the point instance handler.
    auto fallbackHandler = MayaUsd::ufe::UsdTransform3dFallbackMayaXformStackHandler::create();
    auto matrixHandler = MayaUsd::ufe::UsdTransform3dMatrixOpHandler::create(fallbackHandler);
    auto commonAPIHandler = MayaUsd::ufe::UsdTransform3dCommonAPIHandler::create(matrixHandler);
    auto mayaStackHandler
        = MayaUsd::ufe::UsdTransform3dMayaXformStackHandler::create(commonAPIHandler);
    auto pointInstanceHandler
        = MayaUsd::ufe::UsdTransform3dPointInstanceHandler::create(mayaStackHandler);

    handlers.transform3dHandler = pointInstanceHandler;

    g_USDRtid = runTimeMgr.register_(kUSDRunTimeName, handlers);
    MayaUsd::ufe::UsdUIUfeObserver::create();

#ifdef HAVE_PATH_MAPPING
    g_MayaPathMappingHandler = runTimeMgr.pathMappingHandler(g_MayaRtid);
    auto pathMappingHndlr = UsdPathMappingHandler::create();
    runTimeMgr.setPathMappingHandler(g_MayaRtid, pathMappingHndlr);

    // Replace any existing info handler with our own.
    g_MayaUIInfoHandler = runTimeMgr.uiInfoHandler(g_MayaRtid);
    auto uiInfoHandler = MayaUIInfoHandler::create();
    runTimeMgr.setUIInfoHandler(g_MayaRtid, uiInfoHandler);
#endif

#else
    auto usdHierHandler = UsdHierarchyHandler::create();
    auto usdTrans3dHandler = UsdTransform3dHandler::create();
    auto usdSceneItemOpsHandler = UsdSceneItemOpsHandler::create();
    g_USDRtid = runTimeMgr.register_(
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

    // Initialize edit router registry with default routers.
    auto defaults = defaultEditRouters();
    for (const auto& entry : defaults) {
        registerEditRouter(entry.first, entry.second);
    }

    gExitingCbId = MSceneMessage::addCallback(MSceneMessage::kMayaExiting, exitingCallback);

    return MS::kSuccess;
}

MStatus finalize(bool exiting)
{
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();

    // If more than one plugin still has us registered, do nothing.
    if (gRegistrationCount-- > 1 && !exiting)
        return MS::kSuccess;

    // Restore the normal Maya hierarchy handler, and unregister.
    runTimeMgr.setHierarchyHandler(g_MayaRtid, g_MayaHierarchyHandler);
#ifdef UFE_V2_FEATURES_AVAILABLE
    // Restore the normal Maya context ops handler (can be empty).
    if (g_MayaContextOpsHandler)
        runTimeMgr.setContextOpsHandler(g_MayaRtid, g_MayaContextOpsHandler);
    g_MayaContextOpsHandler.reset();

    MayaUsd::ufe::UsdUIUfeObserver::destroy();
#endif
    runTimeMgr.unregister(g_USDRtid);
    g_MayaHierarchyHandler.reset();

#ifdef HAVE_PATH_MAPPING
    // Remove the Maya path mapping handler that we added above.
    runTimeMgr.setPathMappingHandler(g_MayaRtid, g_MayaPathMappingHandler);
    g_MayaPathMappingHandler.reset();

    // Remove the Maya path mapping handler that we added above.
    runTimeMgr.setUIInfoHandler(g_MayaRtid, g_MayaUIInfoHandler);
    g_MayaUIInfoHandler.reset();
#endif

    g_StagesSubject.Reset();

    MMessage::removeCallback(gExitingCbId);

    return MS::kSuccess;
}

Ufe::Rtid getUsdRunTimeId() { return g_USDRtid; }

Ufe::Rtid getMayaRunTimeId() { return g_MayaRtid; }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
