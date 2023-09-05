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

#include <mayaUsd/nodes/proxyShapeStageExtraData.h>
#include <mayaUsd/ufe/MayaStagesSubject.h>
#include <mayaUsd/ufe/MayaUsdContextOpsHandler.h>
#include <mayaUsd/ufe/MayaUsdObject3dHandler.h>
#include <mayaUsd/ufe/MayaUsdUIInfoHandler.h>
#include <mayaUsd/ufe/ProxyShapeContextOpsHandler.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/ProxyShapeHierarchyHandler.h>
#include <mayaUsd/ufe/UsdAttributesHandler.h>
#include <mayaUsd/ufe/UsdSceneItemOpsHandler.h>
#include <mayaUsd/ufe/UsdTransform3dCommonAPI.h>
#include <mayaUsd/ufe/UsdTransform3dFallbackMayaXformStack.h>
#include <mayaUsd/ufe/UsdTransform3dMatrixOp.h>
#include <mayaUsd/ufe/UsdTransform3dMayaXformStack.h>
#include <mayaUsd/ufe/UsdTransform3dPointInstance.h>
#include <mayaUsd/ufe/UsdUIUfeObserver.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/editability.h>

#ifdef UFE_V3_FEATURES_AVAILABLE
#define HAVE_PATH_MAPPING
#include <mayaUsd/ufe/MayaUIInfoHandler.h>
#include <mayaUsd/ufe/MayaUsdHierarchyHandler.h>
#include <mayaUsd/ufe/PulledObjectHierarchyHandler.h>
#include <mayaUsd/ufe/UsdPathMappingHandler.h>
#endif

#if UFE_LIGHTS_SUPPORT
#include <mayaUsd/ufe/UsdLightHandler.h>
#endif

#if UFE_MATERIALS_SUPPORT
#include <mayaUsd/ufe/UsdMaterialHandler.h>
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/ProxyShapeCameraHandler.h>
#include <mayaUsd/ufe/UsdConnectionHandler.h>
#include <mayaUsd/ufe/UsdShaderNodeDefHandler.h>
#include <mayaUsd/ufe/UsdTransform3dRead.h>
#include <mayaUsd/ufe/UsdUINodeGraphNodeHandler.h>

#if UFE_PREVIEW_BATCHOPS_SUPPORT
#include <mayaUsd/ufe/UsdBatchOpsHandler.h>
#endif

#endif

#ifdef UFE_PREVIEW_CODE_WRAPPER_HANDLER_SUPPORT
#include <mayaUsd/ufe/UsdCodeWrapperHandler.h>
#endif

#if UFE_SCENE_SEGMENT_SUPPORT
#include <mayaUsd/ufe/ProxyShapeSceneSegmentHandler.h>
#endif
#include <mayaUsd/utils/mayaEditRouter.h>

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/utils/editRouter.h>

#include <maya/MSceneMessage.h>
#include <ufe/hierarchyHandler.h>
#include <ufe/pathString.h>
#include <ufe/runTimeMgr.h>

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

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

int gRegistrationCount = 0;

MCallbackId gExitingCbId = 0;

// Subject singleton for observation of all USD stages.
MayaUsd::ufe::MayaStagesSubject::RefPtr g_StagesSubject;

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

// Maya's UFE run-time name and ID.
static const std::string kMayaRunTimeName("Maya-DG");
Ufe::Rtid                g_MayaRtid = 0;

// The normal Maya hierarchy handler, which we decorate for ProxyShape support.
// Keep a reference to it to restore on finalization.
Ufe::HierarchyHandler::Ptr g_MayaHierarchyHandler;

// The normal Maya context ops handler, which we decorate for ProxyShape support.
// Keep a reference to it to restore on finalization.
Ufe::ContextOpsHandler::Ptr g_MayaContextOpsHandler;

#if UFE_SCENE_SEGMENT_SUPPORT
// The normal Maya scene segment handler, which we decorate for ProxyShape support.
// Keep a reference to it to restore on finalization.
Ufe::SceneSegmentHandler::Ptr g_MayaSceneSegmentHandler;

// The normal Maya camera handler, which we decorate for ProxyShape support.
// Keep a reference to it to restore on finalization.
Ufe::CameraHandler::Ptr g_MayaCameraHandler;
#endif

#ifdef HAVE_PATH_MAPPING
Ufe::PathMappingHandler::Ptr g_MayaPathMappingHandler;

Ufe::UIInfoHandler::Ptr g_MayaUIInfoHandler;
#endif

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

    // Set the Maya specific functions required for the UsdUfe plugin to work correctly.
    UsdUfe::DCCFunctions dccFunctions;
    dccFunctions.stageAccessorFn = MayaUsd::ufe::getStage;
    dccFunctions.stagePathAccessorFn = MayaUsd::ufe::stagePath;
    dccFunctions.ufePathToPrimFn = MayaUsd::ufe::ufePathToPrim;
    dccFunctions.timeAccessorFn = MayaUsd::ufe::getTime;
    dccFunctions.isAttributeLockedFn = MayaUsd::Editability::isAttributeLocked;
    dccFunctions.saveStageLoadRulesFn = MayaUsd::MayaUsdProxyShapeStageExtraData::saveLoadRules;
    dccFunctions.uniqueChildNameFn = MayaUsd::ufe::uniqueChildNameMayaStandard;

    // Replace the Maya hierarchy handler with ours.
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    g_MayaRtid = runTimeMgr.getId(kMayaRunTimeName);
#if !defined(NDEBUG)
    assert(g_MayaRtid != 0);
#endif
    if (g_MayaRtid == 0)
        return MS::kFailure;

    const std::string kUSDRunTimeName = UsdUfe::getUsdRunTimeName();

    g_MayaHierarchyHandler = runTimeMgr.hierarchyHandler(g_MayaRtid);
    auto proxyShapeHierHandler = ProxyShapeHierarchyHandler::create(g_MayaHierarchyHandler);
#ifdef UFE_V3_FEATURES_AVAILABLE
    auto pulledObjectHierHandler = PulledObjectHierarchyHandler::create(proxyShapeHierHandler);
    runTimeMgr.setHierarchyHandler(g_MayaRtid, pulledObjectHierHandler);
#else
    runTimeMgr.setHierarchyHandler(g_MayaRtid, proxyShapeHierHandler);
#endif

    g_MayaContextOpsHandler = runTimeMgr.contextOpsHandler(g_MayaRtid);
    auto proxyShapeContextOpsHandler = ProxyShapeContextOpsHandler::create(g_MayaContextOpsHandler);
    runTimeMgr.setContextOpsHandler(g_MayaRtid, proxyShapeContextOpsHandler);

    UsdUfe::Handlers          usdUfeHandlers;
    Ufe::RunTimeMgr::Handlers handlers;
#ifdef UFE_V3_FEATURES_AVAILABLE
    usdUfeHandlers.hierarchyHandler = MayaUsdHierarchyHandler::create();
#endif
    handlers.sceneItemOpsHandler = UsdSceneItemOpsHandler::create();
    handlers.attributesHandler = UsdAttributesHandler::create();
    usdUfeHandlers.object3dHandler = MayaUsdObject3dHandler::create();
    usdUfeHandlers.contextOpsHandler = MayaUsdContextOpsHandler::create();
    usdUfeHandlers.uiInfoHandler = MayaUsdUIInfoHandler::create();

#ifdef UFE_V4_FEATURES_AVAILABLE

#if UFE_LIGHTS_SUPPORT
    handlers.lightHandler = UsdLightHandler::create();
#endif

#if UFE_MATERIALS_SUPPORT
    handlers.materialHandler = UsdMaterialHandler::create();
#endif
    handlers.connectionHandler = UsdConnectionHandler::create();
    handlers.uiNodeGraphNodeHandler = UsdUINodeGraphNodeHandler::create();

#ifdef UFE_PREVIEW_CODE_WRAPPER_HANDLER_SUPPORT
    handlers.batchOpsHandler = UsdCodeWrapperHandler::create();
#elif UFE_PREVIEW_BATCHOPS_SUPPORT
    handlers.batchOpsHandler = UsdBatchOpsHandler::create();
#endif

    handlers.nodeDefHandler = UsdShaderNodeDefHandler::create();

#endif /* UFE_V4_FEATURES_AVAILABLE */

#if UFE_SCENE_SEGMENT_SUPPORT
    // set up the SceneSegmentHandler
    g_MayaSceneSegmentHandler = runTimeMgr.sceneSegmentHandler(g_MayaRtid);
    auto proxyShapeSceneSegmentHandler
        = ProxyShapeSceneSegmentHandler::create(g_MayaSceneSegmentHandler);
    runTimeMgr.setSceneSegmentHandler(g_MayaRtid, proxyShapeSceneSegmentHandler);
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
    // set up the ProxyShapeCameraHandler
    g_MayaCameraHandler = runTimeMgr.cameraHandler(g_MayaRtid);
    auto proxyShapeCameraHandler = ProxyShapeCameraHandler::create(g_MayaCameraHandler);
    runTimeMgr.setCameraHandler(g_MayaRtid, proxyShapeCameraHandler);
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
    Ufe::Transform3dHandler::Ptr lastHandler
        = MayaUsd::ufe::UsdTransform3dFallbackMayaXformStackHandler::create();
    lastHandler = MayaUsd::ufe::UsdTransform3dMatrixOpHandler::create(lastHandler);
    lastHandler = MayaUsd::ufe::UsdTransform3dCommonAPIHandler::create(lastHandler);
    lastHandler = MayaUsd::ufe::UsdTransform3dMayaXformStackHandler::create(lastHandler);
    lastHandler = MayaUsd::ufe::UsdTransform3dPointInstanceHandler::create(lastHandler);
#ifdef UFE_V4_FEATURES_AVAILABLE
    lastHandler = MayaUsd::ufe::UsdTransform3dReadHandler::create(lastHandler);
#endif
    handlers.transform3dHandler = lastHandler;

    // Initialize UsdUfe which will register all the default handlers
    // and the overrides we provide.
    // Subject singleton for observation of all USD stages.
    g_StagesSubject = MayaStagesSubject::create();
    auto usdRtid = UsdUfe::initialize(dccFunctions, usdUfeHandlers, g_StagesSubject);

    // TEMP (UsdUfe)
    // Can only call Ufe::RunTimeMgr::register_() once for a given runtime name.
    // UsdUfe does that (in the call to initialize), so we must individually
    // register all the other handlers.
    if (handlers.transform3dHandler)
        runTimeMgr.setTransform3dHandler(usdRtid, handlers.transform3dHandler);
    if (handlers.sceneItemOpsHandler)
        runTimeMgr.setSceneItemOpsHandler(usdRtid, handlers.sceneItemOpsHandler);
    if (handlers.attributesHandler)
        runTimeMgr.setAttributesHandler(usdRtid, handlers.attributesHandler);
    if (handlers.object3dHandler)
        runTimeMgr.setObject3dHandler(usdRtid, handlers.object3dHandler);
    if (handlers.contextOpsHandler)
        runTimeMgr.setContextOpsHandler(usdRtid, handlers.contextOpsHandler);
    if (handlers.uiInfoHandler)
        runTimeMgr.setUIInfoHandler(usdRtid, handlers.uiInfoHandler);
#ifdef UFE_V4_FEATURES_AVAILABLE
    if (handlers.lightHandler)
        runTimeMgr.setLightHandler(usdRtid, handlers.lightHandler);
    if (handlers.materialHandler)
        runTimeMgr.setMaterialHandler(usdRtid, handlers.materialHandler);
    if (handlers.nodeDefHandler)
        runTimeMgr.setNodeDefHandler(usdRtid, handlers.nodeDefHandler);
    if (handlers.connectionHandler)
        runTimeMgr.setConnectionHandler(usdRtid, handlers.connectionHandler);
    if (handlers.uiNodeGraphNodeHandler)
        runTimeMgr.setUINodeGraphNodeHandler(usdRtid, handlers.uiNodeGraphNodeHandler);
    if (handlers.batchOpsHandler)
        runTimeMgr.setBatchOpsHandler(usdRtid, handlers.batchOpsHandler);
#endif

    MayaUsd::ufe::UsdUIUfeObserver::create();

#ifndef UFE_V4_FEATURES_AVAILABLE

#if UFE_LIGHTS_SUPPORT
    runTimeMgr.setLightHandler(usdRtid, UsdLightHandler::create());
#endif
#if UFE_MATERIALS_SUPPORT
    runTimeMgr.setMaterialHandler(usdRtid, UsdMaterialHandler::create());
#endif

#endif /* UFE_V4_FEATURES_AVAILABLE */

#ifdef HAVE_PATH_MAPPING
    g_MayaPathMappingHandler = runTimeMgr.pathMappingHandler(g_MayaRtid);
    auto pathMappingHndlr = UsdPathMappingHandler::create();
    runTimeMgr.setPathMappingHandler(g_MayaRtid, pathMappingHndlr);

    // Replace any existing info handler with our own.
    g_MayaUIInfoHandler = runTimeMgr.uiInfoHandler(g_MayaRtid);
    auto uiInfoHandler = MayaUIInfoHandler::create();
    runTimeMgr.setUIInfoHandler(g_MayaRtid, uiInfoHandler);
#endif

#if !defined(NDEBUG)
    assert(usdRtid != 0);
#endif
    if (usdRtid == 0)
        return MS::kFailure;

    // Register for UFE string to path service using path component separator '/'
    Ufe::PathString::registerPathComponentSeparator(usdRtid, '/');

    // Initialize edit router registry with default routers.
    MayaUsd::registerMayaEditRouters();
    UsdUfe::restoreAllDefaultEditRouters();

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
    // Restore the normal Maya context ops handler (can be empty).
    if (g_MayaContextOpsHandler)
        runTimeMgr.setContextOpsHandler(g_MayaRtid, g_MayaContextOpsHandler);
    g_MayaContextOpsHandler.reset();

    MayaUsd::ufe::UsdUIUfeObserver::destroy();

    UsdUfe::finalize(exiting);
    g_MayaHierarchyHandler.reset();

    // Destroy our stages subject.
    g_StagesSubject.Reset();

#if UFE_SCENE_SEGMENT_SUPPORT
    runTimeMgr.setSceneSegmentHandler(g_MayaRtid, g_MayaSceneSegmentHandler);
    g_MayaSceneSegmentHandler.reset();

    runTimeMgr.setCameraHandler(g_MayaRtid, g_MayaCameraHandler);
    g_MayaCameraHandler.reset();
#endif

#ifdef HAVE_PATH_MAPPING
    // Remove the Maya path mapping handler that we added above.
    runTimeMgr.setPathMappingHandler(g_MayaRtid, g_MayaPathMappingHandler);
    g_MayaPathMappingHandler.reset();

    // Remove the Maya path mapping handler that we added above.
    runTimeMgr.setUIInfoHandler(g_MayaRtid, g_MayaUIInfoHandler);
    g_MayaUIInfoHandler.reset();
#endif

    MMessage::removeCallback(gExitingCbId);

    return MS::kSuccess;
}

Ufe::Rtid getMayaRunTimeId() { return g_MayaRtid; }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
