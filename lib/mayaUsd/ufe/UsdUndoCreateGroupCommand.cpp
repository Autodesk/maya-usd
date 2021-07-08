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
#include "UsdUndoCreateGroupCommand.h"

#include <mayaUsd/ufe/UsdUndoAddNewPrimCommand.h>
#include <mayaUsd/ufe/UsdUndoSetKindCommand.h>

#include <pxr/usd/kind/registry.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoCreateGroupCommand::UsdUndoCreateGroupCommand(
    const UsdSceneItem::Ptr&  parentItem,
    const Ufe::Selection&     selection,
    const Ufe::PathComponent& name)
    : Ufe::InsertChildCommand()
    , _parentItem(parentItem)
    , _name(name)
    , _selection(selection)
    , _groupCompositeCmd(std::make_shared<Ufe::CompositeUndoableCommand>())

{
}

UsdUndoCreateGroupCommand::~UsdUndoCreateGroupCommand() { }

UsdUndoCreateGroupCommand::Ptr UsdUndoCreateGroupCommand::create(
    const UsdSceneItem::Ptr&  parentItem,
    const Ufe::Selection&     selection,
    const Ufe::PathComponent& name)
{
    return std::make_shared<UsdUndoCreateGroupCommand>(parentItem, selection, name);
}

Ufe::SceneItem::Ptr UsdUndoCreateGroupCommand::insertedChild() const { return _groupItem; }

void UsdUndoCreateGroupCommand::execute()
{
    auto addPrimCmd = UsdUndoAddNewPrimCommand::create(_parentItem, _name.string(), "Xform");
    _groupCompositeCmd->append(addPrimCmd);
    addPrimCmd->execute();

    _groupItem = UsdSceneItem::create(addPrimCmd->newUfePath(), addPrimCmd->newPrim());

    // If the parent prim is part of the model hierarchy, set the kind of the
    // newly created group prim to make sure that the model hierarchy remains
    // contiguous.
    const PXR_NS::UsdPrim& parentPrim = _parentItem->prim();
    if (PXR_NS::UsdModelAPI(parentPrim).IsModel()) {
        const PXR_NS::UsdPrim& groupPrim = _groupItem->prim();
        auto setKindCmd = UsdUndoSetKindCommand::create(groupPrim, PXR_NS::KindTokens->group);
        _groupCompositeCmd->append(setKindCmd);
        setKindCmd->execute();
    }

    // Make sure to handle the exception if the parenting operation fails.
    // This scenario happens if a user tries to group prim(s) in a layer
    // other than the one where they were defined. In this case, the group creation itself
    // will succeed, however the re-parenting is expected to throw an exception. We also need to
    // make sure to undo the previous command ( AddNewPrimCommand ) when this happens.
    try {
        auto newParentHierarchy = Ufe::Hierarchy::hierarchy(_groupItem);
        if (newParentHierarchy) {
            for (auto child : _selection) {
                auto parentCmd = newParentHierarchy->appendChildCmd(child);
                _groupCompositeCmd->append(parentCmd);
                parentCmd->execute();
            }
        }

        // Make sure to add the newly created _groupItem (a.k.a parent) to selection. This matches
        // native Maya behavior and also prevents the crash on grouping a prim twice.
        Ufe::Selection groupSelect;
        groupSelect.append(_groupItem);
        Ufe::GlobalSelection::get()->replaceWith(groupSelect);

        TF_VERIFY(
            Ufe::GlobalSelection::get()->size() == 1,
            "_groupItem node should be in the global selection now. \n");
    } catch (...) {
        // undo previous AddNewPrimCommand
        undo();

        throw; // re-throw the same exception
    }
}

void UsdUndoCreateGroupCommand::undo() { _groupCompositeCmd->undo(); }

void UsdUndoCreateGroupCommand::redo() { _groupCompositeCmd->redo(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
