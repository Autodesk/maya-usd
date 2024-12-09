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

#include "UsdUINodeGraphNodeHandler.h"
#include "UsdUINodeGraphNode.h"

#include <ufe/runTimeMgr.h>

namespace LookdevXUsd
{

Ufe::UINodeGraphNodeHandler::Ptr UsdUINodeGraphNodeHandler::m_wrappedUINodeGraphNodeHandler;
Ufe::Rtid UsdUINodeGraphNodeHandler::m_rtid;

UsdUINodeGraphNodeHandler::~UsdUINodeGraphNodeHandler() = default;

void UsdUINodeGraphNodeHandler::registerHandler(const Ufe::Rtid& rtId)
{
    if (m_wrappedUINodeGraphNodeHandler)
    {
        return;
    }

    m_rtid = rtId;
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    m_wrappedUINodeGraphNodeHandler = runTimeMgr.uiNodeGraphNodeHandler(m_rtid);
    runTimeMgr.setUINodeGraphNodeHandler(m_rtid, std::make_shared<UsdUINodeGraphNodeHandler>());
}

void UsdUINodeGraphNodeHandler::unregisterHandler()
{
    if (m_wrappedUINodeGraphNodeHandler)
    {
        auto& runTimeMgr = Ufe::RunTimeMgr::instance();
        if (runTimeMgr.hasId(m_rtid))
        {
            runTimeMgr.setUINodeGraphNodeHandler(m_rtid, m_wrappedUINodeGraphNodeHandler);
        }
        m_wrappedUINodeGraphNodeHandler.reset();
    }
}

Ufe::UINodeGraphNode::Ptr UsdUINodeGraphNodeHandler::uiNodeGraphNode(const Ufe::SceneItem::Ptr& item) const
{
    return lxUINodeGraphNode(item);
}

LookdevXUfe::UINodeGraphNode::Ptr UsdUINodeGraphNodeHandler::lxUINodeGraphNode(const Ufe::SceneItem::Ptr& item) const
{
    return LookdevXUsd::UsdUINodeGraphNode::create(m_wrappedUINodeGraphNodeHandler->uiNodeGraphNode(item));
}

} // namespace LookdevXUsd
