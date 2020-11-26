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

#if UFE_PREVIEW_VERSION_NUM > 2025
#include <mayaUsd/undo/UsdUndoBlock.h>
#endif

#include "private/Utils.h"

#include <mayaUsdUtils/util.h>

#include <ufe/log.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoReorderCommand::UsdUndoReorderCommand(
    const UsdPrim&              parentPrim,
    const std::vector<TfToken>& tokenList)
    : Ufe::UndoableCommand()
    , _parentPrim(parentPrim)
    , _orderedTokens(tokenList)
{
#if UFE_PREVIEW_VERSION_NUM > 2025
    // Apply restriction rules
    for (const auto& childPrim : parentPrim.GetChildren()) {
        ufe::applyCommandRestriction(childPrim, "reorder");
        break;
    }
#endif
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

//HS TODO: Get rif of this ugly guard once PR 121 is out.
#if UFE_PREVIEW_VERSION_NUM > 2025
void UsdUndoReorderCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    const auto& parentPrimSpec = MayaUsdUtils::getPrimSpecAtEditTarget(_parentPrim);
    parentPrimSpec->SetNameChildrenOrder(_orderedTokens);
}

void UsdUndoReorderCommand::undo() { _undoableItem.undo(); }

void UsdUndoReorderCommand::redo() { _undoableItem.redo(); }

#elif
bool UsdUndoReorderCommand::reorder()
{
    const auto& parentPrimSpec = MayaUsdUtils::getPrimSpecAtEditTarget(_parentPrim);

    parentPrimSpec->SetNameChildrenOrder(_orderedTokens);

    return true;
}

void UsdUndoReorderCommand::undo()
{
    try {
        if (!reorder()) {
            UFE_LOG("reorder undo failed");
        }
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}

void UsdUndoReorderCommand::redo()
{
    if (!reorder()) {
        UFE_LOG("reorder redo failed");
    }
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
