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

#include <usdUfe/ufe/UsdAttributesHandler.h>
#include <usdUfe/ufe/UsdCameraHandler.h>
#include <usdUfe/ufe/UsdContextOpsHandler.h>
#include <usdUfe/ufe/UsdHierarchyHandler.h>
#include <usdUfe/ufe/UsdObject3dHandler.h>
#include <usdUfe/ufe/UsdSceneItemOpsHandler.h>
#include <usdUfe/ufe/UsdUIInfoHandler.h>
#include <usdUfe/ufe/trf/UsdTransform3dCommonAPI.h>
#include <usdUfe/ufe/trf/UsdTransform3dMatrixOp.h>
#include <usdUfe/ufe/trf/UsdTransform3dPointInstance.h>

#include <pxr/base/tf/diagnostic.h>

#include <ufe/runTimeMgr.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <usdUfe/ufe/trf/UsdTransform3dRead.h>
#endif

#if UFE_CLIPBOARD_SUPPORT
#include <usdUfe/ufe/UsdClipboardHandler.h>
#endif

#ifndef UFE_V2_FEATURES_AVAILABLE
#error "Compiling usdUfe library with Ufe v1 is no longer supported."
#endif

namespace {
int gRegistrationCount = 0;
}

namespace USDUFE_NS_DEF {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

// Register this run-time with UFE under the following name.
static const std::string kUSDRunTimeName("USD");

// Our run-time ID, allocated by UFE at registration time.  Initialize it
// with illegal 0 value.
Ufe::Rtid g_USDRtid = 0;

// Subject singleton for observation of all USD stages.
StagesSubject::RefPtr g_DefaultStagesSubject;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

Ufe::Rtid initialize(
    const DCCFunctions& dccFunctions,
    const Handlers&     handlers,
    StagesSubject::Ptr  ss /*= nullptr*/)
{
    PXR_NAMESPACE_USING_DIRECTIVE

    // If we're already registered, do nothing.
    if (gRegistrationCount++ > 0)
        return g_USDRtid;

    // Set the DCC specific functions required for the UsdUfe plugin to work.
    UsdUfe::setStageAccessorFn(dccFunctions.stageAccessorFn);
    UsdUfe::setStagePathAccessorFn(dccFunctions.stagePathAccessorFn);
    UsdUfe::setUfePathToPrimFn(dccFunctions.ufePathToPrimFn);
    UsdUfe::setTimeAccessorFn(dccFunctions.timeAccessorFn);
    UsdUfe::setWaitCursorFns(dccFunctions.startWaitCursorFn, dccFunctions.stopWaitCursorFn);

    // Optional DCC specific functions.
    if (dccFunctions.isAttributeLockedFn)
        UsdUfe::setIsAttributeLockedFn(dccFunctions.isAttributeLockedFn);
    if (dccFunctions.saveStageLoadRulesFn)
        UsdUfe::setSaveStageLoadRulesFn(dccFunctions.saveStageLoadRulesFn);
    if (dccFunctions.isRootChildFn)
        UsdUfe::setIsRootChildFn(dccFunctions.isRootChildFn);
    if (dccFunctions.uniqueChildNameFn)
        UsdUfe::setUniqueChildNameFn(dccFunctions.uniqueChildNameFn);
    if (dccFunctions.defaultMaterialScopeNameFn)
        UsdUfe::setDefaultMaterialScopeNameFn(dccFunctions.defaultMaterialScopeNameFn);
    UsdUfe::setDisplayMessageFn(dccFunctions.displayMessageFn);
    if (dccFunctions.extractTRSFn)
        UsdUfe::setExtractTRSFn(dccFunctions.extractTRSFn);
    if (dccFunctions.transform3dMatrixOpNameFn)
        UsdUfe::setTransform3dMatrixOpNameFn(dccFunctions.transform3dMatrixOpNameFn);

    // Create a default stages subject if none is provided.
    if (nullptr == ss) {
        g_DefaultStagesSubject = StagesSubject::create();
    }

    // Copy all the input handlers into the Ufe handler struct and
    // create any default ones which are null.
    Ufe::RunTimeMgr::Handlers rtHandlers;
    rtHandlers.attributesHandler
        = handlers.attributesHandler ? handlers.attributesHandler : UsdAttributesHandler::create();
    rtHandlers.hierarchyHandler
        = handlers.hierarchyHandler ? handlers.hierarchyHandler : UsdHierarchyHandler::create();
    rtHandlers.sceneItemOpsHandler = handlers.sceneItemOpsHandler
        ? handlers.sceneItemOpsHandler
        : UsdSceneItemOpsHandler::create();
    rtHandlers.object3dHandler
        = handlers.object3dHandler ? handlers.object3dHandler : UsdObject3dHandler::create();
    rtHandlers.contextOpsHandler
        = handlers.contextOpsHandler ? handlers.contextOpsHandler : UsdContextOpsHandler::create();
    rtHandlers.uiInfoHandler
        = handlers.uiInfoHandler ? handlers.uiInfoHandler : UsdUIInfoHandler::create();
    rtHandlers.cameraHandler
        = handlers.cameraHandler ? handlers.cameraHandler : UsdCameraHandler::create();

    if (handlers.transform3dHandler) {
        rtHandlers.transform3dHandler = handlers.transform3dHandler;
    } else {
        // USD has a very flexible data model to support 3d transformations --- see
        // https://graphics.pixar.com/usd/docs/api/class_usd_geom_xformable.html
        //
        // To map this flexibility into a UFE Transform3d handler, we set up a
        // chain of responsibility
        // https://en.wikipedia.org/wiki/Chain-of-responsibility_pattern
        // for Transform3d interface creation, from least important to most
        // important:
        // - Perform operations on a 4x4 matrix transform op.
        // - Perform operations using the USD common transform API.
        // - If the object is a point instance, use the point instance handler.
        Ufe::Transform3dHandler::Ptr lastHandler = UsdTransform3dMatrixOpHandler::create(nullptr);
        lastHandler = UsdTransform3dCommonAPIHandler::create(lastHandler);
        lastHandler = UsdTransform3dPointInstanceHandler::create(lastHandler);
#ifdef UFE_V4_FEATURES_AVAILABLE
        lastHandler = UsdTransform3dReadHandler::create(lastHandler);
#endif

        // Finally set the last transform handler we created into Ufe.
        rtHandlers.transform3dHandler = lastHandler;
    }

    g_USDRtid = Ufe::RunTimeMgr::instance().register_(kUSDRunTimeName, rtHandlers);
    TF_VERIFY(g_USDRtid != 0);

    // Handlers to register separately since they may or may not be contained within
    // the Ufe Handlers struct. But they are always guaranteed to have a set method.
#if UFE_CLIPBOARD_SUPPORT
    auto cbhndlr
        = handlers.clipboardHandler ? handlers.clipboardHandler : UsdClipboardHandler::create();
    Ufe::RunTimeMgr::instance().setClipboardHandler(g_USDRtid, cbhndlr);
#endif

    return g_USDRtid;
}

bool finalize(bool exiting)
{
    // If more than one plugin still has us registered, do nothing.
    if (gRegistrationCount-- > 1 && !exiting)
        return true;

    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    runTimeMgr.unregister(g_USDRtid);

    // If we created the default stages subject we must destroy it.
    g_DefaultStagesSubject.Reset();

    return true;
}

std::string getUsdRunTimeName() { return kUSDRunTimeName; }

Ufe::Rtid getUsdRunTimeId() { return g_USDRtid; }

//! Connect a stage to USD notifications.
USDUFE_PUBLIC
TfNotice::Key registerStage(const PXR_NS::UsdStageRefPtr& stage)
{
    return g_DefaultStagesSubject->registerStage(stage);
}

} // namespace USDUFE_NS_DEF
