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
#ifndef USD_DELETE_COMMAND_H
#define USD_DELETE_COMMAND_H

#include <pxr/usd/usd/prim.h>

#include <ufe/sceneItem.h>
#include <ufe/undoableCommand.h>

namespace LookdevXUsd
{
//! \brief This command wraps the MayaUsd::ufe::UsdUndoDeleteCommand to incorporate additional behavior into it.
class UsdDeleteCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdDeleteCommand>;

    UsdDeleteCommand(Ufe::UndoableCommand::Ptr mayaUsdDeleteCmd, Ufe::SceneItem::Ptr item);
    ~UsdDeleteCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdDeleteCommand(const UsdDeleteCommand&) = delete;
    UsdDeleteCommand& operator=(const UsdDeleteCommand&) = delete;
    UsdDeleteCommand(UsdDeleteCommand&&) = delete;
    UsdDeleteCommand& operator=(UsdDeleteCommand&&) = delete;

    //! Create a UsdDeleteCommand.
    static UsdDeleteCommand::Ptr create(const Ufe::UndoableCommand::Ptr& mayaUsdDeleteCmd,
                                        const Ufe::SceneItem::Ptr& item);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Delete"; })

private:
    Ufe::UndoableCommand::Ptr m_mayaUsdDeleteCommand;
    Ufe::SceneItem::Ptr m_item;
    std::shared_ptr<Ufe::CompositeUndoableCommand> m_deleteComponentConnectionsCmd;
    std::shared_ptr<Ufe::CompositeUndoableCommand> m_deleteAdskConverterConnectionsCmd;
}; // UsdDeleteCommand

} // namespace LookdevXUsd
#endif