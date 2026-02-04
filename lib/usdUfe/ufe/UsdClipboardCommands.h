#ifndef USDCLIPBOARDCOMMANDS_H
#define USDCLIPBOARDCOMMANDS_H

//
// Copyright 2024 Autodesk
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

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdClipboard.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <ufe/clipboardCommands.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief Command to copy USD prims to the clipboard.
class USDUFE_PUBLIC UsdCopyClipboardCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdCopyClipboardCommand>;

    UsdCopyClipboardCommand(const Ufe::Selection& selection, const UsdClipboard::Ptr& clipboard);

    // Delete the copy/move constructors assignment operators.
    UsdCopyClipboardCommand(const UsdCopyClipboardCommand&) = delete;
    UsdCopyClipboardCommand& operator=(const UsdCopyClipboardCommand&) = delete;
    UsdCopyClipboardCommand(UsdCopyClipboardCommand&&) = delete;
    UsdCopyClipboardCommand& operator=(UsdCopyClipboardCommand&&) = delete;

    //! Create a UsdCopyClipboardCommand from a selection.
    static Ptr create(const Ufe::Selection& selection, const UsdClipboard::Ptr& clipboard);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Copy"; })

private:
    // The items to copy to the clipboard.
    Ufe::Selection _selection;

    // The cmd sets the clipboard data when executed.
    UsdClipboard::Ptr _clipboard;

}; // UsdCopyClipboardCommand

//! \brief Command to cut USD items to the clipboard and then delete them.
class USDUFE_PUBLIC UsdCutClipboardCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdCutClipboardCommand>;

    UsdCutClipboardCommand(const Ufe::Selection& selection, const UsdClipboard::Ptr& clipboard);
    ~UsdCutClipboardCommand() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdCutClipboardCommand(const UsdCutClipboardCommand&) = delete;
    UsdCutClipboardCommand& operator=(const UsdCutClipboardCommand&) = delete;
    UsdCutClipboardCommand(UsdCutClipboardCommand&&) = delete;
    UsdCutClipboardCommand& operator=(UsdCutClipboardCommand&&) = delete;

    //! Create a UsdCutClipboardCommand from a selection.
    static Ptr create(const Ufe::Selection& selection, const UsdClipboard::Ptr& clipboard);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Cut"; })

private:
    UsdUndoableItem _undoableItem;

    // The items to cut and copy to the clipboard.
    Ufe::Selection _selection;

    // The cmd sets the clipboard data when executed.
    UsdClipboard::Ptr _clipboard;

}; // UsdCutClipboardCommand

//! \brief Retrieve the desired selection after the command has executed.
//         \see UsdUndoSelectAfterCommand.
Ufe::Selection USDUFE_PUBLIC getNewSelectionFromCommand(const UsdCutClipboardCommand& cmd);

//! \brief Command to paste the USD prims from the clipboard.
class USDUFE_PUBLIC UsdPasteClipboardCommand : public Ufe::PasteClipboardCommand
{
public:
    using Ptr = std::shared_ptr<UsdPasteClipboardCommand>;

    UsdPasteClipboardCommand(
        const Ufe::Selection&    dstParentItems,
        const UsdClipboard::Ptr& clipboard);
    UsdPasteClipboardCommand(
        const Ufe::SceneItem::Ptr& dstParentItem,
        const UsdClipboard::Ptr&   clipboard);

    // Delete the copy/move constructors assignment operators.
    UsdPasteClipboardCommand(const UsdPasteClipboardCommand&) = delete;
    UsdPasteClipboardCommand& operator=(const UsdPasteClipboardCommand&) = delete;
    UsdPasteClipboardCommand(UsdPasteClipboardCommand&&) = delete;
    UsdPasteClipboardCommand& operator=(UsdPasteClipboardCommand&&) = delete;

    //! Create a UsdPasteClipboardCommand from a scene item or selection.
    static Ptr create(const Ufe::SceneItem::Ptr& dstParentItem, const UsdClipboard::Ptr& clipboard);
    static Ptr create(const Ufe::Selection& dstParentItems, const UsdClipboard::Ptr& clipboard);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Paste"; })

    // Overridden from Ufe::PasteClipboardCommand
    Ufe::SceneItemList     targetItems() const override;
    std::vector<PasteInfo> getPasteInfos() const override;

    // For selection of pasted items.
    Ufe::SceneItemList itemsToSelect() const { return _itemsToSelect; }

private:
    UsdUndoableItem _undoableItem;

    // The destination parent items for the pasted items.
    std::vector<UsdSceneItem::Ptr> _dstParentItems;

    // The target items.
    Ufe::SceneItemList _targetItems;

    // The target items to select (after paste).
    Ufe::SceneItemList _itemsToSelect;

    // The cmd gets the clipboard data when executed.
    friend class UsdPasteClipboardCommandWithSelection;
    UsdClipboard::Ptr _clipboard;

    // The info messages for the pasted items.
    std::vector<Ufe::PasteClipboardCommand::PasteInfo> _pasteInfos;

}; // UsdPasteClipboardCommand

//! \brief Retrieve the desired selection after the command has executed.
//         \see UsdUndoSelectAfterCommand.
Ufe::Selection USDUFE_PUBLIC getNewSelectionFromCommand(const UsdPasteClipboardCommand& cmd);

} // namespace USDUFE_NS_DEF

#endif // USDCLIPBOARDCOMMANDS_H
