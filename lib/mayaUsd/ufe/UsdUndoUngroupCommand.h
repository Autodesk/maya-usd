//
// Copyright 2021 Autodesk
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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoUngroupCommand
class MAYAUSD_CORE_PUBLIC UsdUndoUngroupCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdUndoUngroupCommand>;

    UsdUndoUngroupCommand(const UsdSceneItem::Ptr& groupItem);
    ~UsdUndoUngroupCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoUngroupCommand(const UsdUndoUngroupCommand&) = delete;
    UsdUndoUngroupCommand& operator=(const UsdUndoUngroupCommand&) = delete;
    UsdUndoUngroupCommand(UsdUndoUngroupCommand&&) = delete;
    UsdUndoUngroupCommand& operator=(UsdUndoUngroupCommand&&) = delete;

    //! Create a UsdUndoUngroupCommand.
    static UsdUndoUngroupCommand::Ptr create(const UsdSceneItem::Ptr& groupItem);

private:
    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdSceneItem::Ptr _groupItem;

    UsdUndoableItem _undoableItem;

}; // UsdUndoUngroupCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
