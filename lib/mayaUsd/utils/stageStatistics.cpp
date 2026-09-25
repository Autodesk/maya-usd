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
#include "stageStatistics.h"

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/work/reduce.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/tokens.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

template <class T>
bool getAttr(const PXR_NS::UsdAttribute& attr, const PXR_NS::UsdTimeCode& time, T* value)
{
    return attr && attr.Get(value, time);
}

bool isExcluded(const PXR_NS::SdfPath& path, const PXR_NS::SdfPathVector& excluded)
{
    for (const PXR_NS::SdfPath& prefix : excluded) {
        if (path.HasPrefix(prefix)) {
            return true;
        }
    }
    return false;
}

bool isDrawn(const PXR_NS::UsdGeomImageable& imageable, const StageStatsOptions& options)
{
    TfToken visibility;
    if (imageable.GetVisibilityAttr().Get(&visibility, options.time)
        && visibility == PXR_NS::UsdGeomTokens->invisible) {
        return false;
    }

    TfToken purpose;
    if (!imageable.GetPurposeAttr().Get(&purpose) || purpose.IsEmpty()
        || purpose == UsdGeomTokens->default_) {
        return true;
    }
    if (purpose == UsdGeomTokens->proxy) {
        return options.drawProxy;
    }
    if (purpose == UsdGeomTokens->render) {
        return options.drawRender;
    }
    if (purpose == UsdGeomTokens->guide) {
        return options.drawGuide;
    }

    return true;
}

void collect(
    const PXR_NS::UsdPrim&        root,
    const StageStatsOptions&      options,
    StageStats*                   stats,
    std::vector<PXR_NS::UsdPrim>* meshes)
{
    Usd_PrimFlagsConjunction base;
    if (!options.includeInactive) {
        base &= UsdPrimIsActive;
    }
    if (!options.includeClasses) {
        base &= !UsdPrimIsAbstract;
    }
    if (!options.includeOvers) {
        base &= UsdPrimIsDefined;
    }

    PXR_NS::UsdPrimRange range = options.traverseInstanceProxies
        ? UsdPrimRange(root, UsdTraverseInstanceProxies(base))
        : UsdPrimRange(root, base);

    for (auto it = range.begin(); it != range.end(); ++it) {
        const PXR_NS::UsdPrim& prim = *it;

        if (prim.IsPseudoRoot()) {
            continue;
        }

        if (!options.excludedPaths.empty() && isExcluded(prim.GetPath(), options.excludedPaths)) {
            ++stats->prunedSubtrees;
            it.PruneChildren();
            continue;
        }

        const PXR_NS::UsdGeomImageable imageable(prim);
        if (imageable && !isDrawn(imageable, options)) {
            ++stats->prunedSubtrees;
            it.PruneChildren();
            continue;
        }

        ++stats->prims;

        if (options.countByType) {
            const TfToken typeName = prim.GetTypeName();
            ++stats->primsByType[typeName.IsEmpty() ? "<untyped>" : typeName.GetString()];
        }

        if (prim.IsInstance()) {
            ++stats->instances;
        }
        if (prim.IsInstanceProxy()) {
            ++stats->instanceProxies;
        }

        if (prim.IsA<UsdGeomMesh>()) {
            meshes->push_back(prim);
        }
    }
}

void collectMeshTopo(
    const PXR_NS::UsdPrim&   prim,
    const StageStatsOptions& options,
    StageStats*              stats)
{
    const UsdGeomMesh mesh(prim);
    ++stats->meshes;

    PXR_NS::VtVec3fArray points;
    if (getAttr(mesh.GetPointsAttr(), options.time, &points)) {
        stats->vertices += points.size();
    }

    PXR_NS::VtIntArray faceVertexCounts;
    if (getAttr(mesh.GetFaceVertexCountsAttr(), options.time, &faceVertexCounts)) {
        stats->faces += faceVertexCounts.size();
        for (const int count : faceVertexCounts) {
            if (count >= 3) {
                stats->triangles += static_cast<std::size_t>(count - 2);
            }
        }
    }
}

// calculate the topology of every collected mesh, in parallel.
void calculateMeshesTopo(
    const std::vector<PXR_NS::UsdPrim>& prims,
    const StageStatsOptions&            options,
    StageStats*                         stats)
{
    if (prims.empty()) {
        return;
    }

    *stats += WorkParallelReduceN(
        StageStats(),
        prims.size(),
        [&prims, &options](std::size_t begin, std::size_t end, StageStats partial) {
            for (std::size_t i = begin; i < end; ++i) {
                collectMeshTopo(prims[i], options, &partial);
            }
            return partial;
        },
        [](const StageStats& lhs, const StageStats& rhs) {
            StageStats joined(lhs);
            joined += rhs;
            return joined;
        });
}

} // namespace

StageStats& StageStats::operator+=(const StageStats& rhs)
{
    prims += rhs.prims;
    meshes += rhs.meshes;
    instances += rhs.instances;
    instanceProxies += rhs.instanceProxies;
    prunedSubtrees += rhs.prunedSubtrees;

    vertices += rhs.vertices;
    faces += rhs.faces;
    triangles += rhs.triangles;

    for (const auto& entry : rhs.primsByType) {
        primsByType[entry.first] += entry.second;
    }

    return *this;
}

StageStats ComputeStageStats(const PXR_NS::UsdPrim& root, const StageStatsOptions& options)
{
    StageStats stats;

    if (!root || !root.IsValid()) {
        return stats;
    }

    std::vector<PXR_NS::UsdPrim> meshes;
    collect(root, options, &stats, &meshes);
    calculateMeshesTopo(meshes, options, &stats);

    return stats;
}

std::unordered_map<std::string, std::size_t> StageStatsCounts(const StageStats& stats)
{
    return {
        { "prims", stats.prims },
        { "meshes", stats.meshes },
        { "instances", stats.instances },
        { "instanceProxies", stats.instanceProxies },
        { "prunedSubtrees", stats.prunedSubtrees },
        { "vertices", stats.vertices },
        { "faces", stats.faces },
        { "triangles", stats.triangles },
    };
}

} // namespace MAYAUSD_NS_DEF
