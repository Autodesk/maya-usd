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
#include "private/Utils.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/editRouter.h>
#include <mayaUsd/utils/editRouterContext.h>
#include <mayaUsd/utils/loadRules.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsdUtils/MergePrims.h>
#endif
#include <mayaUsdUtils/util.h>

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

    _srcLayer = MayaUsdUtils::getDefiningLayerAndPath(srcPrim).layer;
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

#ifdef UFE_V2_FEATURES_AVAILABLE
void UsdUndoDuplicateCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    auto prim = ufePathToPrim(_ufeSrcPath);
    auto path = prim.GetPath();
    auto stage = prim.GetStage();

    OperationEditRouterContext ctx(MayaUsdEditRoutingTokens->RouteDuplicate, prim);
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

    // Retrieve the layers where there are opinion and order them from weak
    // to strong. We will copy the weakest opinions first, so that they will
    // get over-written by the stronger opinions.
    using namespace MayaUsdUtils;
    std::vector<LayerAndPath> authLayerAndPaths = getAuthoredLayerAndPaths(prim);
    std::reverse(authLayerAndPaths.begin(), authLayerAndPaths.end());

    MergePrimsOptions options;
    options.verbosity = MergeVerbosity::None;
    bool isFirst = true;

    for (const LayerAndPath& layerAndPath : authLayerAndPaths) {
        const auto layer = layerAndPath.layer;
        const auto path = layerAndPath.path;
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
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoDuplicateCommand::redo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}
#else
bool UsdUndoDuplicateCommand::duplicateUndo()
{
    // USD sends a ResyncedPaths notification after the prim is removed, but
    // at that point the prim is no longer valid, and thus a UFE post delete
    // notification is no longer possible.  To respect UFE object delete
    // notification semantics, which require the object to be alive when
    // the notification is sent, we send a pre delete notification here.
    auto                 ufeDstItem = duplicatedItem();
    Ufe::ObjectPreDelete notification(ufeDstItem);

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::Scene::instance().notify(notification);
#else
    Ufe::Scene::notifyObjectDelete(notification);
#endif
    auto prim = ufePathToPrim(_ufeSrcPath);
    prim.GetStage()->RemovePrim(_usdDstPath);

    return true;
}

bool UsdUndoDuplicateCommand::duplicateRedo()
{
    auto prim = ufePathToPrim(_ufeSrcPath);
    SdfJustCreatePrimInLayer(_dstLayer, _usdDstPath.GetParentPath());
    return SdfCopySpec(_srcLayer, prim.GetPath(), _dstLayer, _usdDstPath);
}

void UsdUndoDuplicateCommand::undo()
{
    try {
        MayaUsd::ufe::InAddOrDeleteOperation ad;
        if (!duplicateUndo()) {
            UFE_LOG("duplicate undo failed");
        }
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}

void UsdUndoDuplicateCommand::redo()
{
    try {
        MayaUsd::ufe::InAddOrDeleteOperation ad;
        if (!duplicateRedo()) {
            UFE_LOG("duplicate redo failed");
        }
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
