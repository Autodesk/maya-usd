//
// Copyright 2026 Sony Interactive Entertainment
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

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <string>
#include <unordered_map>

namespace MAYAUSD_NS_DEF {

struct MAYAUSD_CORE_PUBLIC StageStatsOptions
{
    bool                  traverseInstanceProxies = true;
    bool                  includeClasses = false;
    bool                  includeInactive = false;
    bool                  includeOvers = false;
    bool                  countByType = true;
    bool                  drawRender = false;
    bool                  drawProxy = true;
    bool                  drawGuide = false;
    PXR_NS::SdfPathVector excludedPaths;
    PXR_NS::UsdTimeCode   time = PXR_NS::UsdTimeCode::Default();
};

struct MAYAUSD_CORE_PUBLIC StageStats
{
    std::size_t prims = 0;
    std::size_t meshes = 0;
    std::size_t instances = 0;
    std::size_t instanceProxies = 0;
    std::size_t prunedSubtrees = 0;

    // mesh topology
    std::size_t vertices = 0;
    std::size_t faces = 0;
    std::size_t triangles = 0;

    std::unordered_map<std::string, std::size_t> primsByType;

    StageStats& operator+=(const StageStats& rhs);
};

MAYAUSD_CORE_PUBLIC
StageStats computeStageStats(const PXR_NS::UsdPrim& root, const StageStatsOptions& options);

MAYAUSD_CORE_PUBLIC
std::unordered_map<std::string, std::size_t> stageStatsCounts(const StageStats& stats);

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_STAGE_STATISTICS_H
