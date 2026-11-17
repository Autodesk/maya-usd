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
#ifndef USDUFE_UNLOADEDPAYLOADBOUNDS_H
#define USDUFE_UNLOADEDPAYLOADBOUNDS_H

#include <usdUfe/base/api.h>

#include <pxr/base/gf/bbox3d.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

namespace USDUFE_NS_DEF {

/*! \brief The space in which addUnloadedPayloadBounds() expresses the bounds.
 */
enum class UnloadedPayloadBoundsSpace
{
    //! The local space of the root prim, excluding its own transform, as
    //! UsdGeomBBoxCache::ComputeUntransformedBound().
    Untransformed,
    //! USD world space, as UsdGeomBBoxCache::ComputeWorldBound().
    World
};

/*! \brief Add to \p bbox the bounds of the unloaded payloads at or under \p root.
 *
 *  UsdGeomBBoxCache ignores unloaded prims, as they have no composed geometry. Viewports draw them
 *  as boxes from their extentsHint (or authored extent), so this adds those boxes, letting
 *  framing and the bounds of a fully unloaded stage match what is drawn.
 *
 *  Only the topmost unloaded prims are considered. Invisible prims, prims whose purpose is not in
 *  \p purposes, and prims with neither extentsHint nor extent are skipped. The extentsHint entries
 *  used are the ones for \p purposes.
 */
USDUFE_PUBLIC
void addUnloadedPayloadBounds(
    PXR_NS::GfBBox3d&            bbox,
    const PXR_NS::UsdPrim&       root,
    PXR_NS::UsdTimeCode          time,
    const PXR_NS::TfTokenVector& purposes,
    UnloadedPayloadBoundsSpace   space);

} // namespace USDUFE_NS_DEF

#endif
