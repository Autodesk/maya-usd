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

#ifndef USD_UI_NODE_GRAPH_NODE_HANDLER_H
#define USD_UI_NODE_GRAPH_NODE_HANDLER_H

#include "Export.h"

#include <LookdevXUfe/UINodeGraphNodeHandler.h>

namespace LookdevXUsd
{

//! \brief Implementation of Ufe::UINodeGraphNodeHandler interface for USD objects.
class UsdUINodeGraphNodeHandler : public LookdevXUfe::UINodeGraphNodeHandler
{
public:
    using Ptr = std::shared_ptr<UsdUINodeGraphNodeHandler>;

    LOOKDEVX_USD_EXPORT UsdUINodeGraphNodeHandler() = default;
    LOOKDEVX_USD_EXPORT ~UsdUINodeGraphNodeHandler() override;

    // Delete the copy/move constructors assignment operators.
    UsdUINodeGraphNodeHandler(const UsdUINodeGraphNodeHandler&) = delete;
    UsdUINodeGraphNodeHandler& operator=(const UsdUINodeGraphNodeHandler&) = delete;
    UsdUINodeGraphNodeHandler(UsdUINodeGraphNodeHandler&&) = delete;
    UsdUINodeGraphNodeHandler& operator=(UsdUINodeGraphNodeHandler&&) = delete;

    LOOKDEVX_USD_EXPORT static void registerHandler(const Ufe::Rtid& rtId);
    LOOKDEVX_USD_EXPORT static void unregisterHandler();

    // Ufe::UINodeGraphNodeHandler overrides
    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::UINodeGraphNode::Ptr uiNodeGraphNode(
        const Ufe::SceneItem::Ptr& item) const override;

    // LookdevXUfe::UINodeGraphNodeHandler overrides
    [[nodiscard]] LOOKDEVX_USD_EXPORT LookdevXUfe::UINodeGraphNode::Ptr lxUINodeGraphNode(
        const Ufe::SceneItem::Ptr& item) const override;

private:
    static Ufe::UINodeGraphNodeHandler::Ptr m_wrappedUINodeGraphNodeHandler;
    static Ufe::Rtid m_rtid;
};

} // namespace LookdevXUsd

#endif /* USDUINODEGRAPHNODEHANDLER_H */
