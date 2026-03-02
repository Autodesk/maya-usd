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
#include "UsdLookdevHandler.h"

#include "UsdMaterialCommands.h"

#include <mayaUsdAPI/utils.h>

#include <usdUfe/ufe/Utils.h>

namespace
{

// Some MayaUSD commands still extend the deprecated Ufe::InsertChildCommand instead of the new
// Ufe::SceneItemResultUndoableCommand. They are basically equivalent. Wrap a InsertChildCommand, so that it can be
// returned as a SceneItemResultUndoableCommand.
class WrapInsertChildCommand : public Ufe::SceneItemResultUndoableCommand
{
public:
    using Ptr = std::shared_ptr<WrapInsertChildCommand>;

    explicit WrapInsertChildCommand(Ufe::InsertChildCommand::Ptr cmd) : m_wrappedCmd(std::move(cmd))
    {
    }

    ~WrapInsertChildCommand() override = default;

    // Delete the copy/move constructors assignment operators.
    WrapInsertChildCommand(const WrapInsertChildCommand&) = delete;
    WrapInsertChildCommand& operator=(const WrapInsertChildCommand&) = delete;
    WrapInsertChildCommand(WrapInsertChildCommand&&) = delete;
    WrapInsertChildCommand& operator=(WrapInsertChildCommand&&) = delete;

    //! Create a WrapUsdUndoAddNewMaterialCommand.
    static WrapInsertChildCommand::Ptr create(const Ufe::InsertChildCommand::Ptr& cmd)
    {
        return std::make_shared<WrapInsertChildCommand>(cmd);
    }

    [[nodiscard]] Ufe::SceneItem::Ptr sceneItem() const override
    {
        return m_wrappedCmd ? m_wrappedCmd->insertedChild() : nullptr;
    }

    void execute() override
    {
        if (m_wrappedCmd)
        {
            m_wrappedCmd->execute();
        }
    }

    void undo() override
    {
        if (m_wrappedCmd)
        {
            m_wrappedCmd->undo();
        }
    }

    void redo() override
    {
        if (m_wrappedCmd)
        {
            m_wrappedCmd->redo();
        }
    }

private:
    Ufe::InsertChildCommand::Ptr m_wrappedCmd;
}; // WrapInsertChildCommand

} // namespace

namespace LookdevXUsd
{

//------------------------------------------------------------------------------
// UsdLookdevHandler overrides
//------------------------------------------------------------------------------

Ufe::SceneItemResultUndoableCommand::Ptr UsdLookdevHandler::createLookdevContainerCmdImpl(
    const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const
{
    if (!MayaUsdAPI::isUsdSceneItem(parent) || !MayaUsdAPI::getPrimForUsdSceneItem(parent).IsValid())
    {
        return nullptr;
    }

    return MayaUsdAPI::createAddNewPrimCommand(parent, name.string(), "Material");
}

Ufe::SceneItemResultUndoableCommand::Ptr UsdLookdevHandler::createLookdevContainerCmdImpl(
    const Ufe::SceneItem::Ptr& parent, const Ufe::NodeDef::Ptr& nodeDef) const
{
    if (!MayaUsdAPI::isUsdSceneItem(parent) || !MayaUsdAPI::getPrimForUsdSceneItem(parent).IsValid())
    {
        return nullptr;
    }

    if (!nodeDef)
    {
        return nullptr;
    }

    auto cmd =
        std::dynamic_pointer_cast<Ufe::InsertChildCommand>(MayaUsdAPI::addNewMaterialCommand(parent, nodeDef->type()));
    if (!cmd)
    {
        return nullptr;
    }
    return WrapInsertChildCommand::create(cmd);
}

Ufe::SceneItemResultUndoableCommand::Ptr UsdLookdevHandler::createLookdevEnvironmentCmdImpl(
    const Ufe::SceneItem::Ptr& ancestor, Ufe::Rtid targetRunTimeId) const
{
    if (!MayaUsdAPI::isUsdSceneItem(ancestor) || targetRunTimeId != MayaUsdAPI::getUsdRunTimeId())
    {
        return nullptr;
    }

    return UsdCreateMaterialParentCommand::create(ancestor);
}

bool UsdLookdevHandler::isLookdevContainerImpl(const Ufe::SceneItem::Ptr& item) const
{
    return UsdUfe::getSceneItemNodeType(item) == "Material";
}

} // namespace LookdevXUsd
