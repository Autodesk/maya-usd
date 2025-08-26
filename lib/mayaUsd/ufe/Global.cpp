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

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/nodes/proxyShapeStageExtraData.h>
#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>
#include <mayaUsd/ufe/MayaStagesSubject.h>
#include <mayaUsd/ufe/MayaUsdContextOpsHandler.h>
#include <mayaUsd/ufe/MayaUsdObject3dHandler.h>
#include <mayaUsd/ufe/MayaUsdSceneItemOpsHandler.h>
#include <mayaUsd/ufe/MayaUsdUIInfoHandler.h>
#include <mayaUsd/ufe/ProxyShapeContextOpsHandler.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/ProxyShapeHierarchyHandler.h>
#include <mayaUsd/ufe/UsdUIUfeObserver.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/ufe/trf/UsdTransform3dFallbackMayaXformStack.h>
#include <mayaUsd/ufe/trf/UsdTransform3dMayaXformStack.h>
#include <mayaUsd/ufe/trf/XformOpUtils.h>

#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/trf/UsdTransform3dCommonAPI.h>
#include <usdUfe/ufe/trf/UsdTransform3dMatrixOp.h>
#include <usdUfe/ufe/trf/UsdTransform3dPointInstance.h>

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

#if UFE_LIGHTS2_SUPPORT
#include <mayaUsd/ufe/UsdLight2Handler.h>
#endif

#if UFE_MATERIALS_SUPPORT
#include <mayaUsd/ufe/UsdMaterialHandler.h>
#endif

#if defined(UFE_V4_FEATURES_AVAILABLE) || (UFE_MAJOR_VERSION == 3 && UFE_CAMERAHANDLER_HAS_FINDALL)
#include <mayaUsd/ufe/ProxyShapeCameraHandler.h>
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdConnectionHandler.h>
#include <mayaUsd/ufe/UsdShaderNodeDefHandler.h>
#include <mayaUsd/ufe/UsdUINodeGraphNodeHandler.h>

#include <usdUfe/ufe/trf/UsdTransform3dRead.h>

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

#if UFE_CLIPBOARD_SUPPORT
#include <mayaUsd/utils/utilSerialization.h>

#include <usdUfe/ufe/UsdClipboardHandler.h>
#endif

#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>
#include <ufe/hierarchyHandler.h>
#include <ufe/pathString.h>
#include <ufe/runTimeMgr.h>

#include <cassert>
#include <cstdlib>
#include <string>
#if UFE_CLIPBOARD_SUPPORT
#include <ghc/filesystem.hpp>
#endif

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

void mayaStartWaitCursor()
{
    ProxyRenderDelegate::setLongDurationRendering();
    MGlobal::executeCommand("waitCursor -state 1");
}

void mayaStopWaitCursor() { MGlobal::executeCommand("waitCursor -state 0"); }

// Note: MayaUsd::ufe::getStage takes two parameters, so wrap it in a function taking only one.
PXR_NS::UsdStageWeakPtr mayaGetStage(const Ufe::Path& path) { return MayaUsd::ufe::getStage(path); }

// Wrapped to return std::string from static function returning const std::string.
std::string defaultMaterialsScopeName()
{
    return UsdMayaJobExportArgs::GetDefaultMaterialsScopeName();
}

const char* getTransform3dMatrixOpName() { return std::getenv("MAYA_USD_MATRIX_XFORM_OP_NAME"); }

void displayInfoMessage(const std::string& msg) { MGlobal::displayInfo(msg.c_str()); }
void displayWarningMessage(const std::string& msg) { MGlobal::displayWarning(msg.c_str()); }
void displayErrorMessage(const std::string& msg) { MGlobal::displayError(msg.c_str()); }

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
    dccFunctions.stageAccessorFn = mayaGetStage;
    dccFunctions.stagePathAccessorFn = MayaUsd::ufe::stagePath;
    dccFunctions.ufePathToPrimFn = MayaUsd::ufe::ufePathToPrim;
    dccFunctions.timeAccessorFn = MayaUsd::ufe::getTime;
    dccFunctions.saveStageLoadRulesFn = MayaUsd::MayaUsdProxyShapeStageExtraData::saveLoadRules;
    dccFunctions.uniqueChildNameFn = MayaUsd::ufe::uniqueChildNameMayaStandard;
    dccFunctions.displayMessageFn[static_cast<int>(UsdUfe::MessageType::kInfo)]
        = displayInfoMessage;
    dccFunctions.displayMessageFn[static_cast<int>(UsdUfe::MessageType::kWarning)]
        = displayWarningMessage;
    dccFunctions.displayMessageFn[static_cast<int>(UsdUfe::MessageType::kError)]
        = displayErrorMessage;
    dccFunctions.startWaitCursorFn = mayaStartWaitCursor;
    dccFunctions.stopWaitCursorFn = mayaStopWaitCursor;
    dccFunctions.defaultMaterialScopeNameFn = defaultMaterialsScopeName;
    dccFunctions.extractTRSFn = MayaUsd::ufe::extractTRS;
    dccFunctions.transform3dMatrixOpNameFn = getTransform3dMatrixOpName;

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
    usdUfeHandlers.sceneItemOpsHandler = MayaUsdSceneItemOpsHandler::create();
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

#if defined(UFE_V4_FEATURES_AVAILABLE) || (UFE_MAJOR_VERSION == 3 && UFE_CAMERAHANDLER_HAS_FINDALL)
    // set up the ProxyShapeCameraHandler
    g_MayaCameraHandler = runTimeMgr.cameraHandler(g_MayaRtid);
    auto proxyShapeCameraHandler = ProxyShapeCameraHandler::create(g_MayaCameraHandler);
    runTimeMgr.setCameraHandler(g_MayaRtid, proxyShapeCameraHandler);
#endif

    // Note: see UsdUfe::initialize() method lib/usdUfe/ufe/Global.cpp
    //
    // In MayaUsd we add in two extra Maya transform handlers:
    //
    // - Perform operations on a Maya transform stack appended to the existing
    //   transform stack (fallback).
    // - Perform operations on a 4x4 matrix transform op.
    // - Perform operations using the USD common transform API.
    // - Perform operations using a Maya transform stack.
    // - If the object is a point instance, use the point instance handler.
    Ufe::Transform3dHandler::Ptr lastHandler
        = MayaUsd::ufe::UsdTransform3dFallbackMayaXformStackHandler::create();
    lastHandler = UsdUfe::UsdTransform3dMatrixOpHandler::create(lastHandler);
    lastHandler = UsdUfe::UsdTransform3dCommonAPIHandler::create(lastHandler);
    lastHandler = MayaUsd::ufe::UsdTransform3dMayaXformStackHandler::create(lastHandler);
    lastHandler = UsdUfe::UsdTransform3dPointInstanceHandler::create(lastHandler);
#ifdef UFE_V4_FEATURES_AVAILABLE
    lastHandler = UsdUfe::UsdTransform3dReadHandler::create(lastHandler);
#endif
    usdUfeHandlers.transform3dHandler = lastHandler;

    // Initialize UsdUfe which will register all the default handlers
    // and the overrides we provide.
    // Subject singleton for observation of all USD stages.
    g_StagesSubject = MayaStagesSubject::create();
    auto usdRtid = UsdUfe::initialize(dccFunctions, usdUfeHandlers, g_StagesSubject);

    // Can only call Ufe::RunTimeMgr::register_() once for a given runtime name.
    // UsdUfe does that (in the call to initialize), so we must individually
    // register all the other handlers.
    if (handlers.sceneItemOpsHandler)
        runTimeMgr.setSceneItemOpsHandler(usdRtid, handlers.sceneItemOpsHandler);
    if (handlers.object3dHandler)
        runTimeMgr.setObject3dHandler(usdRtid, handlers.object3dHandler);
    if (handlers.contextOpsHandler)
        runTimeMgr.setContextOpsHandler(usdRtid, handlers.contextOpsHandler);
    if (handlers.uiInfoHandler)
        runTimeMgr.setUIInfoHandler(usdRtid, handlers.uiInfoHandler);
#ifdef UFE_V4_FEATURES_AVAILABLE
    if (handlers.lightHandler)
        runTimeMgr.setLightHandler(usdRtid, handlers.lightHandler);
#if UFE_LIGHTS2_SUPPORT
    auto light2Handler = UsdLight2Handler::create();
    if (light2Handler)
        runTimeMgr.setLight2Handler(usdRtid, light2Handler);
#endif
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

#if UFE_CLIPBOARD_SUPPORT
    // Get the clipboard handler registered by UsdUfe and set a MayaUsd clipboard path.
    auto clipboardHandler = std::dynamic_pointer_cast<UsdUfe::UsdClipboardHandler>(
        runTimeMgr.clipboardHandler(usdRtid));
    if (clipboardHandler) {
        auto clipboardFilePath = ghc::filesystem::temp_directory_path();
        clipboardFilePath.append("MayaUsdClipboard.usd");
        clipboardHandler->setClipboardFilePath(clipboardFilePath.string());
        clipboardHandler->setClipboardFileFormat(MayaUsd::utils::usdFormatArgOption());
    }
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

    UsdUfe::clearAllEditRouters();

    MMessage::removeCallback(gExitingCbId);

    return MS::kSuccess;
}

Ufe::Rtid getMayaRunTimeId() { return g_MayaRtid; }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
