//*****************************************************************************
// Copyright (c) 2025 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#include "ProxyShapeLookdevHandler.h"

#include <mayaUsdAPI/utils.h>

#include <ufe/hierarchy.h>

namespace LookdevXUsd
{

ProxyShapeLookdevHandler::ProxyShapeLookdevHandler(LookdevXUfe::LookdevHandler::Ptr previousHandler)
    : m_previousHandler(std::move(previousHandler))
{
}

/*static*/
ProxyShapeLookdevHandler::Ptr ProxyShapeLookdevHandler::create(const LookdevXUfe::LookdevHandler::Ptr& previousHandler)
{
    return std::make_shared<ProxyShapeLookdevHandler>(previousHandler);
}

//------------------------------------------------------------------------------
// ProxyShapeLookdevHandler overrides
//------------------------------------------------------------------------------

Ufe::SceneItemResultUndoableCommand::Ptr ProxyShapeLookdevHandler::createLookdevContainerCmdImpl(
    const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const
{
    // Check if parent is a proxy shape.
    if (!parent || !MayaUsdAPI::isAGatewayType(parent->nodeType()))
    {
        return m_previousHandler ? m_previousHandler->createLookdevContainerCmdImpl(parent, name) : nullptr;
    }

    // Treat proxy shapes in the same way as every other USD item.
    auto parentItem = MayaUsdAPI::createUsdSceneItem(parent->path(), MayaUsdAPI::ufePathToPrim(parent->path()));
    if (!parentItem || !MayaUsdAPI::getPrimForUsdSceneItem(parentItem).IsValid())
    {
        return nullptr;
    }

    return MayaUsdAPI::createAddNewPrimCommand(parentItem, name.string(), "Material");
}

Ufe::SceneItemResultUndoableCommand::Ptr ProxyShapeLookdevHandler::createLookdevContainerCmdImpl(
    const Ufe::SceneItem::Ptr& parent, const Ufe::NodeDef::Ptr& nodeDef) const
{
    // The underlying command MayaUsd::ufe::UsdUndoAddNewMaterialCommand currently doesn't work for the proxy shape.
    // Pass to the previous handler.
    return m_previousHandler ? m_previousHandler->createLookdevContainerCmdImpl(parent, nodeDef) : nullptr;
}

Ufe::SceneItemResultUndoableCommand::Ptr ProxyShapeLookdevHandler::createLookdevEnvironmentCmdImpl(
    const Ufe::SceneItem::Ptr& ancestor, Ufe::Rtid targetRunTimeId) const
{
    // This handler is only aware of gateways from Maya to USD.
    if (!ancestor || ancestor->runTimeId() != MayaUsdAPI::getMayaRunTimeId() ||
        targetRunTimeId != MayaUsdAPI::getUsdRunTimeId())
    {
        return m_previousHandler ? m_previousHandler->createLookdevEnvironmentCmdImpl(ancestor, targetRunTimeId)
                                 : nullptr;
    }

    return MayaUsdCreateLookdevEnvironmentCommand::create(ancestor->path());
}

bool ProxyShapeLookdevHandler::isLookdevContainerImpl(const Ufe::SceneItem::Ptr& item) const
{
    // If item is not a proxy shape, pass to the previous handler.
    if (!item || !MayaUsdAPI::isAGatewayType(item->nodeType()))
    {
        return m_previousHandler ? m_previousHandler->isLookdevContainerImpl(item) : false;
    }

    return item->nodeType() == "Material";
}

MayaUsdCreateLookdevEnvironmentCommand::MayaUsdCreateLookdevEnvironmentCommand(Ufe::Path ancestor)
    : m_ancestor(std::move(ancestor)),
      m_cmds(std::make_shared<Ufe::CompositeUndoableCommand>())
{
}

MayaUsdCreateLookdevEnvironmentCommand::Ptr MayaUsdCreateLookdevEnvironmentCommand::create(const Ufe::Path& ancestor)
{
    return std::make_shared<MayaUsdCreateLookdevEnvironmentCommand>(ancestor);
}

Ufe::SceneItem::Ptr MayaUsdCreateLookdevEnvironmentCommand::sceneItem() const
{
    return Ufe::Hierarchy::createItem(m_materialParent);
}

void MayaUsdCreateLookdevEnvironmentCommand::execute()
{
    bool success = executeCommand();
    if (!success)
    {
        m_cmds->undo();
        m_cmds.reset();
    }
}

bool MayaUsdCreateLookdevEnvironmentCommand::executeCommand()
{
    auto ancestor = Ufe::Hierarchy::createItem(m_ancestor);

    if (!ancestor || ancestor->runTimeId() != MayaUsdAPI::getMayaRunTimeId())
    {
        return false;
    }

    // Check if m_ancestor is a proxy shape or the transform of a proxy shape.
    Ufe::SceneItem::Ptr proxyShape = nullptr;
    if (MayaUsdAPI::isAGatewayType(ancestor->nodeType()))
    {
        proxyShape = ancestor;
    }
    else
    {
        Ufe::Hierarchy::Ptr hierarchy = Ufe::Hierarchy::hierarchy(ancestor);
        if (hierarchy && hierarchy->children().size() == 1)
        {
            auto child = hierarchy->children().back();
            if (child && MayaUsdAPI::isAGatewayType(child->nodeType()))
            {
                proxyShape = child;
            }
        }
    }

    // If not, create a new proxy shape.
    if (!proxyShape)
    {
        auto createProxyCommand = std::dynamic_pointer_cast<Ufe::SceneItemResultUndoableCommand>(
            MayaUsdAPI::createStageWithNewLayerCommand(ancestor));
        if (!createProxyCommand)
        {
            return false;
        }

        createProxyCommand->execute();
        proxyShape = createProxyCommand->sceneItem();
        if (!proxyShape)
        {
            return false;
        }
        m_cmds->append(createProxyCommand);
    }

    // Create a materials scope under the proxy shape.
    auto proxyShapeItem =
        MayaUsdAPI::createUsdSceneItem(proxyShape->path(), MayaUsdAPI::ufePathToPrim(proxyShape->path()));
    if (!proxyShapeItem || !MayaUsdAPI::getPrimForUsdSceneItem(proxyShapeItem).IsValid())
    {
        return false;
    }
    auto createMaterialsScopeCmd = std::dynamic_pointer_cast<Ufe::SceneItemResultUndoableCommand>(
        MayaUsdAPI::createMaterialsScopeCommand(proxyShapeItem));
    if (!createMaterialsScopeCmd)
    {
        return false;
    }
    createMaterialsScopeCmd->execute();

    auto materialsScope = createMaterialsScopeCmd->sceneItem();
    if (!materialsScope)
    {
        return false;
    }
    m_cmds->append(createMaterialsScopeCmd);
    m_materialParent = materialsScope->path();

    return true;
}

void MayaUsdCreateLookdevEnvironmentCommand::undo()
{
    if (m_cmds)
    {
        m_cmds->undo();
        m_materialParent = {};
    }
}

void MayaUsdCreateLookdevEnvironmentCommand::redo()
{
    if (m_cmds)
    {
        m_cmds->redo();
    }
}

} // namespace LookdevXUsd
