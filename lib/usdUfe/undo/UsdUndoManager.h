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

#ifndef USDUFE_UNDO_UNDOMANAGER_H
#define USDUFE_UNDO_UNDOMANAGER_H

#include <usdUfe/base/api.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/sdf/layer.h>

#include <functional>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

//! \brief Singleton class to manage layer states.
/*!
    The UndoManager is responsible for :
    1- tracking layer state changes from UsdUndoStateDelegate
    2- collecting InvertFunc() in every state change
    3- transferring collected edits into an UsdUndoableItem
*/
class USDUFE_PUBLIC UsdUndoManager
{
public:
    // returns an instance of the undo manager.
    static UsdUndoManager& instance();

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoManager);

    // tracks layer states by spawning a new UsdUndoStateDelegate
    void trackLayerStates(const SdfLayerHandle& layer);

private:
    friend class UsdUndoManagerAccessor;

    UsdUndoManager() = default;
    ~UsdUndoManager() = default;

    void addInverse(UsdUndoableItem::InvertFunc func);
    void transferEdits(UsdUndoableItem& undoableItem, bool extraEdits);

private:
    UsdUndoableItem::InvertFuncs _invertFuncs;
};

//! \brief Helper struct which exists only to provide controlled,
//!        deliberate access to UsdUndoManager addInverse/transferEdits
//!        private methods.
class USDUFE_PUBLIC UsdUndoManagerAccessor
{
public:
    // Delete everything.
    UsdUndoManagerAccessor() = delete;
    ~UsdUndoManagerAccessor() = delete;
    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoManagerAccessor);

    static void addInverse(UsdUndoableItem::InvertFunc func)
    {
        auto& undoManager = UsdUfe::UsdUndoManager::instance();
        undoManager.addInverse(func);
    }
    static void transferEdits(UsdUndoableItem& undoableItem, bool extraEdits = false)
    {
        auto& undoManager = UsdUfe::UsdUndoManager::instance();
        undoManager.transferEdits(undoableItem, extraEdits);
    }
};

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UNDO_UNDOMANAGER_H
