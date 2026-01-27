//
// Copyright 2024 Autodesk
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

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/loadRules.h>
#include <usdUfe/utils/mergePrims.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

namespace USDUFE_NS_DEF {

// Ensure that UsdUndoDuplicateCommand is properly setup.
#ifdef UFE_V4_FEATURES_AVAILABLE
USDUFE_VERIFY_CLASS_SETUP(Ufe::SceneItemResultUndoableCommand, UsdUndoDuplicateCommand);
#else
USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoDuplicateCommand);
#endif

namespace {

// Local ufePathToPrim that uses input stage instead of getStage function.
UsdPrim ufePathToPrim(const Ufe::Path& path, const PXR_NS::UsdStageWeakPtr& stage)
{
    if (!stage)
        return {};

    const Ufe::Path            ufePrimPath = stripInstanceIndexFromUfePath(path);
    const Ufe::Path::Segments& segments = ufePrimPath.getSegments();
    if (segments.empty())
        return {};

    // Anonymous layer.
    if (path.string().at(0) == '/') {
        return stage->GetPrimAtPath(SdfPath(path.string()).GetPrimPath());
    }

    // If there is only a single segment in the path, it must point to the
    // proxy shape, otherwise we would not have retrieved a valid stage.
    // The second path segment is the USD path.
    return (segments.size() == 1U)
        ? stage->GetPseudoRoot()
        : stage->GetPrimAtPath(SdfPath(segments[1].string()).GetPrimPath());
}

} // namespace

UsdUndoDuplicateCommand::UsdUndoDuplicateCommand(
    const UsdSceneItem::Ptr& srcItem,
    const UsdSceneItem::Ptr& dstParentItem)
    : _ufeDstPath(dstParentItem->path())
    , _ufeSrcPath(srcItem->path())
    , _dstStage(dstParentItem->prim().GetStage())
    , _srcStage(srcItem->prim().GetStage())
{
    auto srcPrim = srcItem->prim();
    auto newName = uniqueChildName(dstParentItem->prim(), srcPrim.GetName(), nullptr);
    _usdDstPath = dstParentItem->prim().GetPath().AppendChild(TfToken(newName));
}

UsdUndoDuplicateCommand::Ptr UsdUndoDuplicateCommand::create(
    const UsdSceneItem::Ptr& srcItem,
    const UsdSceneItem::Ptr& dstParentItem)
{
    return std::make_shared<UsdUndoDuplicateCommand>(srcItem, dstParentItem);
}
UsdSceneItem::Ptr UsdUndoDuplicateCommand::duplicatedItem() const
{
    Ufe::Path ufeSrcPath;

    if (!_ufeDstPath.getSegments().empty()) {
        ufeSrcPath = appendToUsdPath(_ufeDstPath, _usdDstPath.GetElementString());
    } else {
        // Temporary USD stage case.
        const auto ufePathAsString = "/" + _usdDstPath.GetElementString();
        ufeSrcPath = Ufe::PathSegment(ufePathAsString, getUsdRunTimeId(), '/');
    }

    UsdPrim prim = ufePathToPrim(ufeSrcPath, _dstStage);
    return UsdSceneItem::create(ufeSrcPath, prim);
}

void UsdUndoDuplicateCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    UsdPrim prim = ufePathToPrim(_ufeSrcPath, _srcStage);

    auto path = prim.GetPath();
    auto stage = prim.GetStage();

    OperationEditRouterContext ctx(EditRoutingTokens->RouteDuplicate, prim);

    // The loaded state of a model is controlled by the load rules of the stage.
    // When duplicating a node, we want the new node to be in the same loaded
    // state.
    duplicateLoadRules(*stage, path, _usdDstPath);

    const auto dstLayer = _dstStage->GetEditTarget().GetLayer();

    // Make sure all necessary parent exists in the target layer, at least as over,
    // otherwise SdfCopySepc will fail.
    SdfJustCreatePrimInLayer(dstLayer, _usdDstPath.GetParentPath());

    // Retrieve the local layers around where the prim is defined and order them
    // from weak to strong. That weak-to-strong order allows us to copy the weakest
    // opinions first, so that they will get over-written by the stronger opinions.
    SdfPrimSpecHandleVector authLayerAndPaths = getDefiningPrimStack(prim);
    std::reverse(authLayerAndPaths.begin(), authLayerAndPaths.end());

    const bool includeTopLayer = true;
    const auto sessionLayers
        = UsdUfe::getAllSublayerRefs(stage->GetSessionLayer(), includeTopLayer);

    bool isFirst = true;

    for (const SdfPrimSpecHandle& layerAndPath : authLayerAndPaths) {
        const auto layer = layerAndPath->GetLayer();
        const auto localPath = layerAndPath->GetPath();

        // We want to leave session data in the session layers.
        // If a layer is a session layer then we set the target to be that same layer.
        const bool isInSession = UsdUfe::isSessionLayer(layer, sessionLayers);
        const auto targetLayer = isInSession ? layer : dstLayer;

        if (isInSession)
            SdfJustCreatePrimInLayer(targetLayer, _usdDstPath);

        const bool result = (isFirst || isInSession)
            ? SdfCopySpec(layer, localPath, targetLayer, _usdDstPath)
            : UsdUfe::mergePrims(stage, layer, localPath, _dstStage, targetLayer, _usdDstPath);

        TF_VERIFY(
            result,
            "Failed to copy the USD prim at '%s' in layer '%s' to '%s'",
            localPath.GetText(),
            layer->GetDisplayName().c_str(),
            _usdDstPath.GetText());

        // We only set the first-layer flag to false once we have processed a non-session layer.
        if (!isInSession)
            isFirst = false;
    }
}

void UsdUndoDuplicateCommand::undo() { _undoableItem.undo(); }

void UsdUndoDuplicateCommand::redo() { _undoableItem.redo(); }

} // namespace USDUFE_NS_DEF
