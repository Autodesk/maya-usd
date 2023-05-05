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
#ifndef MAYAUSD_UFE_USDUNDOCLIPBOARDCOMMAND_H
#define MAYAUSD_UFE_USDUNDOCLIPBOARDCOMMAND_H

#include "UsdUndoDuplicateSelectionCommand.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoCopyClipboardCommand
class MAYAUSD_CORE_PUBLIC UsdUndoCopyClipboardCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoCopyClipboardCommand> Ptr;

    UsdUndoCopyClipboardCommand(const Ufe::Selection& selection);
    ~UsdUndoCopyClipboardCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoCopyClipboardCommand(const UsdUndoCopyClipboardCommand&) = delete;
    UsdUndoCopyClipboardCommand& operator=(const UsdUndoCopyClipboardCommand&) = delete;
    UsdUndoCopyClipboardCommand(UsdUndoCopyClipboardCommand&&) = delete;
    UsdUndoCopyClipboardCommand& operator=(UsdUndoCopyClipboardCommand&&) = delete;

    //! Create a UsdUndoCopyClipboardCommand from a selection.
    static Ptr create(const Ufe::Selection& selection);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdUndoableItem _undoableItem;
    Ufe::Selection  _selection;
}; // UsdUndoCopyClipboardCommand

//! \brief UsdUndoCutClipboardCommand
class MAYAUSD_CORE_PUBLIC UsdUndoCutClipboardCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoCutClipboardCommand> Ptr;

    UsdUndoCutClipboardCommand(const Ufe::Selection& selection);
    ~UsdUndoCutClipboardCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoCutClipboardCommand(const UsdUndoCutClipboardCommand&) = delete;
    UsdUndoCutClipboardCommand& operator=(const UsdUndoCutClipboardCommand&) = delete;
    UsdUndoCutClipboardCommand(UsdUndoCutClipboardCommand&&) = delete;
    UsdUndoCutClipboardCommand& operator=(UsdUndoCutClipboardCommand&&) = delete;

    //! Create a UsdUndoCutClipboardCommand from a selection.
    static Ptr create(const Ufe::Selection& selection);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdUndoableItem _undoableItem;
    Ufe::Selection  _selection;
}; // UsdUndoCutClipboardCommand

//! \brief UsdUndoPasteClipboardCommand
class MAYAUSD_CORE_PUBLIC UsdUndoPasteClipboardCommand : public Ufe::SelectionUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoPasteClipboardCommand> Ptr;

    UsdUndoPasteClipboardCommand(const Ufe::SceneItem::Ptr& dstParentItem);
    ~UsdUndoPasteClipboardCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoPasteClipboardCommand(const UsdUndoPasteClipboardCommand&) = delete;
    UsdUndoPasteClipboardCommand& operator=(const UsdUndoPasteClipboardCommand&) = delete;
    UsdUndoPasteClipboardCommand(UsdUndoPasteClipboardCommand&&) = delete;
    UsdUndoPasteClipboardCommand& operator=(UsdUndoPasteClipboardCommand&&) = delete;

    //! Create a UsdUndoPasteClipboardCommand from a scene item.
    static Ptr create(const Ufe::SceneItem::Ptr& dstParentItem);

    void execute() override;
    void undo() override;
    void redo() override;

    Ufe::SceneItem::Ptr              targetItem(const Ufe::Path& sourcePath) const override;
    std::vector<Ufe::SceneItem::Ptr> targetItems() const override;

private:
    UsdUndoableItem _undoableItem;

    // The destination parent item for the pasted items.
    UsdSceneItem::Ptr _dstParentItem;

    // Needed by targetItem and by targetItems.
    UsdUndoDuplicateSelectionCommand::Ptr _selectionUndoableCmd;

}; // UsdUndoPasteClipboardCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
