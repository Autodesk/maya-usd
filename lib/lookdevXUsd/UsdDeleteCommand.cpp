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
#include "UsdDeleteCommand.h"

#include <LookdevXUfe/UfeUtils.h>

#include <pxr/base/tf/diagnostic.h>

using namespace PXR_NS;

namespace LookdevXUsd
{
UsdDeleteCommand::UsdDeleteCommand(Ufe::UndoableCommand::Ptr mayaUsdDeleteCmd, Ufe::SceneItem::Ptr item)
    : m_mayaUsdDeleteCommand(std::move(mayaUsdDeleteCmd)), m_item(std::move(item))
{
}

UsdDeleteCommand::~UsdDeleteCommand() = default;

UsdDeleteCommand::Ptr UsdDeleteCommand::create(const Ufe::UndoableCommand::Ptr& mayaUsdDeleteCmd,
                                               const Ufe::SceneItem::Ptr& item)
{
    return std::make_shared<UsdDeleteCommand>(mayaUsdDeleteCmd, item);
}

void UsdDeleteCommand::execute()
{
    if (!TF_VERIFY(m_item, "Invalid item\n") || !TF_VERIFY(m_mayaUsdDeleteCommand, "Invalid MayaUsd Delete Cmd\n"))
    {
        return;
    }

    // 1. Before deleting the prim, when possible, delete all component connections connected to and from the item to be
    // deleted.
    {
        m_deleteComponentConnectionsCmd = LookdevXUfe::UfeUtils::deleteComponentConnections(m_item);
        if (m_deleteComponentConnectionsCmd)
        {
            m_deleteComponentConnectionsCmd->execute();
        }
    }

    // 2. Delete also, when possible, the converter nodes connected to the node to be deleted.
    {
        m_deleteAdskConverterConnectionsCmd
            = LookdevXUfe::UfeUtils::deleteAdskConverterConnections(m_item);
        if (m_deleteAdskConverterConnectionsCmd)
        {
            m_deleteAdskConverterConnectionsCmd->execute();
        }
    }

    // 3. Delete the item with its regular connections.
    m_mayaUsdDeleteCommand->execute();
}

void UsdDeleteCommand::undo()
{
    m_mayaUsdDeleteCommand->undo();

    if (m_deleteAdskConverterConnectionsCmd)
    {
        m_deleteAdskConverterConnectionsCmd->undo();
    }

    if (m_deleteComponentConnectionsCmd)
    {
        m_deleteComponentConnectionsCmd->undo();
    }
}

void UsdDeleteCommand::redo()
{
    // Redo the component connections first, then redo the delete command. We need to redo the component connections
    // first, otherwise some of the properties from the combined and separated items needed by the deleted item
    // are not found.
    if (m_deleteComponentConnectionsCmd)
    {
        m_deleteComponentConnectionsCmd->redo();
    }

    if (m_deleteAdskConverterConnectionsCmd)
    {
        m_deleteAdskConverterConnectionsCmd->redo();
    }

    m_mayaUsdDeleteCommand->redo();
}
} // namespace LookdevXUsd
