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

} // namespace MAYAUSD_NS_DEF
