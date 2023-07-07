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
#include "MayaUsdUndoBlock.h"

#include <usdUfe/base/debugCodes.h>
#include <usdUfe/undo/UsdUndoManager.h>

#include <pxr/base/tf/diagnostic.h>

#include <maya/MGlobal.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

const MString MayaUsdUndoBlockCmd::commandName { "undoBlockCmd" };

UsdUfe::UsdUndoableItem MayaUsdUndoBlockCmd::argUndoItem;

MayaUsdUndoBlock::MayaUsdUndoBlock()
    : UsdUfe::UsdUndoBlock(nullptr)
{
}

MayaUsdUndoBlock::~MayaUsdUndoBlock()
{
    // Will be decremented to 0 by our base class.
    if (depth() == 1) {
        UsdUndoableItem undoItem;
        UsdUfe::UsdUndoManagerAccessor::transferEdits(undoItem);
        MayaUsdUndoBlockCmd::execute(undoItem);

        TF_DEBUG_MSG(USDUFE_UNDOSTACK, "Undoable Item adopted the new edits.\n");
    }
}

void MayaUsdUndoBlockCmd::execute(const UsdUfe::UsdUndoableItem& undoableItem)
{
    PXR_NAMESPACE_USING_DIRECTIVE

    argUndoItem = undoableItem;

    auto status = MGlobal::executeCommand(commandName, true, true);
    if (!status) {
        TF_CODING_ERROR("Executing undoBlock command failed!");
    }

    argUndoItem = UsdUfe::UsdUndoableItem();
}

MayaUsdUndoBlockCmd::MayaUsdUndoBlockCmd(UsdUfe::UsdUndoableItem undoableItem)
    : _undoItem(std::move(undoableItem))
{
}

bool MayaUsdUndoBlockCmd::isUndoable() const { return true; }

void* MayaUsdUndoBlockCmd::creator() { return new MayaUsdUndoBlockCmd(std::move(argUndoItem)); }

MStatus MayaUsdUndoBlockCmd::doIt(const MArgList& args)
{
    /* Do nothing */
    return MS::kSuccess;
}

MStatus MayaUsdUndoBlockCmd::redoIt()
{
    _undoItem.redo();

    return MS::kSuccess;
}

MStatus MayaUsdUndoBlockCmd::undoIt()
{
    _undoItem.undo();

    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
