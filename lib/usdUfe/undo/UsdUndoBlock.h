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

#ifndef USDUFE_UNDO_UNDOBLOCK_H
#define USDUFE_UNDO_UNDOBLOCK_H

#include <usdUfe/base/api.h>
#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/base/tf/declarePtrs.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

TF_DECLARE_WEAK_AND_REF_PTRS(UsdUndoManager);

//! \brief UsdUndoBlock collects multiple edits into a single undo operation.
/*!
 */
class USDUFE_PUBLIC UsdUndoBlock
{
public:
    /// @brief Create an undo block that will capture all undo into the given undo item.
    /// @param undoItem the item to receive the undos.
    /// @param extraEdits if true, the undos are added the item, even if the item already contained
    /// undos.
    ///                   Otherwise, any undos that were already in the items are discarded.
    UsdUndoBlock(UsdUndoableItem* undoItem, bool extraEdits = false);
    virtual ~UsdUndoBlock();

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoBlock);

    static uint32_t depth() { return _undoBlockDepth; }

private:
    static uint32_t _undoBlockDepth;

    UsdUndoableItem* _undoItem;
    bool             _extraEdits;
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UNDO_UNDOBLOCK_H
