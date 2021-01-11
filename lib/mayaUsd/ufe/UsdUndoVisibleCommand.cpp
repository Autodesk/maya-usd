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
#include "UsdUndoVisibleCommand.h"

#include <mayaUsd/undo/UsdUndoBlock.h>

#include <pxr/usd/usdGeom/imageable.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoVisibleCommand::UsdUndoVisibleCommand(const UsdPrim& prim, bool vis)
    : Ufe::UndoableCommand()
    , _prim(prim)
    , _visible(vis)
{
}

UsdUndoVisibleCommand::~UsdUndoVisibleCommand() { }

UsdUndoVisibleCommand::Ptr UsdUndoVisibleCommand::create(const UsdPrim& prim, bool vis)
{
    if (!prim) {
        return nullptr;
    }
    return std::make_shared<UsdUndoVisibleCommand>(prim, vis);
}

void UsdUndoVisibleCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    _visible ? UsdGeomImageable(_prim).MakeVisible() : UsdGeomImageable(_prim).MakeInvisible();
}

void UsdUndoVisibleCommand::redo() { _undoableItem.redo(); }

void UsdUndoVisibleCommand::undo() { _undoableItem.undo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
