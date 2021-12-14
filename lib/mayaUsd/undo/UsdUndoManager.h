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

#ifndef MAYAUSD_UNDO_UNDOMANAGER_H
#define MAYAUSD_UNDO_UNDOMANAGER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/undo/OpUndoItemList.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/usd/sdf/layer.h>

#include <functional>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

class OpUndoItemMuting;

//! \brief Singleton class to manage layer states.
/*!
    The UndoManager is responsible for :
    1- tracking layer state changes from UsdUndoStateDelegate
    2- collecting InvertFunc() in every state change
    3- transferring collected edits into an UsdUndoableItem
*/
class MAYAUSD_CORE_PUBLIC UsdUndoManager
{
public:
    using InvertFunc = std::function<void()>;
    using InvertFuncs = std::vector<InvertFunc>;

    // returns an instance of the undo manager.
    static UsdUndoManager& instance();

    // delete the copy/move constructors assignment operators.
    UsdUndoManager(const UsdUndoManager&) = delete;
    UsdUndoManager& operator=(const UsdUndoManager&) = delete;
    UsdUndoManager(UsdUndoManager&&) = delete;
    UsdUndoManager& operator=(UsdUndoManager&&) = delete;

    // tracks layer states by spawning a new UsdUndoStateDelegate
    void trackLayerStates(const SdfLayerHandle& layer);

    // Retrieve the operation undo info, used to record undo items.
    // The undo info can later be extracted into a command to implement
    // its undo and redo. See: OpUndoItemList::extract()
    OpUndoItemList& getUndoInfo() { return _undoInfo; }

private:
    friend class UsdUndoStateDelegate;
    friend class UsdUndoBlock;
    friend class UsdUndoableItem;
    friend class OpUndoItemMuting;

    UsdUndoManager() = default;
    ~UsdUndoManager() = default;

    void addInverse(InvertFunc func);
    void transferEdits(UsdUndoableItem& undoableItem);

private:
    InvertFuncs _invertFuncs;
    OpUndoItemList  _undoInfo;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UNDO_UNDOMANAGER_H
