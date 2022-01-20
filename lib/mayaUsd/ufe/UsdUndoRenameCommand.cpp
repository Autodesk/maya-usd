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
#include "UsdUndoRenameCommand.h"

#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsdUtils/util.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/changeBlock.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/log.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>
#else
#include <cassert>
#endif

#include <cctype>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

/*
    HS, May 15, 2020

    See usd-interest: Question around SdfPrimSepc's SetName routine

    SdfPrimSpec::SetName() will rename any prim in the layer, but it does not allow you to reparent
   the prim, nor will it update any relationship or connection targets in the layer that targeted
   the prim or any of its decendants (they will all break unless you fix them up yourself.Renaming
   and reparenting prims destructively in composed scenes is pretty tricky stuff that cannot really
   practically be done with 100% guarantees.
*/

UsdUndoRenameCommand::UsdUndoRenameCommand(
    const UsdSceneItem::Ptr&  srcItem,
    const Ufe::PathComponent& newName)
    : Ufe::UndoableCommand()
    , _ufeSrcItem(srcItem)
    , _ufeDstItem(nullptr)
    , _stage(_ufeSrcItem->prim().GetStage())
{
    const UsdPrim& prim = _stage->GetPrimAtPath(_ufeSrcItem->prim().GetPath());

    ufe::applyCommandRestriction(prim, "rename");

    // handle unique name for _newName
    _newName = TfMakeValidIdentifier(uniqueChildName(prim.GetParent(), newName.string()));
}

UsdUndoRenameCommand::~UsdUndoRenameCommand() { }

UsdUndoRenameCommand::Ptr
UsdUndoRenameCommand::create(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
{
    return std::make_shared<UsdUndoRenameCommand>(srcItem, newName);
}

UsdSceneItem::Ptr UsdUndoRenameCommand::renamedItem() const { return _ufeDstItem; }

bool UsdUndoRenameCommand::renameRedo()
{
    // get the stage's default prim path
    auto defaultPrimPath = _stage->GetDefaultPrim().GetPath();

    // 1- open a changeblock to delay sending notifications.
    // 2- update the Internal References paths (if any) first
    // 3- set the new name
    // Note: during the changeBlock scope we are still working with old items/paths/prims.
    // it's only after the scope ends that we start working with new items/paths/prims
    {
        SdfChangeBlock changeBlock;

        const UsdPrim& prim = _stage->GetPrimAtPath(_ufeSrcItem->prim().GetPath());

        auto ufeSiblingPath = _ufeSrcItem->path().sibling(Ufe::PathComponent(_newName));
        bool status = MayaUsdUtils::updateReferencedPath(
            prim, SdfPath(ufeSiblingPath.getSegments()[1].string()));
        if (!status) {
            return false;
        }

        // set the new name
        auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
        status = primSpec->SetName(_newName);
        if (!status) {
            return false;
        }
    }

    // the renamed scene item is a "sibling" of its original name.
    _ufeDstItem = createSiblingSceneItem(_ufeSrcItem->path(), _newName);

    // update stage's default prim
    if (_ufeSrcItem->prim().GetPath() == defaultPrimPath) {
        _stage->SetDefaultPrim(_ufeDstItem->prim());
    }

    // send notification to update UFE data model
    sendNotification<Ufe::ObjectRename>(_ufeDstItem, _ufeSrcItem->path());

    return true;
}

bool UsdUndoRenameCommand::renameUndo()
{
    // get the stage's default prim path
    auto defaultPrimPath = _stage->GetDefaultPrim().GetPath();

    // 1- open a changeblock to delay sending notifications.
    // 2- update the Internal References paths (if any) first
    // 3- set the new name
    // Note: during the changeBlock scope we are still working with old items/paths/prims.
    // it's only after the scope ends that we start working with new items/paths/prims
    {
        SdfChangeBlock changeBlock;

        const UsdPrim& prim = _stage->GetPrimAtPath(_ufeDstItem->prim().GetPath());

        auto ufeSiblingPath
            = _ufeSrcItem->path().sibling(Ufe::PathComponent(_ufeSrcItem->prim().GetName()));
        bool status = MayaUsdUtils::updateReferencedPath(
            prim, SdfPath(ufeSiblingPath.getSegments()[1].string()));
        if (!status) {
            return false;
        }

        auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
        status = primSpec->SetName(_ufeSrcItem->prim().GetName());
        if (!status) {
            return false;
        }
    }

    // the renamed scene item is a "sibling" of its original name.
    _ufeSrcItem = createSiblingSceneItem(_ufeDstItem->path(), _ufeSrcItem->prim().GetName());

    // update stage's default prim
    if (_ufeDstItem->prim().GetPath() == defaultPrimPath) {
        _stage->SetDefaultPrim(_ufeSrcItem->prim());
    }

    // send notification to update UFE data model
    sendNotification<Ufe::ObjectRename>(_ufeSrcItem, _ufeDstItem->path());

    return true;
}

void UsdUndoRenameCommand::undo()
{
    try {
        InPathChange pc;
        if (!renameUndo()) {
            UFE_LOG("rename undo failed");
        }
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}

void UsdUndoRenameCommand::redo()
{
    InPathChange pc;
    if (!renameRedo()) {
        UFE_LOG("rename redo failed");
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
