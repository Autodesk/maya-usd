//
// Copyright 2019 Autodesk
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
#include "private/Utils.h"

#include <ufe/log.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/sdf/changeBlock.h>

#include <mayaUsd/ufe/Utils.h>

#include <mayaUsdUtils/util.h>

#include "private/UfeNotifGuard.h"

#include <iostream>

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoReorderCommand::UsdUndoReorderCommand(const UsdSceneItem::Ptr& child, const std::vector<TfToken>& tokenList)
    : Ufe::UndoableCommand()
    , _childPath(child->path())
    , _orderedTokens(tokenList)
{
}

UsdUndoReorderCommand::~UsdUndoReorderCommand()
{
}

UsdUndoReorderCommand::Ptr UsdUndoReorderCommand::create(const UsdSceneItem::Ptr& child, const std::vector<TfToken>& tokenList)
{
    if (!child) {
        return nullptr;
    }
    return std::make_shared<UsdUndoReorderCommand>(child, tokenList);
}

bool UsdUndoReorderCommand::reorderRedo()
{
    const auto& childPrim = ufePathToPrim(_childPath);
    const auto& parentPrim = childPrim.GetParent();
    const auto& parentPrimSpec = MayaUsdUtils::getPrimSpecAtEditTarget(parentPrim);

    parentPrimSpec->SetNameChildrenOrder(_orderedTokens);

    return true;
}

bool UsdUndoReorderCommand::reorderUndo()
{
    const auto& childPrim = ufePathToPrim(_childPath);
    const auto& parentPrim = childPrim.GetParent();
    const auto& parentPrimSpec = MayaUsdUtils::getPrimSpecAtEditTarget(parentPrim);

    parentPrimSpec->SetNameChildrenOrder(_orderedTokens);

    return true;
}

void UsdUndoReorderCommand::undo()
{
    try {
        if (!reorderUndo()) {
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
    if (!reorderRedo()) {
        UFE_LOG("reorder redo failed");
    }
}

} // namespace ufe
} // namespace MayaUsd
