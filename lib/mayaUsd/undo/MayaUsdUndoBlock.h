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

#ifndef MAYAUSD_UNDO_UNDOBLOCK_H
#define MAYAUSD_UNDO_UNDOBLOCK_H

#include <mayaUsd/base/api.h>

#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <maya/MPxCommand.h>

namespace MAYAUSD_NS_DEF {

//! \brief MayaUsdUndoBlock collects multiple edits into a single undo operation.
/*!
 */
class MAYAUSD_CORE_PUBLIC MayaUsdUndoBlock : public UsdUfe::UsdUndoBlock
{
public:
    MayaUsdUndoBlock();
    ~MayaUsdUndoBlock() override;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(MayaUsdUndoBlock);
};

//! \brief MayaUsdUndoBlockCmd is used to collect USD edits inside Maya MPxCommand.
/*!
 */
class MAYAUSD_CORE_PUBLIC MayaUsdUndoBlockCmd : public MPxCommand
{
public:
    MayaUsdUndoBlockCmd(UsdUfe::UsdUndoableItem undoableItem);
    static void* creator();

    static void                    execute(const UsdUfe::UsdUndoableItem& undoableItem);
    static UsdUfe::UsdUndoableItem argUndoItem;
    static const MString           commandName;

    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
    MStatus undoIt() override;
    bool    isUndoable() const override;

private:
    UsdUfe::UsdUndoableItem _undoItem;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_UNDOBLOCK_H
