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
#include "UsdClipboardHandler.h"

#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/nodeGraph.h>

#include <ufe/runTimeMgr.h>

#include <mayaUsdAPI/clipboard.h>

#include <filesystem>

namespace
{
bool isNodeGraph(const PXR_NS::UsdPrim& prim)
{
    return bool(PXR_NS::UsdShadeNodeGraph(prim));
}

bool isNonMaterial(const PXR_NS::UsdPrim& prim)
{
    return !PXR_NS::UsdShadeMaterial(prim);
}

bool isMaterial(const PXR_NS::UsdPrim& prim)
{
    return bool(PXR_NS::UsdShadeMaterial(prim));
}

} // namespace

namespace LookdevXUsd
{
Ufe::ClipboardHandler::Ptr UsdClipboardHandler::m_wrappedClipboardHandler;
Ufe::Rtid UsdClipboardHandler::m_rtid;

void UsdClipboardHandler::registerHandler(const Ufe::Rtid& rtId)
{
    if (m_wrappedClipboardHandler)
    {
        return;
    }

    m_rtid = rtId;
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    m_wrappedClipboardHandler = runTimeMgr.clipboardHandler(m_rtid);
    // Only register our override if there is a USD one registered.
    // Thus the m_wrappedClipboardHandler is guaranteed to be non-null here.
    if (m_wrappedClipboardHandler)
    {
        runTimeMgr.setClipboardHandler(m_rtid, std::make_shared<UsdClipboardHandler>());
    }
}

void UsdClipboardHandler::unregisterHandler()
{
    if (m_wrappedClipboardHandler)
    {
        auto& runTimeMgr = Ufe::RunTimeMgr::instance();
        if (runTimeMgr.hasId(m_rtid))
        {
            runTimeMgr.setClipboardHandler(m_rtid, m_wrappedClipboardHandler);
        }
        m_wrappedClipboardHandler.reset();
    }
}

Ufe::UndoableCommand::Ptr UsdClipboardHandler::cutCmd_(const Ufe::Selection& selection)
{
    return m_wrappedClipboardHandler->cutCmd_(selection);
}

Ufe::UndoableCommand::Ptr UsdClipboardHandler::copyCmd_(const Ufe::Selection& selection)
{
    return m_wrappedClipboardHandler->copyCmd_(selection);
}

Ufe::PasteClipboardCommand::Ptr UsdClipboardHandler::pasteCmd_(const Ufe::SceneItem::Ptr& parentItem)
{
    return m_wrappedClipboardHandler->pasteCmd_(parentItem);
}

Ufe::UndoableCommand::Ptr UsdClipboardHandler::pasteCmd_(const Ufe::Selection& parentItems)
{
    return m_wrappedClipboardHandler->pasteCmd_(parentItems);
}

bool UsdClipboardHandler::hasMaterialToPasteImpl()
{
    return MayaUsdAPI::hasItemToPaste(m_wrappedClipboardHandler, &isMaterial);
}

bool UsdClipboardHandler::hasItemsToPaste_()
{
    return m_wrappedClipboardHandler->hasItemsToPaste_();
}

bool UsdClipboardHandler::hasNodeGraphsToPasteImpl()
{
    return MayaUsdAPI::hasItemToPaste(m_wrappedClipboardHandler, &isNodeGraph);
}

bool UsdClipboardHandler::hasNonMaterialToPasteImpl()
{
    return MayaUsdAPI::hasItemToPaste(m_wrappedClipboardHandler, &isNonMaterial);
}

void UsdClipboardHandler::setClipboardPath(const std::string& clipboardPath)
{
    auto tmpPath = std::filesystem::path(clipboardPath);
    tmpPath.append(clipboardFileName);
    tmpPath.replace_extension("usda");
    MayaUsdAPI::setClipboardFilePath(m_wrappedClipboardHandler, tmpPath.string());
    MayaUsdAPI::setClipboardFileFormat(m_wrappedClipboardHandler, "usda");
}

bool UsdClipboardHandler::canBeCut_(const Ufe::SceneItem::Ptr& item)
{
    return m_wrappedClipboardHandler->canBeCut_(item);
}

void UsdClipboardHandler::preCopy_()
{
    m_wrappedClipboardHandler->preCopy_();
}

void UsdClipboardHandler::preCut_()
{
    m_wrappedClipboardHandler->preCut_();
}

} // namespace LookdevXUsd
