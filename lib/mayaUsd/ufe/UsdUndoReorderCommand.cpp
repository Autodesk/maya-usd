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

#include <ufe/log.h>

#include <mayaUsdUtils/util.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoReorderCommand::UsdUndoReorderCommand(const UsdPrim& child, const std::vector<TfToken>& tokenList)
    : Ufe::UndoableCommand()
    , _childPrim(child)
    , _orderedTokens(tokenList)
{
}

UsdUndoReorderCommand::~UsdUndoReorderCommand()
{
}

UsdUndoReorderCommand::Ptr UsdUndoReorderCommand::create(const UsdPrim& child, const std::vector<TfToken>& tokenList)
{
    if (!child) {
        return nullptr;
    }
    return std::make_shared<UsdUndoReorderCommand>(child, tokenList);
}

bool UsdUndoReorderCommand::reorder()
{
    const auto& parentPrim = _childPrim.GetParent();
    const auto& parentPrimSpec = MayaUsdUtils::getPrimSpecAtEditTarget(parentPrim);

    parentPrimSpec->SetNameChildrenOrder(_orderedTokens);

    return true;
}

void UsdUndoReorderCommand::undo()
{
    try {
        if (!reorder()) {
            UFE_LOG("reorder undo failed");
        }
    }
    catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw;  // re-throw the same exception
    }
}

void UsdUndoReorderCommand::redo()
{
    if (!reorder()) {
        UFE_LOG("reorder redo failed");
    }
}

} // namespace ufe
} // namespace MayaUsd
