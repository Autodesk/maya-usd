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
#ifndef MAYAUSD_STAGE_STATISTICS_H
#define MAYAUSD_STAGE_STATISTICS_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <cstddef>

namespace MAYAUSD_NS_DEF {

//! \brief Aggregate geometry counts for a USD prim subtree.
struct MAYAUSD_CORE_PUBLIC StageStats
{
    std::size_t prims = 0;
    std::size_t meshes = 0;
    std::size_t vertices = 0;
    std::size_t triangles = 0;
    std::size_t faces = 0;
    std::size_t normals = 0;
    StageStats& operator+=(const StageStats& rhs);
};

//! \brief Controls which prims ComputeStageStats() counts.
struct MAYAUSD_CORE_PUBLIC StageStatsOptions
{
    //! When true, count only what a viewport would draw: skip inactive,
    //! undefined, unloaded and abstract prims, and prune subtrees that are
    //! invisible or carry a purpose absent from drawnPurposes. When false,
    //! report the authored stage instead.
    bool visibleOnly = true;

    PXR_NS::TfTokenVector drawnPurposes;

    PXR_NS::UsdTimeCode time = PXR_NS::UsdTimeCode::Default();
};

//! \brief Count prims and mesh geometry beneath given root.
MAYAUSD_CORE_PUBLIC
StageStats ComputeStageStats(const PXR_NS::UsdPrim& root, const StageStatsOptions& options);

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_STAGE_STATISTICS_H
