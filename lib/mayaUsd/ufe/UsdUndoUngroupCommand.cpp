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

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsdUtils/util.h>

#include <pxr/usd/usd/editContext.h>

#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/scene.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoUngroupCommand::UsdUndoUngroupCommand(const UsdSceneItem::Ptr& groupItem)
    : Ufe::UndoableCommand()
    , _groupItem(groupItem)
    , _compositeInsertCmd(std::make_shared<Ufe::CompositeUndoableCommand>())
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

    // move group's children one level up
    auto groupHier = Ufe::Hierarchy::hierarchy(_groupItem);
    auto levelUpHier = Ufe::Hierarchy::hierarchy(groupHier->parent());
    for (auto& child : groupHier->children()) {
        auto insertChildCmd = levelUpHier->appendChildCmd(child);
        _compositeInsertCmd->append(insertChildCmd);
    }

    _compositeInsertCmd->execute();

    // remove group prim
    auto status = removePrimGroup();
    TF_VERIFY(status, "Removing group node failed!");
}

void UsdUndoUngroupCommand::undo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.undo();

    // After undo, the group prim loses it's specifier and type.
    // At this point _groupItem->prim() is stale and should not be used.
    // Create a new prim from ufe path and manually set the SetSpecifier and SetTypeName
    // to get back the original concrete xfrom prim.
    auto prim = ufePathToPrim(_groupItem->path());
    TF_AXIOM(prim);

    auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
    TF_AXIOM(primSpec);

    primSpec->SetSpecifier(SdfSpecifierDef);
    primSpec->SetTypeName("Xform");

    // create a new group scene item
    _groupItem = UsdSceneItem::create(_groupItem->path(), prim);
    TF_AXIOM(_groupItem);

    // undo the inserted children
    _compositeInsertCmd->undo();

    // Make sure to add the newly created _group (a.k.a parent) to selection. This matches native
    // Maya behavior and also prevents the crash on grouping a prim twice.
    Ufe::Selection groupSelect;
    groupSelect.append(_groupItem);
    Ufe::GlobalSelection::get()->replaceWith(groupSelect);

    TF_VERIFY(
        Ufe::GlobalSelection::get()->size() == 1,
        "_group node should be in the global selection now. \n");
}

void UsdUndoUngroupCommand::redo()
{
    _compositeInsertCmd->redo();

    _undoableItem.redo();
}

bool UsdUndoUngroupCommand::removePrimGroup()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;
    UsdUndoBlock                         undoBlock(&_undoableItem);

    auto           stage = _groupItem->prim().GetStage();
    UsdEditContext ctx(stage, stage->GetEditTarget().GetLayer());
    return stage->RemovePrim(_groupItem->prim().GetPath());
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
