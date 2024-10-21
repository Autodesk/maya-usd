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

#include "undo.h"

#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/undo/UsdUndoableItem.h>

namespace MAYAUSDAPI_NS_DEF {

void undoManagerTrackLayerStates(const PXR_NS::SdfLayerHandle& layer)
{
    UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
}

struct UsdUndoableItemImpl
{
    UsdUfe::UsdUndoableItem _item;
};

struct UsdUndoBlockImpl
{
    UsdUndoBlockImpl(UsdUndoableItem* undoItem)
        : _block(&(undoItem->_imp->_item))
    {
    }

    UsdUfe::UsdUndoBlock _block;
};

UsdUndoableItem::UsdUndoableItem()
    : _imp(new UsdUndoableItemImpl())
{
}

// When using a pimpl you need to define the destructor here in the
// .cpp so the compiler has access to the impl.
UsdUndoableItem::~UsdUndoableItem() = default;

void UsdUndoableItem::undo() { _imp->_item.undo(); }

void UsdUndoableItem::redo() { _imp->_item.redo(); }

UsdUndoBlock::UsdUndoBlock(UsdUndoableItem* undoItem)
    : _imp(new UsdUndoBlockImpl(undoItem))
{
}

// When using a pimpl you need to define the destructor here in the
// .cpp so the compiler has access to the impl.
UsdUndoBlock::~UsdUndoBlock() = default;

} // namespace MAYAUSDAPI_NS_DEF
