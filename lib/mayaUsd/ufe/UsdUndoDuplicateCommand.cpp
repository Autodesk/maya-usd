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
#include "UsdUndoDuplicateCommand.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/loadRules.h>

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/loadRules.h>
#include <usdUfe/utils/mergePrims.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/hierarchy.h>
#include <ufe/log.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#include <algorithm>

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Ensure that UsdUndoDuplicateCommand is properly setup.
#ifdef UFE_V4_FEATURES_AVAILABLE
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::SceneItemResultUndoableCommand, UsdUndoDuplicateCommand);
#else
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoDuplicateCommand);
#endif

UsdUndoDuplicateCommand::UsdUndoDuplicateCommand(const UsdUfe::UsdSceneItem::Ptr& srcItem)
#ifdef UFE_V4_FEATURES_AVAILABLE
    : Ufe::SceneItemResultUndoableCommand()
#else
    : Ufe::UndoableCommand()
#endif
    , _ufeSrcPath(srcItem->path())
{
    auto srcPrim = srcItem->prim();
    auto parentPrim = srcPrim.GetParent();

    auto newName = UsdUfe::uniqueChildName(parentPrim, srcPrim.GetName());
    _usdDstPath = parentPrim.GetPath().AppendChild(TfToken(newName));

    auto primSpec = UsdUfe::getDefiningPrimSpec(srcPrim);
    if (primSpec)
        _srcLayer = primSpec->GetLayer();
}

UsdUndoDuplicateCommand::Ptr
UsdUndoDuplicateCommand::create(const UsdUfe::UsdSceneItem::Ptr& srcItem)
{
    return std::make_shared<UsdUndoDuplicateCommand>(srcItem);
}

UsdUfe::UsdSceneItem::Ptr UsdUndoDuplicateCommand::duplicatedItem() const
{
    return UsdUfe::createSiblingSceneItem(_ufeSrcPath, _usdDstPath.GetElementString());
}

void UsdUndoDuplicateCommand::execute()
{
    UsdUfe::InAddOrDeleteOperation ad;

    UsdUfe::UsdUndoBlock undoBlock(&_undoableItem);

    auto prim = ufePathToPrim(_ufeSrcPath);
    auto path = prim.GetPath();
    auto stage = prim.GetStage();

    UsdUfe::OperationEditRouterContext ctx(UsdUfe::EditRoutingTokens->RouteDuplicate, prim);
    _dstLayer = stage->GetEditTarget().GetLayer();

    auto                               item = Ufe::Hierarchy::createItem(_ufeSrcPath);
    MayaUsd::ufe::ReplicateExtrasToUSD extras;
    extras.initRecursive(item);

    // The loaded state of a model is controlled by the load rules of the stage.
    // When duplicating a node, we want the new node to be in the same loaded
    // state.
    UsdUfe::duplicateLoadRules(*stage, path, _usdDstPath);

    // Make sure all necessary parent exists in the target layer, at least as over,
    // otherwise SdfCopySepc will fail.
    SdfJustCreatePrimInLayer(_dstLayer, _usdDstPath.GetParentPath());

    // Retrieve the local layers around where the prim is defined and order them
    // from weak to strong. That weak-to-strong order allows us to copy the weakest
    // opinions first, so that they will get over-written by the stronger opinions.
    SdfPrimSpecHandleVector authLayerAndPaths = UsdUfe::getDefiningPrimStack(prim);
    std::reverse(authLayerAndPaths.begin(), authLayerAndPaths.end());

    const bool includeTopLayer = true;
    const auto sessionLayers
        = UsdUfe::getAllSublayerRefs(stage->GetSessionLayer(), includeTopLayer);

    UsdUfe::MergePrimsOptions options;
    options.verbosity = UsdUfe::MergeVerbosity::None;
    options.mergeChildren = true;

    bool isFirst = true;

    for (const SdfPrimSpecHandle& layerAndPath : authLayerAndPaths) {
        const auto layer = layerAndPath->GetLayer();
        const auto path = layerAndPath->GetPath();

        // We want to leave session data in the session layers.
        // If a layer is a session layer then we set the target to be that same layer.
        const bool isInSession = UsdUfe::isSessionLayer(layer, sessionLayers);
        const auto targetLayer = isInSession ? layer : _dstLayer;

        if (isInSession)
            SdfJustCreatePrimInLayer(targetLayer, _usdDstPath);

        // If it's the first layer processed, or if a session layer, we want a basic copy of the
        // specs on this layer. For session layers we want to keep changes in the same layers.
        // However, if the target itself is the session layer, then we need a merge, otherwise we
        // would overwrite previously written specs to that layer.
        bool simpleCopy = isFirst || (isInSession && _dstLayer != layer);

        const bool result = simpleCopy
            ? SdfCopySpec(layer, path, targetLayer, _usdDstPath)
            : UsdUfe::mergePrims(stage, layer, path, stage, targetLayer, _usdDstPath, options);

        TF_VERIFY(
            result,
            "Failed to copy the USD prim at '%s' in layer '%s' to '%s'",
            path.GetText(),
            layer->GetDisplayName().c_str(),
            _usdDstPath.GetText());

        // We only set the first-layer flag to false once we have processed a non-session layer.
        if (!isInSession)
            isFirst = false;
    }

    MayaUsd::ufe::ReplicateExtrasToUSD::RenamedPaths renamed;
    renamed[path] = _usdDstPath;
    extras.finalize(MayaUsd::ufe::stagePath(stage), renamed);
}

void UsdUndoDuplicateCommand::undo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoDuplicateCommand::redo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
