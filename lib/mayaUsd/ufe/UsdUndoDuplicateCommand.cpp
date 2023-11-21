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

#include "private/UfeNotifGuard.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/loadRules.h>
#include <mayaUsdUtils/MergePrims.h>

#include <usdUfe/base/tokens.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/loadRules.h>
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

UsdUndoDuplicateCommand::UsdUndoDuplicateCommand(const UsdSceneItem::Ptr& srcItem)
#ifdef UFE_V4_FEATURES_AVAILABLE
    : Ufe::SceneItemResultUndoableCommand()
#else
    : Ufe::UndoableCommand()
#endif
    , _ufeSrcPath(srcItem->path())
{
    auto srcPrim = srcItem->prim();
    auto parentPrim = srcPrim.GetParent();

    auto newName = uniqueChildName(parentPrim, srcPrim.GetName());
    _usdDstPath = parentPrim.GetPath().AppendChild(TfToken(newName));

    auto primSpec = UsdUfe::getDefiningPrimSpec(srcPrim);
    if (primSpec)
        _srcLayer = primSpec->GetLayer();
}

UsdUndoDuplicateCommand::~UsdUndoDuplicateCommand() { }

UsdUndoDuplicateCommand::Ptr UsdUndoDuplicateCommand::create(const UsdSceneItem::Ptr& srcItem)
{
    return std::make_shared<UsdUndoDuplicateCommand>(srcItem);
}

UsdSceneItem::Ptr UsdUndoDuplicateCommand::duplicatedItem() const
{
    return createSiblingSceneItem(_ufeSrcPath, _usdDstPath.GetElementString());
}

void UsdUndoDuplicateCommand::execute()
{
    UsdUfe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    auto prim = ufePathToPrim(_ufeSrcPath);
    auto path = prim.GetPath();
    auto stage = prim.GetStage();

    OperationEditRouterContext ctx(UsdUfe::EditRoutingTokens->RouteDuplicate, prim);
    _dstLayer = stage->GetEditTarget().GetLayer();

    auto                               item = Ufe::Hierarchy::createItem(_ufeSrcPath);
    MayaUsd::ufe::ReplicateExtrasToUSD extras;
    extras.initRecursive(item);

    // The loaded state of a model is controlled by the load rules of the stage.
    // When duplicating a node, we want the new node to be in the same loaded
    // state.
    duplicateLoadRules(*stage, path, _usdDstPath);

    // Make sure all necessary parent exists in the target layer, at least as over,
    // otherwise SdfCopySepc will fail.
    SdfJustCreatePrimInLayer(_dstLayer, _usdDstPath.GetParentPath());

    // Retrieve the local layers around where the prim is defined and order them
    // from weak to strong. That weak-to-strong order allows us to copy the weakest
    // opinions first, so that they will get over-written by the stronger opinions.
    SdfPrimSpecHandleVector authLayerAndPaths = getDefiningPrimStack(prim);
    std::reverse(authLayerAndPaths.begin(), authLayerAndPaths.end());

    MayaUsdUtils::MergePrimsOptions options;
    options.verbosity = MayaUsdUtils::MergeVerbosity::None;
    options.mergeChildren = true;
    bool isFirst = true;

    for (const SdfPrimSpecHandle& layerAndPath : authLayerAndPaths) {
        const auto layer = layerAndPath->GetLayer();
        const auto path = layerAndPath->GetPath();
        const bool result = isFirst
            ? SdfCopySpec(layer, path, _dstLayer, _usdDstPath)
            : mergePrims(stage, layer, path, stage, _dstLayer, _usdDstPath, options);

        TF_VERIFY(
            result,
            "Failed to copy the USD prim at '%s' in layer '%s' to '%s'",
            path.GetText(),
            layer->GetDisplayName().c_str(),
            _usdDstPath.GetText());

        isFirst = false;
    }

    extras.finalize(MayaUsd::ufe::stagePath(stage), &path, &_usdDstPath);
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
