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

#include "UsdUndoableItem.h"

#include "UsdUndoBlock.h"

#include <pxr/usd/sdf/changeBlock.h>

namespace MAYAUSD_NS_DEF {

void UsdUndoableItem::undo() { doInvert(); }

void UsdUndoableItem::redo() { doInvert(); }

void UsdUndoableItem::doInvert()
{
    if (UsdUndoBlock::depth() != 0) {
        TF_CODING_ERROR("Inversion during open edit block may result in corrupted undo "
                        "stack.");
    }

    UsdUndoBlock undoBlock(this);

    // call invert functions in reverse order
    {
        SdfChangeBlock changeBlock;
        for (auto it = _invertFuncs.rbegin(); it != _invertFuncs.rend(); ++it) {
            (*it)();
        }
    }
}

} // namespace MAYAUSD_NS_DEF