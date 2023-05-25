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
    UsdUndoBlock(UsdUndoableItem* undoItem);
    ~UsdUndoBlock();

    // delete the copy/move constructors assignment operators.
    UsdUndoBlock(const UsdUndoBlock&) = delete;
    UsdUndoBlock& operator=(const UsdUndoBlock&) = delete;
    UsdUndoBlock(UsdUndoBlock&&) = delete;
    UsdUndoBlock& operator=(UsdUndoBlock&&) = delete;

    static uint32_t depth() { return _undoBlockDepth; }

private:
    static uint32_t _undoBlockDepth;

    UsdUndoableItem* _undoItem;
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UNDO_UNDOBLOCK_H
