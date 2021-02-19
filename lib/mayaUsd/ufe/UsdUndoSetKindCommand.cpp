//
// Copyright 2021 Autodesk
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
#include "UsdUndoSetKindCommand.h"

#include <mayaUsd/undo/UsdUndoBlock.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

#include <memory>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoSetKindCommand::UsdUndoSetKindCommand(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& kind)
    : Ufe::UndoableCommand()
    , _prim(prim)
    , _kind(kind)
{
}

UsdUndoSetKindCommand::Ptr
UsdUndoSetKindCommand::create(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& kind)
{
    if (!prim) {
        return nullptr;
    }

    return std::make_shared<UsdUndoSetKindCommand>(prim, kind);
}

void UsdUndoSetKindCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    PXR_NS::UsdModelAPI(_prim).SetKind(_kind);
}

void UsdUndoSetKindCommand::redo() { _undoableItem.redo(); }

void UsdUndoSetKindCommand::undo() { _undoableItem.undo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
