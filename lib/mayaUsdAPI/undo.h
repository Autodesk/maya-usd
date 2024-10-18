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
#ifndef MAYAUSDAPI_UNDO_H
#define MAYAUSDAPI_UNDO_H

#include <mayaUsdAPI/api.h>

#include <pxr/usd/sdf/layer.h>

#include <memory>

namespace MAYAUSDAPI_NS_DEF {

// Forward declarations.
struct UsdUndoableItemImpl;
struct UsdUndoBlockImpl;

//! Tracks layer states by spawning a new UsdUndoStateDelegate.
MAYAUSD_API_PUBLIC
void undoManagerTrackLayerStates(const PXR_NS::SdfLayerHandle& layer);

//! \brief UsdUndoableItem
/*!
    This class stores the list of inverse edit functions that are invoked
    on undo() / redo() call. This is the object that must be placed in DCC's undo stack.
*/
class MAYAUSD_API_PUBLIC UsdUndoableItem
{
public:
    friend struct UsdUndoBlockImpl;

    UsdUndoableItem();
    ~UsdUndoableItem();

    void undo();
    void redo();

private:
    //! Private implementation member
    std::unique_ptr<UsdUndoableItemImpl> _imp;
};

//! \brief UsdUndoBlock collects multiple edits into a single undo operation.
class MAYAUSD_API_PUBLIC UsdUndoBlock
{
public:
    UsdUndoBlock(UsdUndoableItem* undoItem);
    ~UsdUndoBlock();

    UsdUndoBlock(const UsdUndoBlock&) = delete;
    UsdUndoBlock& operator=(const UsdUndoBlock&) = delete;
    UsdUndoBlock(UsdUndoBlock&&) = delete;
    UsdUndoBlock& operator=(UsdUndoBlock&&) = delete;

private:
    //! Private implementation member
    std::unique_ptr<UsdUndoBlockImpl> _imp;
};

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_UNDO_H
