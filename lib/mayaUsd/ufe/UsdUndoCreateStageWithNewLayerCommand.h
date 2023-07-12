//
// Copyright 2023 Autodesk
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
#ifndef MAYAUSD_UFE_CREATESTAGEWITHNEWLAYERCOMMAND_H
#define MAYAUSD_UFE_CREATESTAGEWITHNEWLAYERCOMMAND_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/undo/OpUndoItemList.h>

#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief This command is used to create a new empty stage in memory.
class MAYAUSD_CORE_PUBLIC UsdUndoCreateStageWithNewLayerCommand
    : public Ufe::SceneItemResultUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoCreateStageWithNewLayerCommand> Ptr;

    UsdUndoCreateStageWithNewLayerCommand(const Ufe::SceneItem::Ptr& parentItem);
    ~UsdUndoCreateStageWithNewLayerCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoCreateStageWithNewLayerCommand(const UsdUndoCreateStageWithNewLayerCommand&) = delete;
    UsdUndoCreateStageWithNewLayerCommand& operator=(const UsdUndoCreateStageWithNewLayerCommand&)
        = delete;
    UsdUndoCreateStageWithNewLayerCommand(UsdUndoCreateStageWithNewLayerCommand&&) = delete;
    UsdUndoCreateStageWithNewLayerCommand& operator=(UsdUndoCreateStageWithNewLayerCommand&&)
        = delete;

    //! Create a UsdUndoCreateStageWithNewLayerCommand. Executing this command should produce the
    //! following:
    //! - Proxyshape
    //! - Stage
    //! - Session Layer
    //! - Anonymous Root Layer (this is set as the target layer)
    //! Since the proxy shape does not have a USD file associated (in the .filePath attribute), the
    //! proxy shape base will create an empty stage in memory. This will create the session and root
    //! layer as well.
    static UsdUndoCreateStageWithNewLayerCommand::Ptr create(const Ufe::SceneItem::Ptr& parentItem);

    Ufe::SceneItem::Ptr sceneItem() const override;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    // Executes the command, called within a undo recorder.
    // Returns true on success.
    bool executeWithinUndoRecorder();

    Ufe::SceneItem::Ptr _parentItem;
    Ufe::SceneItem::Ptr _insertedChild;

    OpUndoItemList _undoItemList;
}; // UsdUndoCreateStageWithNewLayerCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UFE_CREATESTAGEWITHNEWLAYERCOMMAND_H
