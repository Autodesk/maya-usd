//
// Copyright 2019 Autodesk
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
#ifndef MAYAUSD_USDUNDORENAMECOMMAND_H
#define MAYAUSD_USDUNDORENAMECOMMAND_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <ufe/path.h>
#include <ufe/pathComponent.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoRenameCommand
#ifdef UFE_V4_FEATURES_AVAILABLE
class MAYAUSD_CORE_PUBLIC UsdUndoRenameCommand : public Ufe::SceneItemResultUndoableCommand
#else
class MAYAUSD_CORE_PUBLIC UsdUndoRenameCommand : public Ufe::UndoableCommand
#endif
{
public:
    typedef std::shared_ptr<UsdUndoRenameCommand> Ptr;

    UsdUndoRenameCommand(
        const UsdUfe::UsdSceneItem::Ptr& srcItem,
        const Ufe::PathComponent&        newName);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoRenameCommand);

    //! Create a UsdUndoRenameCommand from a USD scene item and UFE pathcomponent.
    static UsdUndoRenameCommand::Ptr
    create(const UsdUfe::UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName);

    UsdUfe::UsdSceneItem::Ptr renamedItem() const;
    UFE_V4(Ufe::SceneItem::Ptr sceneItem() const override { return renamedItem(); })

private:
    void renameRedo();
    void renameUndo();

    void undo() override;
    void redo() override;

    UsdUfe::UsdSceneItem::Ptr _ufeSrcItem;
    UsdUfe::UsdSceneItem::Ptr _ufeDstItem;

    PXR_NS::UsdStageWeakPtr _stage;
    std::string             _newName;

}; // UsdUndoRenameCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDUNDORENAMECOMMAND_H
