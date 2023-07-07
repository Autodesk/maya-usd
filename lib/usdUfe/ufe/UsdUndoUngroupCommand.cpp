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
#include "UsdUndoUngroupCommand.h"

#include "private/UfeNotifGuard.h"

#include <usdUfe/undo/UsdUndoBlock.h>

#include <pxr/usd/usd/editContext.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

UsdUndoUngroupCommand::UsdUndoUngroupCommand(const UsdSceneItem::Ptr& groupItem)
    : Ufe::UndoableCommand()
    , _groupItem(groupItem)
{
}

UsdUndoUngroupCommand::~UsdUndoUngroupCommand() { }

UsdUndoUngroupCommand::Ptr UsdUndoUngroupCommand::create(const UsdSceneItem::Ptr& groupItem)
{
    return std::make_shared<UsdUndoUngroupCommand>(groupItem);
}

void UsdUndoUngroupCommand::execute()
{
    // "Ungrouping" means moving group's children up a level in hierarchy
    // followed by group node getting removed.

    // Handling insertion (a.k.a move ) is best to be done on DCC side to cover
    // all possible flags ( absolute, relative, world, parent ).

    // remove group prim
    UsdUfe::InAddOrDeleteOperation ad;
    UsdUndoBlock                   undoBlock(&_undoableItem);

    auto           stage = _groupItem->prim().GetStage();
    UsdEditContext ctx(stage, stage->GetEditTarget().GetLayer());
    auto           retVal = stage->RemovePrim(_groupItem->prim().GetPath());
    TF_VERIFY(retVal, "Failed to remove '%s'", _groupItem->prim().GetPath().GetText());
}

void UsdUndoUngroupCommand::undo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoUngroupCommand::redo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}

} // namespace USDUFE_NS_DEF
