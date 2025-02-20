//**************************************************************************/
// Copyright 2024 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//**************************************************************************/
#ifndef USD_SCENE_ITEM_OPS_H
#define USD_SCENE_ITEM_OPS_H

#include <ufe/path.h>
#include <ufe/sceneItem.h>
#include <ufe/sceneItemOps.h>
#include <ufe/undoableCommand.h>

namespace LookdevXUsd
{

class UsdSceneItemOps : public Ufe::SceneItemOps
{
public:
    using Ptr = std::shared_ptr<UsdSceneItemOps>;

    explicit UsdSceneItemOps(Ufe::SceneItemOps::Ptr wrappedMayaUsdSceneItemOps)
        : m_wrappedMayaUsdSceneItemOps(std::move(wrappedMayaUsdSceneItemOps))
    {
    }

    ~UsdSceneItemOps() override = default;

    //@{
    //! Delete the copy/move constructors assignment operators.
    UsdSceneItemOps(const UsdSceneItemOps&) = delete;
    UsdSceneItemOps& operator=(const UsdSceneItemOps&) = delete;
    UsdSceneItemOps(UsdSceneItemOps&&) = delete;
    UsdSceneItemOps& operator=(UsdSceneItemOps&&) = delete;
    //@}

    //! Create a UsdSceneItemOps.
    static UsdSceneItemOps::Ptr create(Ufe::SceneItemOps::Ptr wrappedMayaUsdSceneItemOps);

    //! Ufe::SceneItemOps overrides.
    [[nodiscard]] Ufe::SceneItem::Ptr sceneItem() const override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr deleteItemCmdNoExecute() override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr deleteItemCmd() override;
    [[nodiscard]] bool deleteItem() override;
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr duplicateItemCmdNoExecute() override;
    [[nodiscard]] Ufe::Duplicate duplicateItemCmd() override;
    [[nodiscard]] Ufe::SceneItem::Ptr duplicateItem() override;
    [[nodiscard]] Ufe::SceneItemResultUndoableCommand::Ptr renameItemCmdNoExecute(
        const Ufe::PathComponent& newName) override;
    [[nodiscard]] Ufe::Rename renameItemCmd(const Ufe::PathComponent& newName) override;
    [[nodiscard]] Ufe::SceneItem::Ptr renameItem(const Ufe::PathComponent& newName) override;

private:
    Ufe::SceneItemOps::Ptr m_wrappedMayaUsdSceneItemOps;

}; // UsdSceneItemOps

} // namespace LookdevXUsd
#endif