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
#include "UsdUndoReorderCommand.h"

#include "Utils.h"

#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/layers.h>

#include <ufe/log.h>

namespace USDUFE_NS_DEF {

UsdUndoReorderCommand::UsdUndoReorderCommand(
    const UsdPrim&              parentPrim,
    const std::vector<TfToken>& tokenList)
    : Ufe::UndoableCommand()
    , _parentPrim(parentPrim)
    , _orderedTokens(tokenList)
{
    // Apply restriction rules
    for (const auto& childPrim : parentPrim.GetChildren()) {
        applyCommandRestriction(childPrim, "reorder");
        break;
    }
}

UsdUndoReorderCommand::~UsdUndoReorderCommand() { }

UsdUndoReorderCommand::Ptr
UsdUndoReorderCommand::create(const UsdPrim& parentPrim, const std::vector<TfToken>& tokenList)
{
    if (!parentPrim) {
        return nullptr;
    }
    return std::make_shared<UsdUndoReorderCommand>(parentPrim, tokenList);
}

void UsdUndoReorderCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Do the reordering in the target layer and all other applicable layers,
    // which, due to command restrictions that have been verified when the
    // command was created, should only be session layers.
    const auto   newOrder = _orderedTokens;
    PrimSpecFunc reorderFunc
        = [newOrder](const UsdPrim&, const SdfPrimSpecHandle& primSpec) -> void {
        primSpec->SetNameChildrenOrder(newOrder);
    };
    applyToAllPrimSpecs(_parentPrim, reorderFunc);
}

void UsdUndoReorderCommand::undo() { _undoableItem.undo(); }

void UsdUndoReorderCommand::redo() { _undoableItem.redo(); }

} // namespace USDUFE_NS_DEF
