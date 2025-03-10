//*****************************************************************************
// Copyright (c) 2023 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#pragma once

#include "Export.h"

#include <ufe/sceneItem.h>
#include <ufe/undoableCommand.h>

namespace LookdevXUsd
{

class LOOKDEVX_USD_EXPORT UsdCreateMaterialParentCommand : public Ufe::SceneItemResultUndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdCreateMaterialParentCommand>;

    explicit UsdCreateMaterialParentCommand(Ufe::SceneItem::Ptr ancestor);
    ~UsdCreateMaterialParentCommand() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdCreateMaterialParentCommand(const UsdCreateMaterialParentCommand&) = delete;
    UsdCreateMaterialParentCommand& operator=(const UsdCreateMaterialParentCommand&) = delete;
    UsdCreateMaterialParentCommand(UsdCreateMaterialParentCommand&&) = delete;
    UsdCreateMaterialParentCommand& operator=(UsdCreateMaterialParentCommand&&) = delete;

    //! Create a UsdCreateMaterialParentCommand that finds or creates an item under \p ancestor that can serve as a
    //! parent of a material.
    //! - If \p ancestor is a materials scope, it will be returned.
    //! - If \p ancestor is the parent of a materials scope, the materials scope will be returned.
    //! - Otherwise, a materials scope will be created under \p ancestor.
    static UsdCreateMaterialParentCommand::Ptr create(const Ufe::SceneItem::Ptr& ancestor);

    [[nodiscard]] Ufe::SceneItem::Ptr sceneItem() const override;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    Ufe::SceneItem::Ptr m_ancestor;
    Ufe::SceneItem::Ptr m_materialParent;
    Ufe::SceneItemResultUndoableCommand::Ptr m_cmd;
}; // UsdCreateMaterialParentCommand

} // namespace LookdevXUsd
