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
#ifndef HDVP2_GAUSSIAN_SPLAT_VIEW_CORE_H
#define HDVP2_GAUSSIAN_SPLAT_VIEW_CORE_H

// Pure math of the Gaussian splat view-dependent work: back-to-front sort (parallel radix) and
// spherical harmonics (SH) evaluation for one view. No Maya types: the view cache converts the Maya
// values before calling it.
//
// Conventions: matrices are row-vector (USD and Maya): worldPoint = localPoint * localToWorld,
// cameraPoint = worldPoint * viewMatrix. The camera looks down its local -Z axis.

#include <mayaUsd/mayaUsd.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace MAYAUSD_NS_DEF {

namespace GaussianSplatViewCore {

/// How the splats are ordered (UsdVol sortingModeHint).
enum class SortMode
{
    ZDepth,         ///< depth along the view direction (default)
    CameraDistance, ///< squared world space distance from the camera
    RayHitDistance  ///< squared world space distance from the camera
};

/// Maps a sortingModeHint token to a mode. Empty or unknown tokens give ZDepth.
SortMode SortModeFromHint(const PXR_NS::TfToken& hint);

/// Immutable splat data handed over by the commit lambda. Never modified after it is published.
/// The large arrays are copy-on-write (VtArray): copying a SplatData to change the matrix or the
/// hint shares them without copying the elements.
struct SplatData
{
    /// Splat centres in prim local space. Defines the splat count.
    PXR_NS::VtVec3fArray points;
    /// SH coefficients, `shStride` per splat, band-major (band 0 first). Empty when not authored.
    PXR_NS::VtVec3fArray shCoefficients;
    /// Coefficients per splat in `shCoefficients` (1, 4, 9 or 16). 0 when not authored.
    size_t shStride = 0;
    /// Authored SH degree (0 to 3). The degree used is also limited by `shStride`.
    int shDegree = 0;
    /// Authored sortingModeHint.
    PXR_NS::TfToken sortingModeHint;
    /// Prim local-to-world matrix.
    PXR_NS::GfMatrix4d localToWorld = PXR_NS::GfMatrix4d(1.0);
    /// Color used when no usable SH coefficients are authored.
    PXR_NS::GfVec3f defaultColor = PXR_NS::GfVec3f(0.5f);

    size_t Count() const { return points.size(); }
};

/// Writes the splat indices of `data` ordered back to front (farthest first) for `viewMatrix`.
/// Stable: equal keys keep index order. NaN keys come last. Uses a parallel radix sort.
void SortBackToFront(
    const SplatData&          data,
    const PXR_NS::GfMatrix4d& viewMatrix,
    std::vector<uint32_t>&    order);

/// Writes `Count()*4` floats (RGBA, alpha 1, RGB clamped to [0,1]) evaluated for `viewMatrix`.
void ComputeColors(
    const SplatData&          data,
    const PXR_NS::GfMatrix4d& viewMatrix,
    std::vector<float>&       rgba);

} // namespace GaussianSplatViewCore

} // namespace MAYAUSD_NS_DEF

#endif // HDVP2_GAUSSIAN_SPLAT_VIEW_CORE_H
