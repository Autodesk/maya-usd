//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#include "LookdevXUsd.h"

#ifdef LDX_USD_USE_MAYAUSDAPI
#include "ProxyShapeLookdevHandler.h"
#endif
#include "UsdCapabilityHandler.h"
#include "UsdClipboardHandler.h"
#include "UsdDebugHandler.h"
#include "UsdExtendedAttributeHandler.h"
#include "UsdFileHandler.h"
#include "UsdHierarchyHandler.h"
#include "UsdLookdevHandler.h"
#include "UsdMaterialHandler.h"
#include "UsdSceneItemUIHandler.h"
#include "UsdSceneItemOpsHandler.h"
#include "UsdSoloingHandler.h"
#include "UsdUINodeGraphNodeHandler.h"

#include <usdUfe/ufe/Global.h>

#include <ufe/handlerInterface.h>
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>

#include <pxr/usd/sdr/registry.h>

#ifdef LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER
#include "UsdExtendedConnectionHandler.h"
#endif
#ifdef LDX_USD_USE_MAYAUSDAPI
#include <mayaUsdAPI/utils.h>
#endif

namespace
{

#ifdef LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER
Ufe::SceneItemOpsHandler::Ptr mayaUsdSceneItemOpsHandler;
#endif
#ifdef LDX_USD_USE_MAYAUSDAPI
LookdevXUfe::LookdevHandler::Ptr mayaLookdevHandler;
#endif

} // namespace

namespace LookdevXUsd
{

void initialize()
{
    auto & runTimeMgr = Ufe::RunTimeMgr::instance();

    // New extension handlers.
    const auto usdRuntimeId = UsdUfe::getUsdRunTimeId();
    runTimeMgr.registerHandler(usdRuntimeId, UsdDebugHandler::id, UsdDebugHandler::create());
    runTimeMgr.registerHandler(usdRuntimeId, UsdLookdevHandler::id, UsdLookdevHandler::create());
    runTimeMgr.registerHandler(usdRuntimeId, UsdMaterialHandler::id, UsdMaterialHandler::create());
    runTimeMgr.registerHandler(usdRuntimeId, UsdSoloingHandler::id, UsdSoloingHandler::create());
    runTimeMgr.registerHandler(usdRuntimeId, UsdFileHandler::id, UsdFileHandler::create());
    runTimeMgr.registerHandler(usdRuntimeId, UsdSceneItemUIHandler::id,
                                             UsdSceneItemUIHandler::create());
    runTimeMgr.registerHandler(usdRuntimeId, UsdExtendedAttributeHandler::id,
                                             UsdExtendedAttributeHandler::create());
    runTimeMgr.registerHandler(usdRuntimeId, UsdCapabilityHandler::id,
                                             UsdCapabilityHandler::create());
#ifdef LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER
    runTimeMgr.registerHandler(usdRuntimeId, UsdExtendedConnectionHandler::id,
                                             UsdExtendedConnectionHandler::create());
#endif

    // Replacements/wrappers for existing handlers.
    UsdUINodeGraphNodeHandler::registerHandler(usdRuntimeId);
    UsdHierarchyHandler::registerHandler(usdRuntimeId);
    UsdClipboardHandler::registerHandler(usdRuntimeId);

#ifdef LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER
    mayaUsdSceneItemOpsHandler = runTimeMgr.sceneItemOpsHandler(usdRuntimeId);
    runTimeMgr.setSceneItemOpsHandler(
        usdRuntimeId, LookdevXUsd::UsdSceneItemOpsHandler::create(mayaUsdSceneItemOpsHandler));
#endif

#ifdef LDX_USD_USE_MAYAUSDAPI
    auto mayaRuntimeId = MayaUsdAPI::getMayaRunTimeId();
    mayaLookdevHandler = LookdevXUfe::LookdevHandler::get(mayaRuntimeId);
    runTimeMgr.registerHandler(mayaRuntimeId, ProxyShapeLookdevHandler::id,
        ProxyShapeLookdevHandler::create(mayaLookdevHandler));
#endif

    // Force loading the Sdr library to preload the source of the NodeLibrary on the USD side. This will load the Arnold
    // DLL if it is in the USD paths and initialize it for its nodes, which should result in a slight delay.
    //
    // Hopefully this will fix:
    //   - LOOKDEVX-2609: Module error when saving and reopening a scene file with USD data
    //   - The long library load wait when creating the first USD material or opening LookdevX on a USD tab
    //
    // Without re-causing:
    //   - LOOKDEVX-871: Arnold's Library isn't loaded while autoloading both LookdevX and Bifrost
    //      (should stay fixed since MAYA-130935 ensures we load LookdevX last in the bundle)
    //
    PXR_NS::SdrRegistry::GetInstance();
}

void uninitialize()
{
    // New extension handlers.
    auto & runTimeMgr = Ufe::RunTimeMgr::instance();
    const auto usdRuntimeId = UsdUfe::getUsdRunTimeId();
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdDebugHandler::id);
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdLookdevHandler::id);
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdMaterialHandler::id);
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdSoloingHandler::id);
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdFileHandler::id);
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdSceneItemUIHandler::id);
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdExtendedAttributeHandler::id);
#ifdef LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdExtendedConnectionHandler::id);
#endif
    runTimeMgr.unregisterHandler(usdRuntimeId, UsdCapabilityHandler::id);

    // Replacements/wrappers for existing handlers
    UsdUINodeGraphNodeHandler::unregisterHandler();
    UsdHierarchyHandler::unregisterHandler();
    UsdClipboardHandler::unregisterHandler();

    // Unregister the decorated MayaUsd handlers and restore the original ones.
#ifdef LOOKDEVXUFE_HAS_EXTENDED_CONNECTION_HANDLER
    if (runTimeMgr.hasId(usdRuntimeId)) {
        runTimeMgr.setSceneItemOpsHandler(usdRuntimeId, mayaUsdSceneItemOpsHandler);
    }
    mayaUsdSceneItemOpsHandler.reset();
#endif

#ifdef LDX_USD_USE_MAYAUSDAPI
    // Unregister the decorated Maya handlers and restore the original ones.
    auto mayaRuntimeId = MayaUsdAPI::getMayaRunTimeId();
    runTimeMgr.unregisterHandler(mayaRuntimeId, ProxyShapeLookdevHandler::id);
    if (runTimeMgr.hasId(mayaRuntimeId) && mayaLookdevHandler) {
        runTimeMgr.registerHandler(mayaRuntimeId, ProxyShapeLookdevHandler::id, mayaLookdevHandler);
    }
    mayaLookdevHandler.reset();
#endif
}

} // namespace LookdevXUsd
