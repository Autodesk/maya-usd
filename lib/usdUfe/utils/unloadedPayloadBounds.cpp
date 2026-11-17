//
// Copyright 2026 Autodesk
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
#include "unloadedPayloadBounds.h"

#include <pxr/base/gf/range3d.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageLoadRules.h>
#include <pxr/usd/usdGeom/boundable.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/modelAPI.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformCache.h>

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

bool contains(const TfTokenVector& tokens, const TfToken& token)
{
    return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

GfRange3d extentRange(const VtVec3fArray& extent, size_t index)
{
    if (extent.size() < 2 * (index + 1)) {
        return GfRange3d();
    }
    return GfRange3d(GfVec3d(extent[2 * index]), GfVec3d(extent[2 * index + 1]));
}

// Extent of an unloaded prim. extentsHint holds one (min, max) pair per purpose, in the order of
// UsdGeomImageable::GetOrderedPurposeTokens().
GfRange3d unloadedPrimRange(const UsdPrim& prim, UsdTimeCode time, const TfTokenVector& purposes)
{
    VtVec3fArray extent;
    if (UsdGeomBoundable boundable { prim }) {
        if (boundable.GetExtentAttr().Get(&extent, time)) {
            return extentRange(extent, 0);
        }
    }

    GfRange3d range;
    if (!UsdGeomModelAPI(prim).GetExtentsHint(&extent, time)) {
        return range;
    }
    const TfTokenVector& orderedPurposes = UsdGeomImageable::GetOrderedPurposeTokens();
    for (size_t i = 0; i < orderedPurposes.size(); ++i) {
        if (contains(purposes, orderedPurposes[i])) {
            range.UnionWith(extentRange(extent, i));
        }
    }
    return range;
}

bool isInvisible(const UsdPrim& prim, UsdTimeCode time)
{
    const UsdGeomImageable imageable(prim);
    if (!imageable) {
        return false;
    }
    TfToken visibility;
    imageable.GetVisibilityAttr().Get(&visibility, time);
    return visibility == UsdGeomTokens->invisible;
}

} // namespace

namespace USDUFE_NS_DEF {

void addUnloadedPayloadBounds(
    GfBBox3d&                  bbox,
    const UsdPrim&             root,
    UsdTimeCode                time,
    const TfTokenVector&       purposes,
    UnloadedPayloadBoundsSpace space)
{
    if (!root) {
        return;
    }
    const UsdStagePtr stage = root.GetStage();
    if (!stage || stage->GetLoadRules().IsLoadedWithAllDescendants(root.GetPath())) {
        return;
    }

    UsdGeomXformCache xformCache(time);

    // The default predicate requires prims to be loaded, so build one that does not.
    UsdPrimRange range(
        root, UsdTraverseInstanceProxies(UsdPrimIsActive && UsdPrimIsDefined && !UsdPrimIsAbstract));
    for (auto it = range.begin(); it != range.end(); ++it) {
        const UsdPrim& prim = *it;

        // Visibility is inherited, so an invisible prim hides its whole subtree.
        if (isInvisible(prim, time)) {
            it.PruneChildren();
            continue;
        }

        if (prim.IsLoaded()) {
            continue;
        }
        // Descendants of an unloaded prim are unloaded too, and drawn as part of its box.
        it.PruneChildren();

        if (!contains(purposes, UsdGeomImageable(prim).ComputePurpose())) {
            continue;
        }

        const GfRange3d primRange = unloadedPrimRange(prim, time, purposes);
        if (primRange.IsEmpty()) {
            continue;
        }

        GfMatrix4d matrix(1.0);
        if (space == UnloadedPayloadBoundsSpace::World) {
            matrix = xformCache.GetLocalToWorldTransform(prim);
        } else if (prim != root) {
            bool resetsXformStack = false;
            matrix = xformCache.ComputeRelativeTransform(prim, root, &resetsXformStack);
        }

        bbox = GfBBox3d::Combine(bbox, GfBBox3d(primRange, matrix));
    }
}

} // namespace USDUFE_NS_DEF
