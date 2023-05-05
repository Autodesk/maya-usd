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
#include "UsdUndoClipboardCommand.h"

#include "UsdUndoDeleteCommand.h"
#include "UsdUndoDuplicateSelectionCommand.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>

#include <ufe/pathString.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

static constexpr char stageClipboard[] = "stageClipboard";

void deleteStage(const Ufe::Path& stageUfePath)
{
    auto splittedStrings = splitString(stageUfePath.string(), "|");

    if (splittedStrings.size() >= 1) {
        MString script;
        script.format("delete \"^1s\"", splittedStrings[1].c_str());

        MStatus status = MGlobal::executeCommand(script, false, false);
        if (!status) {
            const std::string error = TfStringPrintf(
                "Failed to delete Clipboard stage with ufe path: \"%s\".", stageUfePath);
            throw std::runtime_error(error);
        }
    }
}

void hideStage(const Ufe::Path& stageUfePath)
{
    auto splittedStrings = splitString(stageUfePath.string(), "|");
    if (splittedStrings.size() >= 1) {
        MString script;
        script.format("setAttr ^1s.hiddenInOutliner 1;", splittedStrings[1].c_str());
        MStatus status = MGlobal::executeCommand(script, false, false);

        if (!status) {
            const std::string error = TfStringPrintf(
                "Failed to hide Clipboard stage with ufe path: \"%s\".", stageUfePath);
            throw std::runtime_error(error);
        }
    }
}

} // namespace

UsdUndoCopyClipboardCommand::UsdUndoCopyClipboardCommand(const Ufe::Selection& selection)
    : Ufe::UndoableCommand()
    , _selection(selection)
{
}

UsdUndoCopyClipboardCommand::~UsdUndoCopyClipboardCommand() { }

UsdUndoCopyClipboardCommand::Ptr
UsdUndoCopyClipboardCommand::create(const Ufe::Selection& selection)
{
    auto retVal = std::make_shared<UsdUndoCopyClipboardCommand>(selection);

    if (retVal->_selection.empty()) {
        return {};
    }

    return retVal;
}

void UsdUndoCopyClipboardCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Step 1. Create a stage for the Clipboard.
    MString script;
    script.format("createNode -name \"^1s\" -ss \"^2s\"", stageClipboard, "mayaUsdProxyShape");

    std::string cmdResult
        = MGlobal::executeCommandStringResult(script, /*display = */ false, /* undoable = */ false)
              .asChar();

    if (cmdResult.length() == 0) {
        const std::string error = TfStringPrintf("Failed to create Clipboard stage.");
        throw std::runtime_error(error);
    }

    // Get the newly created stage.
    std::string ufePathString = "|world|";
    ufePathString
        += cmdResult == stageClipboard ? "mayaUsdProxy1|" + std::string(stageClipboard) : cmdResult;
    Ufe::Path       ufeClipboardPath = Ufe::PathString::path(ufePathString);
    UsdStageWeakPtr clipboardStage = ufe::getStage(ufeClipboardPath);

    if (!clipboardStage) {
        const std::string error
            = TfStringPrintf("Cannot find Clipboard stage for ufe path: \"%s\".", ufeClipboardPath);
        throw std::runtime_error(error);
    }

    // Step 2. Hide the Clipboard stage in the outliner.
    hideStage(ufeClipboardPath);

    // Step 3. Duplicate the selected items to the Clipboard stage using its pseudo-root as parent
    // item destination.
    auto usdParentItem = UsdSceneItem::create(ufeClipboardPath, clipboardStage->GetPseudoRoot());
    auto duplicateSelectionUndoableCmd = UsdUndoDuplicateSelectionCommand::create(
        _selection, Ufe::ValueDictionary(), usdParentItem);
    duplicateSelectionUndoableCmd->execute();

    // Step 4. Export the Clipboard stage.
    const auto tmpDir = std::string(getenv("TMPDIR")) + "/Clipboard.usda";

    if (!clipboardStage->Export(tmpDir)) {
        const std::string error = TfStringPrintf(
            "Failed to export Clipboard stage with ufe path: \"%s\".", ufeClipboardPath);
        throw std::runtime_error(error);
    }

    // Step 5. Delete the Clipboard stage.
    deleteStage(ufeClipboardPath);
}

void UsdUndoCopyClipboardCommand::undo() { _undoableItem.undo(); }

void UsdUndoCopyClipboardCommand::redo() { _undoableItem.redo(); }

UsdUndoCutClipboardCommand::UsdUndoCutClipboardCommand(const Ufe::Selection& selection)
    : Ufe::UndoableCommand()
    , _selection(selection)
{
}

UsdUndoCutClipboardCommand::~UsdUndoCutClipboardCommand() { }

UsdUndoCutClipboardCommand::Ptr UsdUndoCutClipboardCommand::create(const Ufe::Selection& selection)
{
    return std::make_shared<UsdUndoCutClipboardCommand>(selection);
}

void UsdUndoCutClipboardCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Step 1. Copy the selected items to the Clipboard.
    auto copyClipboardCommand = UsdUndoCopyClipboardCommand::create(_selection);
    copyClipboardCommand->execute();

    // Step 2. Delete the selected items.
    for (auto&& item : _selection) {
        UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
        if (!usdItem) {
            continue;
        }

        auto deleteCmd = UsdUndoDeleteCommand::create(usdItem->prim());
        deleteCmd->execute();
    }
}

void UsdUndoCutClipboardCommand::undo() { _undoableItem.undo(); }

void UsdUndoCutClipboardCommand::redo() { _undoableItem.redo(); }

UsdUndoPasteClipboardCommand::Ptr
UsdUndoPasteClipboardCommand::create(const Ufe::SceneItem::Ptr& dstParentItem)
{
    auto retVal = std::make_shared<UsdUndoPasteClipboardCommand>(dstParentItem);

    if (retVal->_dstParentItem) {
        return retVal;
    }
    return {};
}

void UsdUndoPasteClipboardCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Step 1. Load and open the Clipboard stage.
    const auto tmpDir = std::string(getenv("TMPDIR")) + "/Clipboard.usda";

    MString script;
    script.format("mayaUsd_createStageFromFilePath \"^1s\"", tmpDir.c_str());

    std::string cmdResult
        = MGlobal::executeCommandStringResult(script, /*display = */ false, /* undoable = */ false)
              .asChar();

    if (cmdResult.length() == 0) {
        const std::string error
            = TfStringPrintf("Failed to load Clipboard stage in dir: \"%s\".", tmpDir);
        throw std::runtime_error(error);
    }

    // Get the newly created stage.
    const std::string ufePathString = "|world" + cmdResult;
    Ufe::Path         ufeClipboardPath = Ufe::PathString::path(ufePathString);
    UsdStageWeakPtr   clipboardStage = ufe::getStage(ufeClipboardPath);

    if (!clipboardStage) {
        const std::string error
            = TfStringPrintf("Cannot find Clipboard stage for ufe path: \"%s\".", ufeClipboardPath);
        throw std::runtime_error(error);
    }

    // Step 2. Hide the Clipboard stage in the outliner.
    hideStage(ufeClipboardPath);

    // Step 3. Duplicate the first-level in depth items from the Clipboard stage to the destination
    // parent item.

    Ufe::Selection selection;
    for (auto prim : clipboardStage->Traverse()) {
        // Step 3.1 Add to the selection only the first-level in depth items.
        if (prim.GetParent() == clipboardStage->GetPseudoRoot()) {
            auto ufeChildPath = Ufe::PathString::path(
                ufeClipboardPath.string() + ",/" + prim.GetName().GetString());
            auto usdItem = UsdSceneItem::create(ufeChildPath, prim);
            selection.append(usdItem);
        }
    }

    _selectionUndoableCmd = UsdUndoDuplicateSelectionCommand::create(
        selection, Ufe::ValueDictionary(), _dstParentItem);
    _selectionUndoableCmd->execute();

    // Step 4. Delete the Clipboard stage.
    deleteStage(ufeClipboardPath);
}

void UsdUndoPasteClipboardCommand::undo() { _undoableItem.undo(); }

void UsdUndoPasteClipboardCommand::redo() { _undoableItem.redo(); }

Ufe::SceneItem::Ptr UsdUndoPasteClipboardCommand::targetItem(const Ufe::Path& sourcePath) const
{
    if (_selectionUndoableCmd) {
        return _selectionUndoableCmd->targetItem(sourcePath);
    }
    return {};
}

std::vector<Ufe::SceneItem::Ptr> UsdUndoPasteClipboardCommand::targetItems() const
{
    if (_selectionUndoableCmd) {
        return _selectionUndoableCmd->targetItems();
    }
    return {};
}

UsdUndoPasteClipboardCommand::UsdUndoPasteClipboardCommand(const Ufe::SceneItem::Ptr& dstParentItem)
    : Ufe::SelectionUndoableCommand()
{
    _dstParentItem = std::dynamic_pointer_cast<UsdSceneItem>(dstParentItem);
}
UsdUndoPasteClipboardCommand::~UsdUndoPasteClipboardCommand() { }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
