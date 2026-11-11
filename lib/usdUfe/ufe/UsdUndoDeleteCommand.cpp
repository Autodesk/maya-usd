//
// Copyright 2015 Autodesk
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

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editContext.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <usdUfe/ufe/UsdAttributes.h>
#endif

#include <ufe/pathString.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoDeleteCommand);

#ifdef MAYA_ENABLE_NEW_PRIM_DELETE
namespace {

// Where a prim is deleted.
enum class DeleteTarget
{
    // In every layer of its prim stack, as in component stages.
    AllLayers,
    // In a local layer an edit router sends the delete to.
    RoutedLocalLayer,
    // In a referenced or payloaded layer an edit router sends the delete to.
    RoutedReferencedLayer,
    // In the edit target.
    EditTarget
};

DeleteTarget
getDeleteTarget(const PXR_NS::UsdStagePtr& stage, const PXR_NS::UsdEditTarget& routingEditTarget)
{
    if (UsdUfe::isComponentStage(UsdUfe::stagePath(stage))) {
        return DeleteTarget::AllLayers;
    }
    if (routingEditTarget.IsNull()) {
        return DeleteTarget::EditTarget;
    }
    return stage->HasLocalLayer(routingEditTarget.GetLayer()) ? DeleteTarget::RoutedLocalLayer
                                                              : DeleteTarget::RoutedReferencedLayer;
}

// Remove the paths pointing to the prim in the layers it is deleted in.
bool cleanReferencedPath(
    const PXR_NS::UsdPrim&       prim,
    DeleteTarget                 deleteTarget,
    const PXR_NS::UsdEditTarget& routingEditTarget)
{
    if (deleteTarget == DeleteTarget::AllLayers) {
        return UsdUfe::cleanReferencedPathInAllLayers(prim);
    }
    if (deleteTarget == DeleteTarget::RoutedReferencedLayer) {
        return UsdUfe::cleanReferencedPathInLayer(prim, routingEditTarget.GetLayer());
    }
    return UsdUfe::cleanReferencedPath(prim);
}

} // namespace
#endif

UsdUndoDeleteCommand::UsdUndoDeleteCommand(const PXR_NS::UsdPrim& prim)
    : Ufe::UndoableCommand()
    , _prim(prim)
{
}

UsdUndoDeleteCommand::Ptr UsdUndoDeleteCommand::create(const PXR_NS::UsdPrim& prim)
{
    return std::make_shared<UsdUndoDeleteCommand>(prim);
}

void UsdUndoDeleteCommand::execute()
{
    if (!_prim.IsValid())
        return;

    UsdUfe::enforceMutedLayer(_prim, "remove");

    UsdUfe::InAddOrDeleteOperation ad;

    // Pause edit forwarding during the delete operation with RAII
    const UsdUfe::EditForwardingGuard efPauser {};

    UsdUfe::UsdUndoBlock undoBlock(&_undoableItem);

#ifdef MAYA_ENABLE_NEW_PRIM_DELETE
    const auto& stage = _prim.GetStage();
    auto        targetPrimSpec = stage->GetEditTarget().GetPrimSpecForScenePath(_prim.GetPath());

    PXR_NS::UsdEditTarget routingEditTarget
        = UsdUfe::getEditRouterEditTarget(UsdUfe::EditRoutingTokens->RouteDelete, _prim);
    const DeleteTarget deleteTarget = getDeleteTarget(stage, routingEditTarget);
    const bool         isRouted = deleteTarget == DeleteTarget::RoutedLocalLayer
        || deleteTarget == DeleteTarget::RoutedReferencedLayer;

    // Validate that the prim can be deleted, before editing any layer.
    if (deleteTarget == DeleteTarget::AllLayers) {
        UsdUfe::validateComponentNamespaceOperation(_prim, "delete");
    } else if (isRouted) {
        PXR_NS::UsdEditContext ctx(stage, routingEditTarget);

        // Note: we allow stronger opinion when editing inside a reference or payload.
        const bool allowStronger = deleteTarget == DeleteTarget::RoutedReferencedLayer;
        if (!UsdUfe::applyCommandRestrictionNoThrow(_prim, "delete", allowStronger))
            return;
    } else if (!UsdUfe::applyCommandRestrictionNoThrow(_prim, "delete")) {
        return;
    }

    {
        // A delete routed to a local layer edits that layer, so the paths pointing to the
        // prim are cleaned from it.
        PXR_NS::UsdEditContext cleanupCtx(
            stage,
            deleteTarget == DeleteTarget::RoutedLocalLayer ? routingEditTarget
                                                           : stage->GetEditTarget());

#ifdef UFE_V4_FEATURES_AVAILABLE
        UsdUfe::UsdAttributes::removeAttributesConnections(_prim);
#endif
        // Let removeAttributesConnections be run first as it will also cleanup
        // attributes that were authored only to be the destination of a connection.
        if (!cleanReferencedPath(_prim, deleteTarget, routingEditTarget)) {
            const std::string error = TfStringPrintf(
                "Failed to cleanup references to prim \"%s\".", _prim.GetPath().GetText());
            TF_WARN("%s", error.c_str());
            throw std::runtime_error(error);
        }
    }

    if (deleteTarget == DeleteTarget::AllLayers) {
        // Get all prim specs from all layers (including non-local layers like payloads)
        const PXR_NS::SdfPrimSpecHandleVector primStack = _prim.GetPrimStack();

        for (const PXR_NS::SdfPrimSpecHandle& primSpec : primStack) {
            if (!primSpec)
                continue;

            const PXR_NS::SdfLayerHandle layer = primSpec->GetLayer();
            if (!layer)
                continue;

            const PXR_NS::SdfPath primPath = primSpec->GetPath();

            UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);

            // Get the parent spec
            PXR_NS::SdfPrimSpecHandle parent = primSpec->GetRealNameParent();
            if (!parent) {
                const std::string error = TfStringPrintf(
                    "Failed to get parent for prim \"%s\" in layer \"%s\".",
                    primPath.GetText(),
                    layer->GetDisplayName().c_str());
                TF_WARN("%s", error.c_str());
                throw std::runtime_error(error);
            }

            // Remove the prim spec from its parent
            if (!parent->RemoveNameChild(primSpec)) {
                const std::string error = TfStringPrintf(
                    "Failed to delete prim \"%s\" from layer \"%s\".",
                    primPath.GetText(),
                    layer->GetDisplayName().c_str());
                TF_WARN("%s", error.c_str());
                throw std::runtime_error(error);
            }

            layer->RemovePrimIfInert(parent);
        }
    } else if (isRouted) {
        PXR_NS::UsdEditContext ctx(stage, routingEditTarget);

        if (!stage->RemovePrim(_prim.GetPath())) {
            const std::string error
                = TfStringPrintf("Failed to delete prim \"%s\".", _prim.GetPath().GetText());
            TF_WARN("%s", error.c_str());
            throw std::runtime_error(error);
        }
    } else {
        UsdUfe::PrimSpecFunc deleteFunc
            = [stage](const UsdPrim& prim, const SdfPrimSpecHandle& primSpec) -> void {
            if (!primSpec)
                return;

            PXR_NS::UsdEditContext ctx(stage, primSpec->GetLayer());
            if (!stage->RemovePrim(prim.GetPath())) {
                const std::string error
                    = TfStringPrintf("Failed to delete prim \"%s\".", prim.GetPath().GetText());
                TF_WARN("%s", error.c_str());
                throw std::runtime_error(error);
            }
        };
        UsdUfe::applyToAllPrimSpecs(_prim, deleteFunc);
    }
#else
    _prim.SetActive(false);
#endif
}

void UsdUndoDeleteCommand::undo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    // Pause edit forwarding during the undo operation with RAII
    const UsdUfe::EditForwardingGuard efPauser {};

    _undoableItem.undo();
}

void UsdUndoDeleteCommand::redo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    // Pause edit forwarding during the redo operation with RAII
    const UsdUfe::EditForwardingGuard efPauser {};

    _undoableItem.redo();
}

} // namespace USDUFE_NS_DEF
