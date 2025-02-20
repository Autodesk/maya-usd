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

#include "UsdHierarchyHandler.h"
#include "UsdHierarchy.h"

#include <ufe/runTimeMgr.h>

namespace LookdevXUsd
{

Ufe::HierarchyHandler::Ptr UsdHierarchyHandler::m_wrappedHierarchyHandler;
Ufe::Rtid UsdHierarchyHandler::m_rtid;

UsdHierarchyHandler::~UsdHierarchyHandler() = default;

void UsdHierarchyHandler::registerHandler(const Ufe::Rtid& rtId)
{
    if (m_wrappedHierarchyHandler)
    {
        return;
    }

    m_rtid = rtId;
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    m_wrappedHierarchyHandler = runTimeMgr.hierarchyHandler(m_rtid);
    runTimeMgr.setHierarchyHandler(m_rtid, std::make_shared<UsdHierarchyHandler>());
}

void UsdHierarchyHandler::unregisterHandler()
{
    if (m_wrappedHierarchyHandler)
    {
        auto& runTimeMgr = Ufe::RunTimeMgr::instance();
        if (runTimeMgr.hasId(m_rtid))
        {
            runTimeMgr.setHierarchyHandler(m_rtid, m_wrappedHierarchyHandler);
        }
        m_wrappedHierarchyHandler.reset();
    }
}

Ufe::Hierarchy::Ptr UsdHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    return UsdHierarchy::create(m_wrappedHierarchyHandler->hierarchy(item));
}

} // namespace LookdevXUsd