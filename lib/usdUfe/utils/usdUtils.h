//
// Copyright 2020 Autodesk
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

#ifndef USDUFE_USDUTILS_H
#define USDUFE_USDUTILS_H

#include <usdUfe/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <set>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

//! Return a PrimSpec for the argument prim in the layer containing the stage's current edit target.
USDUFE_PUBLIC
SdfPrimSpecHandle getPrimSpecAtEditTarget(const UsdPrim& prim);

//! Convenience function for printing the list of queried composition arcs in order.
USDUFE_PUBLIC
void printCompositionQuery(const UsdPrim& prim, std::ostream& os);

//! This function automatically updates the SdfPath for different
//  composition arcs (internal references, inherits, specializes) when
//  the path to the concrete prim they refer to has changed.
USDUFE_PUBLIC
bool updateReferencedPath(const UsdPrim& oldPrim, const SdfPath& newPath);

//! This function automatically cleans the SdfPath for different
//  composition arcs (internal references, inherits, specializes) when
//  the path to the concrete prim they refer to becomes invalid.
USDUFE_PUBLIC
bool cleanReferencedPath(const UsdPrim& deletedPrim);

//! Returns true if reference is internal.
USDUFE_PUBLIC
bool isInternalReference(const SdfReference&);

//! Check if the src and dst attributes are connected.
//! \return True, if they are connected.
USDUFE_PUBLIC
bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr);

//! Check if a source connection property is allowed to be removed.
//! \return True, if the property can be removed.
USDUFE_PUBLIC
bool canRemoveSrcProperty(const PXR_NS::UsdAttribute& srcAttr);

//! Check if a destination connection property is allowed to be removed.
//! \return True, if the property can be removed.
USDUFE_PUBLIC
bool canRemoveDstProperty(const PXR_NS::UsdAttribute& dstAttr);

//! Returns the root prims of the stage that must be referenced to bring in all its
//! content: every root prim that is defined (not an over) and not abstract (not a
//! class). Unloaded and inactive root prims are included: the reference carries their
//! payload and active state.
USDUFE_PUBLIC
std::vector<PXR_NS::UsdPrim> getRootPrimsToReference(const PXR_NS::UsdStage& stage);

//! What authorCrossRootOverrides() did, and what it couldn't do, to report it.
struct CrossRootOverridesResult
{
    //! Number of relationships, connections and path expressions overridden.
    size_t nbOverrides = 0;

    //! Instances, in the source stage, made non-instanceable in the destination layer
    //! because their descendants needed overrides.
    std::set<PXR_NS::SdfPath> deinstancedPrims;

    //! Targets, in the source stage, dropped because no reference brings them in
    //! (e.g. under a class, or missing).
    std::set<PXR_NS::SdfPath> unresolvedTargets;

    //! Unloaded prims, in the source stage, whose content couldn't be checked.
    PXR_NS::SdfPathVector unloadedPrims;

    //! Properties, in the source stage, overridden for all variants because their
    //! opinion comes from variants through another composition arc.
    PXR_NS::SdfPathVector overridesOutsideVariants;

    //! Properties, in the source stage, whose override couldn't be authored.
    PXR_NS::SdfPathVector failedOverrides;
};

//! When each root prim of a stage is brought in by its own reference, USD can only
//! remap the relationship targets, attribute connections and path expressions that
//! stay inside a single root prim: anything pointing under another root prim is outside
//! the scope of its reference and is dropped.
//!
//! This function finds those cross-root relationships, connections and path
//! expressions in the source stage, where they are still valid, and authors overrides
//! in the destination layer with every path remapped under the destination prim.
//!
//! \param sourceStage The referenced stage, composed as the references see it (e.g.
//!        without its session layer).
//! \param referencedRoots The root prims of the source stage that are referenced. Each
//!        is expected to be referenced as a child of \p destPrimPath with the same name.
//! \param destLayer The layer holding the references, where the overrides are authored
//!        (e.g. the layer being exported).
//! \param destPrimPath The prim, in \p destLayer, holding the references.
//!
//! The overrides are authored with the Sdf API in a single change block, after reading
//! the source stage, so the stages using \p destLayer are only recomposed once.
USDUFE_PUBLIC
CrossRootOverridesResult authorCrossRootOverrides(
    const PXR_NS::UsdStage&             sourceStage,
    const std::vector<PXR_NS::UsdPrim>& referencedRoots,
    const PXR_NS::SdfLayerHandle&       destLayer,
    const PXR_NS::SdfPath&              destPrimPath);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUTILS_H
