//
// Copyright 2022 Autodesk
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
#include "variants.h"

#include <mayaUsd/ufe/Global.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/pcp/node.h>
#include <pxr/usd/pcp/primIndex.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/stage.h>

/// General utility functions for variants
namespace MAYAUSD_NS_DEF {

void applyToAllVariants(
    const PXR_NS::UsdPrim&       primWithVariants,
    bool                         includeNonVariant,
    const std::function<void()>& func)
{
    // Record if we saw at least one variant to apply the function on.
    // Used when non-variant are included.
    bool atLeastOneVariant = false;

    // Clear the edit flag in all other variants, in all variant sets if any.
    PXR_NS::UsdStagePtr    stage = primWithVariants.GetStage();
    PXR_NS::UsdVariantSets variantSets = primWithVariants.GetVariantSets();
    for (const std::string& variantSetName : variantSets.GetNames()) {

        PXR_NS::UsdVariantSet variantSet = primWithVariants.GetVariantSet(variantSetName);

        // Make sure to restore the current selected variant even if the face
        // of exceptions.
        AutoVariantRestore variantRestore(variantSet);

        for (const std::string& variantName : variantSet.GetVariantNames()) {
            if (variantSet.SetVariantSelection(variantName)) {
                PXR_NS::UsdEditTarget target = stage->GetEditTarget();

                PXR_NS::UsdEditContext switchEditContext(
                    stage, variantSet.GetVariantEditTarget(target.GetLayer()));

                func();
                atLeastOneVariant = true;
            }
        }
    }

    // When not a single variant was found and the caller wants to apply
    // the function even in the absence of vairants, call it now.
    if (includeNonVariant && !atLeastOneVariant)
        func();
}

PXR_NS::UsdEditTarget
getEditTargetForVariants(const PXR_NS::UsdPrim& prim, const PXR_NS::SdfLayerHandle& layer)
{
    PXR_NS::UsdEditTarget editTarget(layer);

#ifdef MAYAUSD_DEBUG_EDIT_TARGET_FOR_VARIANTS
    std::vector<std::string> variantPaths;
#endif

    for (const PXR_NS::SdfPath& p : prim.GetPath().GetAncestorsRange()) {
        PXR_NS::UsdPrim          ancestor = prim.GetStage()->GetPrimAtPath(p);
        PXR_NS::UsdVariantSets   variantSets = ancestor.GetVariantSets();
        std::vector<std::string> setNames = variantSets.GetNames();
        for (const std::string& setName : setNames) {
            PXR_NS::UsdVariantSet variant = variantSets.GetVariantSet(setName);
            const std::string     selection = variant.GetVariantSelection();
#ifdef MAYAUSD_DEBUG_EDIT_TARGET_FOR_VARIANTS
            variantPaths.emplace_back(setName + std::string("=") + selection);
#endif
            editTarget = editTarget.ComposeOver(variant.GetVariantEditTarget(layer));
        }
    }

#ifdef MAYAUSD_DEBUG_EDIT_TARGET_FOR_VARIANTS
    using namespace PXR_NS;
    TF_STATUS(
        "edit target for variants for %s: %s",
        prim.GetPath().GetText(),
        PXR_NS::TfStringJoin(variantPaths).c_str());
#endif

    return editTarget;
}

PXR_NS::SdfPath getVariantPath(
    const Ufe::Path&   path,
    const std::string& variantSetName,
    const std::string& variantSelection)
{
    if (path.runTimeId() != MayaUsd::ufe::getUsdRunTimeId())
        return {};

    const Ufe::Path::Segments& segments = path.getSegments();
    PXR_NS::SdfPath            primPath(segments.back().string());
    return primPath.AppendVariantSelection(variantSetName, variantSelection);
}

} // namespace MAYAUSD_NS_DEF
