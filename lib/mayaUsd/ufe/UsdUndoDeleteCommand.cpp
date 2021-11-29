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
#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/layer.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/undo/UsdUndoBlock.h>
#endif

namespace {
bool hasLayersMuted(const PXR_NS::UsdPrim& prim)
{
    const PXR_NS::PcpPrimIndex& primIndex = prim.GetPrimIndex();

    for (const PXR_NS::PcpNodeRef node : primIndex.GetNodeRange()) {

        TF_AXIOM(node);

        const PXR_NS::PcpLayerStackSite&   site = node.GetSite();
        const PXR_NS::PcpLayerStackRefPtr& layerStack = site.layerStack;

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

#ifdef UFE_V2_FEATURES_AVAILABLE
void UsdUndoDeleteCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    const auto& stage = _prim.GetStage();
    auto        targetPrimSpec = stage->GetEditTarget().GetPrimSpecForScenePath(_prim.GetPath());

    if (hasLayersMuted(_prim)) {
        TF_WARN("Cannot remove prim because there are muted layers.");
        return;
    }

    if (MayaUsd::ufe::applyCommandRestrictionNoThrow(_prim, "delete")) {
        auto retVal = stage->RemovePrim(_prim.GetPath());
        if (!retVal) {
            TF_VERIFY(retVal, "Failed to delete '%s'", _prim.GetPath().GetText());
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
    UsdUndoBlock                         undoBlock(&_undoableItem);

    const auto& stage = _prim.GetStage();
    auto        targetPrimSpec = stage->GetEditTarget().GetPrimSpecForScenePath(_prim.GetPath());

    if (hasLayersMuted(_prim)) {
        TF_WARN("Cannot remove prim because there are muted layers.");
        return;
    }

    if (MayaUsd::ufe::applyCommandRestrictionNoThrow(_prim, "delete")) {
        auto retVal = stage->RemovePrim(_prim.GetPath());
        if (!retVal) {
            TF_VERIFY(retVal, "Failed to delete '%s'", _prim.GetPath().GetText());
        }
    }
}

void UsdUndoDeleteCommand::undo() { perform(true); }

void UsdUndoDeleteCommand::redo() { perform(false); }
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
