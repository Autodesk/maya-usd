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

namespace {

// Validate that a prim can be deleted in a component stage.
// Components only allow deleting prims whose path is contained within
// either the material or geometry scope
void validateComponentDelete(const PXR_NS::UsdPrim& prim)
{
    const PXR_NS::UsdStagePtr stage = prim.GetStage();
    if (!stage) {
        return;
    }

    // Get the prim path
    const PXR_NS::SdfPath primPath = prim.GetPath();

    // Get the default prim to construct full scope paths
    auto defaultPrim = stage->GetDefaultPrim();
    if (!defaultPrim) {
        return;
    }
    const PXR_NS::SdfPath defaultPrimPath = defaultPrim.GetPath();

    // Don't allow deleting the default prim itself
    if (primPath == defaultPrimPath) {
        const std::string error = PXR_NS::TfStringPrintf(
            "Cannot delete prim \"%s\" in a component stage. "
            "This is the default prim.",
            primPath.GetText());
        TF_WARN("%s", error.c_str());
        throw std::runtime_error(error);
    }

    // Get material and mesh scope names from component
    std::string materialScopeName = UsdUfe::getComponentMaterialScopeName(stage);
    std::string meshScopeName     = UsdUfe::getComponentMeshScopeName(stage);

    // Build full scope paths
    std::vector<PXR_NS::SdfPath> scopePaths;
    if (!materialScopeName.empty()) {
        scopePaths.push_back(defaultPrimPath.AppendChild(PXR_NS::TfToken(materialScopeName)));
    }
    if (!meshScopeName.empty()) {
        scopePaths.push_back(defaultPrimPath.AppendChild(PXR_NS::TfToken(meshScopeName)));
    }

    // Don't allow deleting the scope prims themselves
    for (const PXR_NS::SdfPath& scopePath : scopePaths) {
        if (primPath == scopePath) {
            const std::string error = PXR_NS::TfStringPrintf(
                "Cannot delete prim \"%s\" in a component stage. "
                "This is a protected scope.",
                primPath.GetText());
            TF_WARN("%s", error.c_str());
            throw std::runtime_error(error);
        }
    }

    // Check if the prim path is contained within any of the scope paths
    bool isInScope = false;
    for (const PXR_NS::SdfPath& scopePath : scopePaths) {
        if (primPath.HasPrefix(scopePath)) {
            isInScope = true;
            break;
        }
    }

    // Allow if prim is within a component scope
    if (isInScope) {
        return;
    }

    // Disallow - prim is not in component scopes
    const std::string error = PXR_NS::TfStringPrintf(
        "Cannot delete prim \"%s\" in a component stage. "
        "Only prims within component material or mesh scopes can be deleted. ",
        primPath.GetText());
    TF_WARN("%s", error.c_str());
    throw std::runtime_error(error);
}

} // namespace

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoDeleteCommand);

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

    UsdUfe::UsdUndoBlock undoBlock(&_undoableItem);

#ifdef MAYA_ENABLE_NEW_PRIM_DELETE
    const auto& stage = _prim.GetStage();
    auto        targetPrimSpec = stage->GetEditTarget().GetPrimSpecForScenePath(_prim.GetPath());

    PXR_NS::UsdEditTarget routingEditTarget
        = UsdUfe::getEditRouterEditTarget(UsdUfe::EditRoutingTokens->RouteDelete, _prim);

#ifdef UFE_V4_FEATURES_AVAILABLE
    UsdUfe::UsdAttributes::removeAttributesConnections(_prim);
#endif
    // Let removeAttributesConnections be run first as it will also cleanup
    // attributes that were authored only to be the destination of a connection.
    if (!UsdUfe::cleanReferencedPath(_prim)) {
        const std::string error = TfStringPrintf(
            "Failed to cleanup references to prim \"%s\".", _prim.GetPath().GetText());
        TF_WARN("%s", error.c_str());
        throw std::runtime_error(error);
    }

    // Check if this is a component stage
    const Ufe::Path proxyPath   = UsdUfe::stagePath(stage);
    const bool      isComponent = UsdUfe::isComponentStage(proxyPath);

    if (isComponent) {
        // Validate that the prim can be deleted in a component stage
        validateComponentDelete(_prim);

        // Get all prim specs from all layers (including non-local layers like payloads)
        const PXR_NS::SdfPrimSpecHandleVector primStack = _prim.GetPrimStack();

        for (const PXR_NS::SdfPrimSpecHandle& primSpec : primStack) {
            if (!primSpec)
                continue;

            const PXR_NS::SdfLayerHandle layer = primSpec->GetLayer();
            if (!layer)
                continue;

            const PXR_NS::SdfPath primPath = primSpec->GetPath();

            // DEBUG: Print primspec information
            TF_WARN("DEBUG: Component delete - PrimSpec path: %s", primPath.GetText());
            TF_WARN("DEBUG: Component delete - Layer: %s", layer->GetDisplayName().c_str());
            TF_WARN("DEBUG: Component delete - Layer identifier: %s",
                    layer->GetIdentifier().c_str());
            TF_WARN("DEBUG: Component delete - PrimSpec specifier: %s",
                    TfEnum::GetName(primSpec->GetSpecifier()).c_str());

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

            // DEBUG: Print parent information
            TF_WARN("DEBUG: Component delete - Parent path: %s", parent->GetPath().GetText());
            TF_WARN("DEBUG: Component delete - Parent specifier: %s",
                    TfEnum::GetName(parent->GetSpecifier()).c_str());
            TF_WARN("DEBUG: Component delete - Parent has %zu children",
                    parent->GetNameChildren().size());

            // Remove the prim spec from its parent
            if (!parent->RemoveNameChild(primSpec)) {
                const std::string error = TfStringPrintf(
                    "Failed to delete prim \"%s\" from layer \"%s\".",
                    primPath.GetText(),
                    layer->GetDisplayName().c_str());
                TF_WARN("%s", error.c_str());
                throw std::runtime_error(error);
            }

            // DEBUG: Confirm removal
            TF_WARN("DEBUG: Component delete - Successfully removed prim from parent");

            layer->RemovePrimIfInert(parent);
        }
    } else if (!routingEditTarget.IsNull()) {
        // NOTE: Need to consider the case of edit routing as well probably
        // maybe in the future???
        PXR_NS::UsdEditContext ctx(stage, routingEditTarget);

        // Note: we allow stronger opinion when editing inside a reference or payload.
        //       We detect this by the fact the ref/payload layer is non-local.
        const bool isLocal = stage->HasLocalLayer(routingEditTarget.GetLayer());
        const bool allowStronger = !isLocal;

        if (!UsdUfe::applyCommandRestrictionNoThrow(_prim, "delete", allowStronger))
            return;

        if (!stage->RemovePrim(_prim.GetPath())) {
            const std::string error
                = TfStringPrintf("Failed to delete prim \"%s\".", _prim.GetPath().GetText());
            TF_WARN("%s", error.c_str());
            throw std::runtime_error(error);
        }
    } else {
        if (!UsdUfe::applyCommandRestrictionNoThrow(_prim, "delete"))
            return;

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

    _undoableItem.undo();
}

void UsdUndoDeleteCommand::redo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}

} // namespace USDUFE_NS_DEF
