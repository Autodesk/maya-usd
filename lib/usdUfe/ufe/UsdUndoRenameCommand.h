//
// Copyright 2025 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef USDUFE_USDUNDORENAMECOMMAND_H
#define USDUFE_USDUNDORENAMECOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/path.h>
#include <ufe/pathComponent.h>
#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoRenameCommand
#ifdef UFE_V4_FEATURES_AVAILABLE
class USDUFE_PUBLIC UsdUndoRenameCommand : public Ufe::SceneItemResultUndoableCommand
#else
class USDUFE_PUBLIC UsdUndoRenameCommand : public Ufe::UndoableCommand
#endif
{
public:
    typedef std::shared_ptr<UsdUndoRenameCommand> Ptr;

    UsdUndoRenameCommand(
        const UsdUfe::UsdSceneItem::Ptr& srcItem,
        const Ufe::PathComponent&        newName);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoRenameCommand);

    //! Create a UsdUndoRenameCommand from a USD scene item and UFE pathcomponent.
    static UsdUndoRenameCommand::Ptr
    create(const UsdUfe::UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName);

    UsdUfe::UsdSceneItem::Ptr renamedItem() const;
    UFE_V4(Ufe::SceneItem::Ptr sceneItem() const override { return renamedItem(); })

    virtual void sendRenameNotification(
        const PXR_NS::UsdStagePtr& stage,
        const PXR_NS::UsdPrim&     prim,
        const Ufe::Path&           srcPath,
        const Ufe::Path&           dstPath);

private:
    void renameRedo();
    void renameUndo();

    void renameFirstTime(const Ufe::Path& srcPath, const Ufe::Path& dstPath);

    // Refresh the item of the renamed prim, from the prim at the new path, and notify UFE.
    void updateItemAndNotify(
        const Ufe::Path&           srcPath,
        const Ufe::Path&           dstPath,
        const std::string&         newName,
        UsdUfe::UsdSceneItem::Ptr& ufeDstItem);

    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Rename"; })

    UsdUfe::UsdSceneItem::Ptr _ufeSrcItem;
    UsdUfe::UsdSceneItem::Ptr _ufeDstItem;

    PXR_NS::UsdStageWeakPtr _stage;
    std::string             _newName;

    // All the layer edits of the command, captured on its first execution: renaming the prim,
    // updating the paths pointing to it and the default prim. Undo and redo invert them.
    UsdUfe::UsdUndoableItem _undoableItem;
    bool                    _hasUndoableEdits { false };

}; // UsdUndoRenameCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDORENAMECOMMAND_H
