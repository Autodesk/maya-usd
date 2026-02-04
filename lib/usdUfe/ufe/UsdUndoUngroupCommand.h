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
#ifndef USDUFE_USDUNDOUNGROUPCOMMAND_H
#define USDUFE_USDUNDOUNGROUPCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoUngroupCommand
class USDUFE_PUBLIC UsdUndoUngroupCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdUndoUngroupCommand>;

    UsdUndoUngroupCommand(const UsdSceneItem::Ptr& groupItem);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoUngroupCommand);

    //! Create a UsdUndoUngroupCommand.
    static UsdUndoUngroupCommand::Ptr create(const UsdSceneItem::Ptr& groupItem);

private:
    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Ungroup"; })

private:
    UsdSceneItem::Ptr _groupItem;

    UsdUndoableItem _undoableItem;

}; // UsdUndoUngroupCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDOUNGROUPCOMMAND_H
