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
#pragma once

#include <LookdevXUfe/LookdevHandler.h>

#include <ufe/undoableCommand.h>

namespace LookdevXUsd
{

//! \brief Maya run-time Lookdev handler. Used to deal with MayaUsdProxyShape items.
class ProxyShapeLookdevHandler : public LookdevXUfe::LookdevHandler
{
public:
    using Ptr = std::shared_ptr<ProxyShapeLookdevHandler>;

    explicit ProxyShapeLookdevHandler(LookdevXUfe::LookdevHandler::Ptr previousHandler);
    ~ProxyShapeLookdevHandler() override = default;

    //@{
    //! Delete the copy/move constructors assignment operators.
    ProxyShapeLookdevHandler(const ProxyShapeLookdevHandler&) = delete;
    ProxyShapeLookdevHandler& operator=(const ProxyShapeLookdevHandler&) = delete;
    ProxyShapeLookdevHandler(ProxyShapeLookdevHandler&&) = delete;
    ProxyShapeLookdevHandler& operator=(ProxyShapeLookdevHandler&&) = delete;
    //@}

    //! Create a ProxyShapeLookdevHandler.
    static ProxyShapeLookdevHandler::Ptr create(const LookdevXUfe::LookdevHandler::Ptr& previousHandler);

    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createLookdevContainerCmdImpl(
        const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name) const override;
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createLookdevContainerCmdImpl(
        const Ufe::SceneItem::Ptr& parent, const Ufe::NodeDef::Ptr& nodeDef) const override;
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr createLookdevEnvironmentCmdImpl(
        const Ufe::SceneItem::Ptr& ancestor, Ufe::Rtid targetRunTimeId) const override;
    [[nodiscard]] bool isLookdevContainerImpl(const Ufe::SceneItem::Ptr& item) const override;

private:
    LookdevXUfe::LookdevHandler::Ptr m_previousHandler;
}; // ProxyShapeLookdevHandler

class MayaUsdCreateLookdevEnvironmentCommand : public Ufe::SceneItemResultUndoableCommand
{
public:
    using Ptr = std::shared_ptr<MayaUsdCreateLookdevEnvironmentCommand>;

    explicit MayaUsdCreateLookdevEnvironmentCommand(Ufe::Path ancestor);
    ~MayaUsdCreateLookdevEnvironmentCommand() override = default;

    // Delete the copy/move constructors assignment operators.
    MayaUsdCreateLookdevEnvironmentCommand(const MayaUsdCreateLookdevEnvironmentCommand&) = delete;
    MayaUsdCreateLookdevEnvironmentCommand& operator=(const MayaUsdCreateLookdevEnvironmentCommand&) = delete;
    MayaUsdCreateLookdevEnvironmentCommand(MayaUsdCreateLookdevEnvironmentCommand&&) = delete;
    MayaUsdCreateLookdevEnvironmentCommand& operator=(MayaUsdCreateLookdevEnvironmentCommand&&) = delete;

    //! Create a MayaUsdCreateLookdevEnvironmentCommand that finds or creates an item under \p ancestor that can serve
    //! as a parent of a material.
    //! - If \p ancestor is a Maya object, a new USD stage will be created under it and a materials scope will be
    //!   created within the new stage.
    //! - If \p ancestor is a USD stage, a materials scope will be created under it.
    //! - If \p ancestor already contains a materials scope, the existing scope will be returned.
    static MayaUsdCreateLookdevEnvironmentCommand::Ptr create(const Ufe::Path& ancestor);

    [[nodiscard]] Ufe::SceneItem::Ptr sceneItem() const override;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    bool executeCommand();

    Ufe::Path m_ancestor;
    Ufe::Path m_materialParent;
    std::shared_ptr<Ufe::CompositeUndoableCommand> m_cmds;
}; // MayaUsdCreateLookdevEnvironmentCommand

} // namespace LookdevXUsd
