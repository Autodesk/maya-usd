//
// Copyright 2020 Autodesk
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
#include "UsdUndoBlock.h"

#include "UsdUndoManager.h"

#include <mayaUsd/base/debugCodes.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>

namespace MAYAUSD_NS_DEF {

uint32_t UsdUndoBlock::_undoBlockDepth { 0 };

const MString UsdUndoBlockCmd::commandName { "undoBlockCmd" };

UsdUndoableItem UsdUndoBlockCmd::argUndoItem;

UsdUndoBlock::UsdUndoBlock()
    : UsdUndoBlock(nullptr)
{
}

UsdUndoBlock::UsdUndoBlock(UsdUndoableItem* undoItem)
    : _undoItem(undoItem)
{
    // TfDebug::Enable(USDMAYA_UNDOSTACK);

    TF_DEBUG_MSG(USDMAYA_UNDOSTACK, "--Opening undo block at depth %i\n", _undoBlockDepth);

    ++_undoBlockDepth;
}

UsdUndoBlock::~UsdUndoBlock()
{
    auto& undoManager = UsdUndoManager::instance();

    // decrease the depth
    --_undoBlockDepth;

    if (_undoBlockDepth == 0) {
        if (_undoItem == nullptr) {
            // create an undoable item
            UsdUndoableItem undoItem;
            // transfer edits
            undoManager.transferEdits(undoItem);
            // execute command
            UsdUndoBlockCmd::execute(undoItem);
        } else {
            // transfer edits
            undoManager.transferEdits(*_undoItem);
        }

        TF_DEBUG_MSG(USDMAYA_UNDOSTACK, "Undoable Item adopted the new edits.\n");
    }

    TF_DEBUG_MSG(USDMAYA_UNDOSTACK, "--Closed undo block at depth %i\n", _undoBlockDepth);
}

void UsdUndoBlockCmd::execute(const UsdUndoableItem& undoableItem)
{
    argUndoItem = undoableItem;

    auto status = MGlobal::executeCommand(commandName, true, true);
    if (!status) {
        TF_CODING_ERROR("Executing undoBlock command failed!");
    }

    argUndoItem = UsdUndoableItem();
}

UsdUndoBlockCmd::UsdUndoBlockCmd(const UsdUndoableItem& undoableItem)
    : _undoItem(undoableItem)
{
}

bool UsdUndoBlockCmd::isUndoable() const { return true; }

void* UsdUndoBlockCmd::creator() { return new UsdUndoBlockCmd(std::move(argUndoItem)); }

MStatus UsdUndoBlockCmd::doIt(const MArgList& args)
{
    /* Do nothing */
    return MS::kSuccess;
}

MStatus UsdUndoBlockCmd::redoIt()
{
    _undoItem.redo();

    return MS::kSuccess;
}

MStatus UsdUndoBlockCmd::undoIt()
{
    _undoItem.undo();

    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF