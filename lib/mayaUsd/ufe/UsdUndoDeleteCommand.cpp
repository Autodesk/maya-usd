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

bool hasLayersMuted(const UsdPrim& prim)
{
    const PXR_NS::PcpPrimIndex& primIndex = prim.GetPrimIndex();

    // iterate through the expanded primIndex
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
{
}

UsdUndoDeleteCommand::~UsdUndoDeleteCommand() { }

UsdUndoDeleteCommand::Ptr UsdUndoDeleteCommand::create(const PXR_NS::UsdPrim& prim)
{
    return std::make_shared<UsdUndoDeleteCommand>(prim);
}

bool isPrimDeletionAllowed(const PXR_NS::UsdPrim& prim, uint32_t targetidx)
{
    PXR_NS::PcpPrimIndex primIndex = prim.ComputeExpandedPrimIndex();

    if (!prim.IsActive()) {
        TF_WARN("Cannot edit [%s] prim because it is inactive.", prim.GetPath().GetText());
        return false;
    }

    for (const PcpNodeRef& node : primIndex.GetNodeRange()) {
        // inspect the edge connecting this node to its parent in the tree
        if (!node.IsDueToAncestor()) {
            if (node.GetArcType() == PcpArcTypeVariant
                || node.GetArcType() == PcpArcTypeReference) {
                uint32_t intro = node.GetDepthBelowIntroduction();
                if (intro < targetidx) {
                    TF_WARN(
                        "Cannot edit [%s] prim because there is a stronger opinion in layer [%s].",
                        prim.GetPath().GetText(),
                        std::to_string(intro).c_str());
                    return false;
                }
            }
        }
    }
    return true;
}

bool _recurseChildren(UsdPrim const& _prim, uint32_t targetidx)
{
    // check down the hierarchy
    UsdPrimSiblingRange children
        = _prim.GetFilteredChildren(UsdPrimIsActive && UsdPrimIsLoaded && !UsdPrimIsAbstract);
    if (!children.empty()) {
        for (UsdPrim const& prim : children) {

            if (!isPrimDeletionAllowed(prim, targetidx)) {
                return false;
            }

            if (!_recurseChildren(prim, targetidx)) {
                return false;
            }

            std::vector<UsdAttribute> attrs = prim.GetAttributes();
            for (auto& attr : attrs) {
                if (!isAttributeEditAllowed(prim, attr.GetName())) {
                    return false;
                }
            }
        }
    }
    return true;
}

#ifdef UFE_V2_FEATURES_AVAILABLE
void UsdUndoDeleteCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);
    bool         isAllowed = true;
    const auto&  stage = _prim.GetStage();
    std::string  errMsg;

    uint32_t targetidx = findLayerIndex(_prim, stage->GetEditTarget().GetLayer());

    if (hasLayersMuted(_prim)) {
        TF_WARN("Cannot remove prim because there is muted layers.");
        return;
    }

    isAllowed = isPrimDeletionAllowed(_prim, targetidx);

    if (isAllowed) {
        std::vector<UsdAttribute> attrs = _prim.GetAttributes();
        for (auto& attr : attrs) {
            if (!isAttributeEditAllowed(_prim, attr.GetName())) {
                isAllowed = false;
                break;
            }
        }
    }

    if (isAllowed)
        isAllowed = _recurseChildren(_prim, targetidx);

    if (isAllowed) {
        auto retVal = stage->RemovePrim(_prim.GetPath());
        if (!retVal) {
            TF_VERIFY(retVal, "Failed to remove '%s'", _prim.GetPath().GetText());
        }
    } else {
        _prim.SetActive(false);
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
