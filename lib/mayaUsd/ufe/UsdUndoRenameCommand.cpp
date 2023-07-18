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

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/loadRules.h>
#include <usdUfe/utils/usdUtils.h>

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

#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>

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
#ifdef UFE_V4_FEATURES_AVAILABLE
    : Ufe::SceneItemResultUndoableCommand()
#else
    : Ufe::UndoableCommand()
#endif
    , _ufeSrcItem(srcItem)
    , _ufeDstItem(nullptr)
    , _stage(_ufeSrcItem->prim().GetStage())
{
    const UsdPrim prim = _stage->GetPrimAtPath(_ufeSrcItem->prim().GetPath());

    UsdUfe::applyCommandRestriction(prim, "rename");

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

namespace {

void sendNotificationToAllStageProxies(
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

        // For all the proxy shapes that are mapping the same stage, we need to fixup the
        // Ufe path since they have different Ufe Paths because it contains proxy shape name.
        const Ufe::PathSegment proxySegment(std::string("|world") + proxyName, mayaId, '|');

        const Ufe::PathSegment& srcUsdSegment = srcPath.getSegments()[1];
        const Ufe::Path adjustedSrcPath(Ufe::Path::Segments({ proxySegment, srcUsdSegment }));

        const Ufe::PathSegment& dstUsdSegment = dstPath.getSegments()[1];
        const Ufe::Path adjustedDstPath(Ufe::Path::Segments({ proxySegment, dstUsdSegment }));

        UsdSceneItem::Ptr newItem = UsdSceneItem::create(adjustedDstPath, prim);

        sendNotification<Ufe::ObjectRename>(newItem, adjustedSrcPath);
    }
}

void doUsdRename(
    const UsdStagePtr& stage,
    const UsdPrim&     prim,
    std::string        newName,
    const Ufe::Path    srcPath,
    const Ufe::Path    dstPath)
{
    enforceMutedLayer(prim, "rename");

    // 1- open a changeblock to delay sending notifications.
    // 2- update the Internal References paths (if any) first
    // 3- set the new name
    // Note: during the changeBlock scope we are still working with old items/paths/prims.
    // it's only after the scope ends that we start working with new items/paths/prims
    SdfChangeBlock changeBlock;

    if (!UsdUfe::updateReferencedPath(prim, SdfPath(dstPath.getSegments()[1].string()))) {
        const std::string error = TfStringPrintf(
            "Failed to update references to prim \"%s\".", prim.GetPath().GetText());
        TF_WARN("%s", error.c_str());
        throw std::runtime_error(error);
    }

    // Make sure the load state of the renamed prim will be preserved.
    // We copy all rules that applied to it specifically and remove the rules
    // that applied to it specifically.
    {
        auto fromPath = SdfPath(srcPath.getSegments()[1].string());
        auto destPath = SdfPath(dstPath.getSegments()[1].string());
        duplicateLoadRules(*stage, fromPath, destPath);
        removeRulesForPath(*stage, fromPath);
    }

    // Do the renaming in the target layer and all other applicable layers,
    // which, due to command restrictions that have been verified when the
    // command was created, should only be session layers.
    PrimSpecFunc renameFunc
        = [&newName](const UsdPrim& prim, const SdfPrimSpecHandle& primSpec) -> void {
        if (!primSpec->SetName(newName)) {
            const std::string error
                = TfStringPrintf("Failed to rename \"%s\".", prim.GetPath().GetText());
            TF_WARN("%s", error.c_str());
            throw std::runtime_error(error);
        }
    };

    applyToAllPrimSpecs(prim, renameFunc);
}

void renameHelper(
    const UsdStagePtr&       stage,
    const UsdSceneItem::Ptr& ufeSrcItem,
    const Ufe::Path&         srcPath,
    UsdSceneItem::Ptr&       ufeDstItem,
    const Ufe::Path&         dstPath,
    const std::string&       newName)
{
    // get the stage's default prim path
    auto defaultPrimPath = stage->GetDefaultPrim().GetPath();

    // Note: must fetch prim again from its path because undo/redo of composite commands
    //       (or doing multiple undo and then multiple redo) can make the cached prim
    //       stale.
    const UsdPrim srcPrim = stage->GetPrimAtPath(ufeSrcItem->prim().GetPath());

    doUsdRename(stage, srcPrim, newName, srcPath, dstPath);

    // the renamed scene item is a "sibling" of its original name.
    ufeDstItem = createSiblingSceneItem(srcPath, newName);

    // update stage's default prim
    if (ufeSrcItem->prim().GetPath() == defaultPrimPath) {
        stage->SetDefaultPrim(ufeDstItem->prim());
    }

    // send notification to update UFE data model
    sendNotificationToAllStageProxies(stage, ufeDstItem->prim(), srcPath, dstPath);
}

} // namespace

void UsdUndoRenameCommand::renameRedo()
{
    // If the new name is the same as the current name, do nothing.
    // This is the same behavior as the Maya rename command for Maya nodes.
    if (_newName.empty())
        return;

    auto srcPath = _ufeSrcItem->path();
    auto dstPath = srcPath.sibling(Ufe::PathComponent(_newName));

    renameHelper(_stage, _ufeSrcItem, srcPath, _ufeDstItem, dstPath, _newName);
}

void UsdUndoRenameCommand::renameUndo()
{
    // If the new name is the same as the current name, do nothing.
    // This is the same behavior as the Maya rename command for Maya nodes.
    if (_newName.empty())
        return;

    auto              srcPath = _ufeDstItem->path();
    auto              dstPath = _ufeSrcItem->path();
    const std::string newName = _ufeSrcItem->prim().GetName();

    renameHelper(_stage, _ufeDstItem, srcPath, _ufeSrcItem, dstPath, newName);
}

void UsdUndoRenameCommand::undo()
{
    InPathChange pc;
    renameUndo();
}

void UsdUndoRenameCommand::redo()
{
    InPathChange pc;
    renameRedo();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
