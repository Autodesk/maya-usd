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
#ifndef HDVP2_GAUSSIAN_SPLAT_INSTANCING_H
#define HDVP2_GAUSSIAN_SPLAT_INSTANCING_H

#include <pxr/pxr.h>

#if PXR_VERSION >= 2605

#include "mayaPrimCommon.h"

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MFloatArray.h>

#include <memory>
#include <vector>

namespace MAYAUSD_NS_DEF {

//! Blend factor of the selection color into the splat color of a selected prim or instance.
constexpr float kHdVP2GaussianSplatSelectionTintAlpha = 0.05f;

/*! \brief  Instancing state of one splat draw item, built on a worker for its commit.

    The instance matrices (with the world matrix of the prim) and the UFE identifiers of the
    drawn instances are in the commit state of the item.
*/
struct HdVP2GaussianSplatInstancing
{
    //! True when the Rprim has an instancer. It never changes during the life of the Rprim: an
    //! edit that stops or starts instancing a prim replaces its Rprims (see design D9).
    bool _instanced = false;
    //! True when the instancer draws no instance: the item is disabled.
    bool _noInstances = false;
    //! One float4 tint per drawn instance (shaded items only), rgb and blend factor.
    std::shared_ptr<MFloatArray> _tints;
    //! True when the tints differ from the ones the item draws.
    bool _tintsChanged = false;
    //! USD instance id of each drawn instance, empty when no instance is dropped.
    std::shared_ptr<std::vector<unsigned int>> _mayaToUsdIds;
};

/*! \brief  Apply the instancing state to a render item. Main thread only.

    Run after the geometry of the item is set. An instanced item gets its instance array (set
    once, then updated in place while the count does not change), its UFE identifiers and its
    per-instance tints. A plain item gets its world matrix.
*/
void HdVP2GaussianSplatCommitInstancing(
    PXR_NS::ProxyRenderDelegate&        drawScene,
    MHWRender::MRenderItem&             renderItem,
    const PXR_NS::MayaUsdCommitState&   stateToCommit,
    const HdVP2GaussianSplatInstancing& instancing,
    bool                                isWire,
    const PXR_NS::SdfPath&              id);

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605

#endif // HDVP2_GAUSSIAN_SPLAT_INSTANCING_H
