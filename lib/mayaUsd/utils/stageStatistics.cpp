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
#include "stageStatistics.h"

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <algorithm>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

template <class T> bool _GetAttrValue(const UsdAttribute& attr, const UsdTimeCode& time, T* value)
{
    if (!attr || !attr.IsValid()) {
        return false;
    }

    if (attr.Get(value, time)) {
        return true;
    }

    if (attr.GetNumTimeSamples() > 0) {
        std::vector<double> samples;
        if (attr.GetTimeSamples(&samples) && !samples.empty()) {
            return attr.Get(value, samples.front());
        }
    }

    return false;
}

void _AccumulateMesh(const UsdPrim& prim, const UsdTimeCode& time, StageStats& stats)
{
    const UsdGeomMesh mesh(prim);

    VtVec3fArray points;
    if (_GetAttrValue(mesh.GetPointsAttr(), time, &points)) {
        stats.vertices += points.size();
    }

    VtVec3fArray normals;
    if (!_GetAttrValue(mesh.GetNormalsAttr(), time, &normals)) {
        const UsdGeomPrimvar primvar = UsdGeomPrimvarsAPI(prim).GetPrimvar(UsdGeomTokens->normals);
        if (primvar && primvar.HasValue()) {
            _GetAttrValue(primvar.GetAttr(), time, &normals);
        }
    }
    stats.normals += normals.size();

    VtIntArray faceVertexCounts;
    if (_GetAttrValue(mesh.GetFaceVertexCountsAttr(), time, &faceVertexCounts)) {
        stats.faces += faceVertexCounts.size();
        for (const int count : faceVertexCounts) {
            if (count >= 3) {
                stats.triangles += static_cast<std::size_t>(count - 2);
            }
        }
    }
}

bool _IsDrawn(const UsdGeomImageable& imageable, const StageStatsOptions& options)
{
    TfToken visibility;
    if (imageable.GetVisibilityAttr().Get(&visibility, options.time)
        && visibility == UsdGeomTokens->invisible) {
        return false;
    }

    TfToken purpose;
    if (!imageable.GetPurposeAttr().Get(&purpose) || purpose.IsEmpty()
        || purpose == UsdGeomTokens->default_) {
        return true;
    }

    return std::find(options.drawnPurposes.begin(), options.drawnPurposes.end(), purpose)
        != options.drawnPurposes.end();
}

} // namespace

StageStats& StageStats::operator+=(const StageStats& rhs)
{
    prims += rhs.prims;
    meshes += rhs.meshes;
    vertices += rhs.vertices;
    triangles += rhs.triangles;
    faces += rhs.faces;
    normals += rhs.normals;
    return *this;
}

StageStats ComputeStageStats(const UsdPrim& root, const StageStatsOptions& options)
{
    StageStats stats;

    if (!root || !root.IsValid()) {
        return stats;
    }

    const Usd_PrimFlagsPredicate predicate = options.visibleOnly
        ? UsdTraverseInstanceProxies(UsdPrimDefaultPredicate)
        : UsdTraverseInstanceProxies(UsdPrimAllPrimsPredicate);

    UsdPrimRange range(root, predicate);
    for (auto it = range.begin(); it != range.end(); ++it) {
        const UsdPrim& prim = *it;

        if (prim.IsPseudoRoot()) {
            continue;
        }

        if (options.visibleOnly) {
            const UsdGeomImageable imageable(prim);
            if (imageable && !_IsDrawn(imageable, options)) {
                it.PruneChildren();
                continue;
            }
        }

        ++stats.prims;

        if (prim.IsA<UsdGeomMesh>()) {
            ++stats.meshes;
            _AccumulateMesh(prim, options.time, stats);
        }
    }

    return stats;
}

} // namespace MAYAUSD_NS_DEF