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
#ifndef MAYAUSD_UTILS_VARIANTS_H
#define MAYAUSD_UTILS_VARIANTS_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/variantSets.h>

#include <functional>
#include <string>

/// General utility functions for variants
namespace MAYAUSD_NS_DEF {

/*! \brief Apply a function to all variants on a prim.

    Optionally, if includeNonVariant is true, apply it even if the prim
    has no variant at all, which is useful when you want to edit something
    on all variations of a prim, even if there are no variations.
*/
MAYAUSD_CORE_PUBLIC
void applyToAllVariants(
    const PXR_NS::UsdPrim&       primWithVariants,
    bool                         includeNonVariant,
    const std::function<void()>& func);

/*! \brief Keep track of the current variant and restore it on destruction.

    The reason we don't make this an variant auto-switcher is that
    switching variant recomposes the stage and one main user of the
    restore is visiting all variants, which would double the number
    of recompose if we restored the variant between each visit.

    IOW, for a set with three variants A, B, C, this design permits
    the switch Current -> A -> B -> C -> Current instead of doing
    Current -> A -> Current -> B -> Current -> C -> Current.
*/

class AutoVariantRestore
{
public:
    MAYAUSD_CORE_PUBLIC
    AutoVariantRestore(PXR_NS::UsdVariantSet& variantSet)
        : _variantSet(variantSet)
        , _variant(variantSet.GetVariantSelection())
    {
    }

    MAYAUSD_CORE_PUBLIC
    ~AutoVariantRestore() { _variantSet.SetVariantSelection(_variant); }

private:
    PXR_NS::UsdVariantSet& _variantSet;
    std::string            _variant;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UTILS_VARIANTS_H
