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

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/loadRules.h>
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
#include <ufe/path.h>
#include <ufe/pathSegment.h>
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
#if (UFE_PREVIEW_VERSION_NUM >= 4041)
    : Ufe::SceneItemResultUndoableCommand()
#else
    : Ufe::UndoableCommand()
#endif
    , _ufeSrcItem(srcItem)
    , _ufeDstItem(nullptr)
    , _stage(_ufeSrcItem->prim().GetStage())
{
    const UsdPrim prim = _stage->GetPrimAtPath(_ufeSrcItem->prim().GetPath());

    ufe::applyCommandRestriction(prim, "rename");

    // Handle trailing #: convert it to a number which will be increased as needed.
    // Increasing the number to make it unique is handled in the function uniqueChildName
    // below.
    //
    // Converting the # must be done before calling TfMakeValidIdentifier as it would
    // convert it to a an underscore ('_').
    std::string newNameStr = newName.string();
    if (newNameStr.size() > 0 && newNameStr.back() == '#') {
        newNameStr.back() = '1';
    }

    // handle unique name for _newName If the name has not changed,
    // the command does nothing and the destination item is the same
    // as the source item.
    const std::string validNewName = TfMakeValidIdentifier(newNameStr);
    if (validNewName != prim.GetName())
        _newName = uniqueChildName(prim.GetParent(), validNewName);
    else
        _ufeDstItem = srcItem;
}

UsdUndoRenameCommand::~UsdUndoRenameCommand() { }

UsdUndoRenameCommand::Ptr
UsdUndoRenameCommand::create(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
{
    return std::make_shared<UsdUndoRenameCommand>(srcItem, newName);
}

UsdSceneItem::Ptr UsdUndoRenameCommand::renamedItem() const { return _ufeDstItem; }

static void sendNotificationToAllStageProxies(
    const UsdStagePtr& stage,
    const UsdPrim&     prim,
    const Ufe::Path&   srcPath,
    const Ufe::Path&   dstPath)
{
    const Ufe::Rtid mayaId = getMayaRunTimeId();
    for (const std::string& proxyName : ProxyShapeHandler::getAllNames()) {
        UsdStagePtr proxyStage = ProxyShapeHandler::dagPathToStage(proxyName);
        if (proxyStage != stage)
            continue;

        const Ufe::PathSegment proxySegment(std::string("|world") + proxyName, mayaId, '|');

        const Ufe::PathSegment& srcUsdSegment = srcPath.getSegments()[1];
        const Ufe::Path adjustedSrcPath(Ufe::Path::Segments({ proxySegment, srcUsdSegment }));

        const Ufe::PathSegment& dstUsdSegment = dstPath.getSegments()[1];
        const Ufe::Path adjustedDstPath(Ufe::Path::Segments({ proxySegment, dstUsdSegment }));

        UsdSceneItem::Ptr newItem = UsdSceneItem::create(adjustedDstPath, prim);

        sendNotification<Ufe::ObjectRename>(newItem, adjustedSrcPath);
    }
}

static bool doUsdRename(
    const UsdStagePtr& stage,
    const UsdPrim&     prim,
    std::string        newName,
    const Ufe::Path    srcPath,
    const Ufe::Path    dstPath)
{
    // 1- open a changeblock to delay sending notifications.
    // 2- update the Internal References paths (if any) first
    // 3- set the new name
    // Note: during the changeBlock scope we are still working with old items/paths/prims.
    // it's only after the scope ends that we start working with new items/paths/prims
    SdfChangeBlock changeBlock;

    bool status
        = MayaUsdUtils::updateReferencedPath(prim, SdfPath(dstPath.getSegments()[1].string()));
    if (!status)
        return false;

    // Make sure the load state of the renamed prim will be preserved.
    // We copy all rules that applied to it specifically and remove the rules
    // that applied to it specifically.
    {
        auto fromPath = SdfPath(srcPath.getSegments()[1].string());
        auto destPath = SdfPath(dstPath.getSegments()[1].string());
        duplicateLoadRules(*stage, fromPath, destPath);
        removeRulesForPath(*stage, fromPath);
    }

    // set the new name
    auto primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
    status = primSpec->SetName(newName);
    if (!status)
        return false;

    return true;
}

bool UsdUndoRenameCommand::renameRedo()
{
    // If the new name is the same as the current name, do nothing.
    // This is the same behavior as the Maya rename command for Maya nodes.
    if (_newName.empty())
        return true;

    // get the stage's default prim path
    auto defaultPrimPath = _stage->GetDefaultPrim().GetPath();

    const Ufe::Path srcPath = _ufeSrcItem->path();
    const Ufe::Path dstPath = srcPath.sibling(Ufe::PathComponent(_newName));
    // Note: must fetch prim again from its path because undo/redo of composite commands
    //       (or doing multiple undo and then multiple redo) can make the cached prim
    //       stale.
    const UsdPrim prim = _stage->GetPrimAtPath(_ufeSrcItem->prim().GetPath());

    if (!doUsdRename(_stage, prim, _newName, srcPath, dstPath))
        return false;

    // the renamed scene item is a "sibling" of its original name.
    _ufeDstItem = createSiblingSceneItem(srcPath, _newName);

    // update stage's default prim
    if (_ufeSrcItem->prim().GetPath() == defaultPrimPath) {
        _stage->SetDefaultPrim(_ufeDstItem->prim());
    }

    // send notification to update UFE data model
    sendNotificationToAllStageProxies(_stage, prim, srcPath, dstPath);

    return true;
}

bool UsdUndoRenameCommand::renameUndo()
{
    // If the new name is the same as the current name, do nothing.
    // This is the same behavior as the Maya rename command for Maya nodes.
    if (_newName.empty())
        return true;

    // get the stage's default prim path
    auto defaultPrimPath = _stage->GetDefaultPrim().GetPath();

    const Ufe::Path srcPath = _ufeDstItem->path();
    const Ufe::Path dstPath = _ufeSrcItem->path();
    // Note: must fetch prim again from its path because undo/redo of composite commands
    //       (or doing multiple undo and then multiple redo) can make the cached prim
    //       stale.
    const UsdPrim     prim = _stage->GetPrimAtPath(_ufeDstItem->prim().GetPath());
    const std::string newName = _ufeSrcItem->prim().GetName();

    if (!doUsdRename(_stage, prim, newName, srcPath, dstPath))
        return false;

    // the renamed scene item is a "sibling" of its original name.
    _ufeSrcItem = createSiblingSceneItem(_ufeDstItem->path(), _ufeSrcItem->prim().GetName());

    // update stage's default prim
    if (_ufeDstItem->prim().GetPath() == defaultPrimPath) {
        _stage->SetDefaultPrim(_ufeSrcItem->prim());
    }

    // send notification to update UFE data model
    sendNotificationToAllStageProxies(_stage, prim, srcPath, dstPath);

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
