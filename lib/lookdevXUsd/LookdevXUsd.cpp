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

#include "ProxyShapeLookdevHandler.h"
#include "UsdCapabilityHandler.h"
#include "UsdClipboardHandler.h"
#include "UsdDebugHandler.h"
#include "UsdExtendedAttributeHandler.h"
#include "UsdExtendedConnectionHandler.h"
#include "UsdFileHandler.h"
#include "UsdHierarchyHandler.h"
#include "UsdLookdevHandler.h"
#include "UsdMaterialHandler.h"
#include "UsdSceneItemUIHandler.h"
#include "UsdSceneItemOpsHandler.h"
#include "UsdSoloingHandler.h"
#include "UsdUINodeGraphNodeHandler.h"

#include <ufe/handlerInterface.h>
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>

#include <pxr/usd/sdr/registry.h>

namespace
{

// Runtime ids (default 0 is invalid).
Ufe::Rtid mayaUsdRuntimeId = 0;
Ufe::Rtid mayaRuntimeId = 0;

Ufe::SceneItemOpsHandler::Ptr mayaUsdSceneItemOpsHandler;
LookdevXUfe::LookdevHandler::Ptr mayaLookdevHandler;

} // namespace

namespace LookdevXUsd
{

constexpr auto kMayaUsdRuntimeName = "USD";
constexpr auto kMayaRuntimeName = "Maya-DG";

void initialize()
{
    // New extension handlers.
    try
    {
        mayaUsdRuntimeId = Ufe::RunTimeMgr::instance().getId(kMayaUsdRuntimeName);
        mayaRuntimeId = Ufe::RunTimeMgr::instance().getId(kMayaRuntimeName);
    }
    catch (const std::exception&)
    {
        return;
    }

    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdDebugHandler::id, UsdDebugHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdLookdevHandler::id, UsdLookdevHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdMaterialHandler::id, UsdMaterialHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdSoloingHandler::id, UsdSoloingHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdFileHandler::id, UsdFileHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdSceneItemUIHandler::id,
                                                UsdSceneItemUIHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdExtendedAttributeHandler::id,
                                                UsdExtendedAttributeHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdExtendedConnectionHandler::id,
                                                UsdExtendedConnectionHandler::create());
    Ufe::RunTimeMgr::instance().registerHandler(mayaUsdRuntimeId, UsdCapabilityHandler::id,
                                                UsdCapabilityHandler::create());

    // Replacements/wrappers for existing handlers.
    UsdUINodeGraphNodeHandler::registerHandler(mayaUsdRuntimeId);
    UsdHierarchyHandler::registerHandler(mayaUsdRuntimeId);
    UsdClipboardHandler::registerHandler(mayaUsdRuntimeId);

    mayaUsdSceneItemOpsHandler = Ufe::RunTimeMgr::instance().sceneItemOpsHandler(mayaUsdRuntimeId);
    Ufe::RunTimeMgr::instance().setSceneItemOpsHandler(
        mayaUsdRuntimeId, LookdevXUsd::UsdSceneItemOpsHandler::create(mayaUsdSceneItemOpsHandler));

    mayaLookdevHandler = LookdevXUfe::LookdevHandler::get(mayaRuntimeId);
    Ufe::RunTimeMgr::instance().registerHandler(mayaRuntimeId, ProxyShapeLookdevHandler::id,
        ProxyShapeLookdevHandler::create(mayaLookdevHandler));

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
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdDebugHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdLookdevHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdMaterialHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdSoloingHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdFileHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdSceneItemUIHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdExtendedAttributeHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdExtendedConnectionHandler::id);
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaUsdRuntimeId, UsdCapabilityHandler::id);

    // Replacements/wrappers for existing handlers
    UsdUINodeGraphNodeHandler::unregisterHandler();
    UsdHierarchyHandler::unregisterHandler();
    UsdClipboardHandler::unregisterHandler();

    // Unregister the decorated MayaUsd handlers and restore the original ones.
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    if (runTimeMgr.hasId(mayaUsdRuntimeId)) {
        runTimeMgr.setSceneItemOpsHandler(mayaUsdRuntimeId, mayaUsdSceneItemOpsHandler);
    }
    mayaUsdSceneItemOpsHandler.reset();

    // Unregister the decorated Maya handlers and restore the original ones.
    Ufe::RunTimeMgr::instance().unregisterHandler(mayaRuntimeId, ProxyShapeLookdevHandler::id);
    if (runTimeMgr.hasId(mayaRuntimeId) && mayaLookdevHandler) {
      Ufe::RunTimeMgr::instance().registerHandler(mayaRuntimeId, ProxyShapeLookdevHandler::id, mayaLookdevHandler);
    }
    mayaLookdevHandler.reset();
}

} // namespace LookdevXUsd
