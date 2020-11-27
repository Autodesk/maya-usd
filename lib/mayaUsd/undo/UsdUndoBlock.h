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

#ifndef MAYAUSD_UNDO_UNDOBLOCK_H
#define MAYAUSD_UNDO_UNDOBLOCK_H

#include "UsdUndoableItem.h"

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/declarePtrs.h>

#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

TF_DECLARE_WEAK_AND_REF_PTRS(UsdUndoManager);

//! \brief UsdUndoBlock collects multiple edits into a single undo operation.
/*!
 */
class MAYAUSD_CORE_PUBLIC UsdUndoBlock
{
public:
    UsdUndoBlock();
    UsdUndoBlock(UsdUndoableItem* undoItem);
    ~UsdUndoBlock();

    // delete the copy/move constructors assignment operators.
    UsdUndoBlock(const UsdUndoBlock&) = delete;
    UsdUndoBlock& operator=(const UsdUndoBlock&) = delete;
    UsdUndoBlock(UsdUndoBlock&&) = delete;
    UsdUndoBlock& operator=(UsdUndoBlock&&) = delete;

    static uint32_t depth() { return _undoBlockDepth; }

private:
    friend class UsdUndoManager;

    static uint32_t _undoBlockDepth;

    UsdUndoableItem* _undoItem;
};

//! \brief UsdUndoBlockCmd is used to collect USD edits inside Maya MPxCommand.
/*!
 */
class MAYAUSD_CORE_PUBLIC UsdUndoBlockCmd : public MPxCommand
{
public:
    UsdUndoBlockCmd(const UsdUndoableItem& undoableItem);
    static void* creator();

    static void            execute(const UsdUndoableItem& undoableItem);
    static UsdUndoableItem argUndoItem;
    static const MString   commandName;

    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
    MStatus undoIt() override;
    bool    isUndoable() const override;

private:
    UsdUndoableItem _undoItem;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_UNDOBLOCK_H
