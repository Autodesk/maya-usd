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
#include "UsdUndoDeleteCommand.h"

#include "private/UfeNotifGuard.h"

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#endif

namespace {

bool hasLayersMuted(const PXR_NS::UsdPrim& prim)
{
    const PXR_NS::PcpPrimIndex& primIndex = prim.GetPrimIndex();

    for (PcpNodeRef node : primIndex.GetNodeRange()) {

        TF_AXIOM(node);

        const PcpLayerStackSite&   site = node.GetSite();
        const PcpLayerStackRefPtr& layerStack = site.layerStack;

        const std::set<std::string>& mutedLayers = layerStack->GetMutedLayers();
        if (mutedLayers.size() > 0) {
            return true;
        }
    }
    return false;
}

} // anonymous namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDeleteCommand::UsdUndoDeleteCommand(const PXR_NS::UsdPrim& prim)
    : Ufe::UndoableCommand()
    , _prim(prim)
    , _stage(prim.GetStage())
{
}

UsdUndoDeleteCommand::~UsdUndoDeleteCommand() { }

UsdUndoDeleteCommand::Ptr UsdUndoDeleteCommand::create(const PXR_NS::UsdPrim& prim)
{
    return std::make_shared<UsdUndoDeleteCommand>(prim);
}

bool isPrimDeletionAllowed(const PXR_NS::UsdPrim& prim, SdfPrimSpecHandle primSpec)
{

    auto primStack = prim.GetPrimStack();

    std::string layerDisplayName;

    // iterate over the prim stack, starting at the highest-priority layer.
    for (const auto& spec : primStack) {
        const auto& layerName = spec->GetLayer()->GetDisplayName();

        // skip if there is no primSpec for the selected prim in the current stage's local layer.
        if (!primSpec) {
            // add "," separator for multiple layers
            if (!layerDisplayName.empty()) {
                layerDisplayName.append(",");
            }
            layerDisplayName.append("[" + layerName + "]");
            continue;
        }

        // one reason for skipping the reference is to not clash
        // with the over that may be created in the stage's sessionLayer.
        // another reason is that one should be able to edit a referenced prim that
        // either as over/def as long as it has a primSpec in the selected edit target layer.
        if (spec->HasReferences()) {
            break;
        }

        // if exists a def/over specs
        if (spec->GetSpecifier() == SdfSpecifierDef || spec->GetSpecifier() == SdfSpecifierOver) {
            // if spec exists in another layer ( e.g sessionLayer or layer other than stage's local
            // layers ).
            if (primSpec->GetLayer() != spec->GetLayer()) {
                layerDisplayName.append("[" + layerName + "]");
                TF_WARN(
                    "Cannot delete [%s] because it is defined on a stronger layer %s.",
                    prim.GetName().GetString().c_str(),
                    layerDisplayName.c_str());
                return false;
            }
            continue;
        }
    }
    // Post a clear message to indicate that deleting a prim inside a variantset is not allowed.
    // This restriction was already caught in the above loop.
    UsdPrimCompositionQuery query(prim);
    for (const auto& compQueryArc : query.GetCompositionArcs()) {
        if (!primSpec && PcpArcTypeVariant == compQueryArc.GetArcType()) {
            TF_WARN(
                "Cannot delete [%s] because it is defined inside the variant composition arc %s.",
                prim.GetName().GetString().c_str(),
                layerDisplayName.c_str());
            return false;
        }
    }

    if (!layerDisplayName.empty()) {
        TF_WARN(
            "Cannot delete [%s]. Please set %s as the target layer to proceed.",
            prim.GetName().GetString().c_str(),
            layerDisplayName.c_str());
        return false;
    }
    return true;
}

#ifdef UFE_V2_FEATURES_AVAILABLE
void UsdUndoDeleteCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    auto targetPrimSpec = _stage->GetEditTarget().GetPrimSpecForScenePath(_prim.GetPath());

    if (hasLayersMuted(_prim)) {
        TF_WARN("Cannot remove prim because there is muted layers.");
        return;
    }

    if (isPrimDeletionAllowed(_prim, targetPrimSpec)) {
        auto retVal = _stage->RemovePrim(_prim.GetPath());
        if (!retVal) {
            TF_VERIFY(retVal, "Failed to remove '%s'", _prim.GetPath().GetText());
        }
    }
}

void UsdUndoDeleteCommand::undo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoDeleteCommand::redo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}
#else
void UsdUndoDeleteCommand::perform(bool state)
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;
    _prim.SetActive(state);
}

void UsdUndoDeleteCommand::undo() { perform(true); }

void UsdUndoDeleteCommand::redo() { perform(false); }
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
