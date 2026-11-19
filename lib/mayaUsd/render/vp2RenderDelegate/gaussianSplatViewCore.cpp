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
#include "gaussianSplatViewCore.h"

#include <pxr/base/gf/math.h>
#include <pxr/base/work/loops.h>

#include <algorithm>
#include <cmath>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace GaussianSplatViewCore {

namespace {

// Spherical harmonics constants (3DGS reference implementation).
constexpr float kSH_C0 = 0.28209479177387814f;
constexpr float kSH_C1 = 0.4886025119029199f;
constexpr float kSH_C2[5] = { 1.0925484305920792f,
                              -1.0925484305920792f,
                              0.31539156525252005f,
                              -1.0925484305920792f,
                              0.5462742152960396f };
constexpr float kSH_C3[7]
    = { -0.5900435899266435f, 2.890611442640554f, -0.4570457994644658f, 0.3731763325901154f,
        -0.4570457994644658f, 1.445305721320277f, -0.5900435899266435f };

constexpr size_t   kKeyGrain = 8192;
constexpr size_t   kColorGrain = 2048;
constexpr uint32_t kNanKey = 0xFFFFFFFFu;

// Camera position and view direction expressed for prim local space.
struct LocalCamera
{
    GfVec3f position;
    // Direction vector d such that dot(p - position, d) is the depth along the view direction of
    // the world space camera for any local point p, also with a non-uniform scale.
    GfVec3f forward;
    // Linear part of localToWorld (row-vector convention: worldOffset = localOffset * linear).
    float linear[3][3];
};

GfMatrix4d SafeInverse(const GfMatrix4d& m)
{
    double     det = 0.0;
    GfMatrix4d inv = m.GetInverse(&det, 1e-30);
    return std::abs(det) > 1e-30 ? inv : GfMatrix4d(1.0);
}

LocalCamera ToLocalCamera(const SplatData& data, const GfMatrix4d& viewMatrix)
{
    const GfMatrix4d camToWorld = SafeInverse(viewMatrix);
    const GfVec3d    camWorld = camToWorld.ExtractTranslation();
    const GfVec3d    fwdWorld = camToWorld.TransformDir(GfVec3d(0, 0, -1));

    const GfMatrix4d worldToLocal = SafeInverse(data.localToWorld);
    LocalCamera      cam;
    cam.position = GfVec3f(worldToLocal.Transform(camWorld));
    // worldOffset = localOffset * L, so dot(worldOffset, f) = dot(localOffset, L * f).
    cam.forward = GfVec3f(data.localToWorld.GetTranspose().TransformDir(fwdWorld));
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            cam.linear[r][c] = static_cast<float>(data.localToWorld[r][c]);
        }
    }
    return cam;
}

// Maps a float to an unsigned key where a larger float gives a smaller key (farthest first).
// NaN gives the largest key, so it ends last.
inline uint32_t DescendingKey(float d)
{
    if (std::isnan(d)) {
        return kNanKey;
    }
    if (d == 0.0f) {
        d = 0.0f; // fold -0 into +0
    }
    uint32_t bits;
    static_assert(sizeof(bits) == sizeof(d), "float and uint32_t must have the same size");
    // Byte-wise copy (char access is allowed to alias) to reinterpret the float as its bits.
    std::copy_n(
        reinterpret_cast<const unsigned char*>(&d),
        sizeof(bits),
        reinterpret_cast<unsigned char*>(&bits));
    const uint32_t ascending = (bits & 0x80000000u) ? ~bits : (bits | 0x80000000u);
    return ~ascending;
}

// Stable LSD radix sort of the indices 0..n-1 by `keys` (ascending), 4 passes of 8 bits.
void RadixSortIndices(std::vector<uint32_t>& keysIn, std::vector<uint32_t>& orderOut)
{
    const size_t n = keysIn.size();
    orderOut.resize(n);
    if (n == 0) {
        return;
    }

    std::vector<uint32_t> keys[2] = { std::move(keysIn), std::vector<uint32_t>(n) };
    std::vector<uint32_t> vals[2] = { std::vector<uint32_t>(n), std::vector<uint32_t>(n) };

    WorkParallelForN(
        n,
        [&](size_t begin, size_t end) {
            for (size_t i = begin; i != end; ++i) {
                vals[0][i] = static_cast<uint32_t>(i);
            }
        },
        kKeyGrain);

    const size_t        chunks = std::max<size_t>(1, std::min<size_t>(64, n / 16384));
    const size_t        chunkSize = (n + chunks - 1) / chunks;
    std::vector<size_t> hist(chunks * 256);
    int                 src = 0;

    for (int pass = 0; pass < 4; ++pass) {
        const int shift = pass * 8;
        std::fill(hist.begin(), hist.end(), size_t(0));
        WorkParallelForN(
            chunks,
            [&](size_t cBegin, size_t cEnd) {
                for (size_t c = cBegin; c != cEnd; ++c) {
                    const size_t b = c * chunkSize, e = std::min(n, b + chunkSize);
                    size_t*      h = &hist[c * 256];
                    for (size_t i = b; i < e; ++i) {
                        ++h[(keys[src][i] >> shift) & 0xFF];
                    }
                }
            },
            1);

        // Exclusive prefix over digits, then chunks, so the scatter is stable.
        size_t running = 0;
        bool   singleBucket = false;
        for (size_t d = 0; d < 256; ++d) {
            const size_t before = running;
            for (size_t c = 0; c < chunks; ++c) {
                const size_t cnt = hist[c * 256 + d];
                hist[c * 256 + d] = running;
                running += cnt;
            }
            singleBucket = singleBucket || (running - before == n);
        }
        if (singleBucket) {
            continue; // every key has the same digit: the pass changes nothing
        }

        WorkParallelForN(
            chunks,
            [&](size_t cBegin, size_t cEnd) {
                for (size_t c = cBegin; c != cEnd; ++c) {
                    const size_t b = c * chunkSize, e = std::min(n, b + chunkSize);
                    size_t*      h = &hist[c * 256];
                    for (size_t i = b; i < e; ++i) {
                        const size_t pos = h[(keys[src][i] >> shift) & 0xFF]++;
                        keys[1 - src][pos] = keys[src][i];
                        vals[1 - src][pos] = vals[src][i];
                    }
                }
            },
            1);
        src = 1 - src;
    }
    orderOut = std::move(vals[src]);
}

// Effective SH degree, or -1 when the coefficients are missing or do not match the splat count.
int UsableShDegree(const SplatData& data)
{
    const size_t count = data.Count();
    if (data.shStride == 0 || data.shCoefficients.size() != data.shStride * count) {
        return -1;
    }
    int fromStride = 0;
    if (data.shStride >= 16) {
        fromStride = 3;
    } else if (data.shStride >= 9) {
        fromStride = 2;
    } else if (data.shStride >= 4) {
        fromStride = 1;
    }
    return std::max(0, std::min({ data.shDegree, fromStride, 3 }));
}

inline GfVec3f EvaluateSh(int degree, const GfVec3f* sh, const GfVec3f& dir)
{
    GfVec3f result = kSH_C0 * sh[0];
    if (degree > 0) {
        const float x = dir[0], y = dir[1], z = dir[2];
        result = result - kSH_C1 * y * sh[1] + kSH_C1 * z * sh[2] - kSH_C1 * x * sh[3];
        if (degree > 1) {
            const float xx = x * x, yy = y * y, zz = z * z;
            const float xy = x * y, yz = y * z, xz = x * z;
            result = result + kSH_C2[0] * xy * sh[4] + kSH_C2[1] * yz * sh[5]
                + kSH_C2[2] * (2.0f * zz - xx - yy) * sh[6] + kSH_C2[3] * xz * sh[7]
                + kSH_C2[4] * (xx - yy) * sh[8];
            if (degree > 2) {
                result = result + kSH_C3[0] * y * (3.0f * xx - yy) * sh[9]
                    + kSH_C3[1] * xy * z * sh[10] + kSH_C3[2] * y * (4.0f * zz - xx - yy) * sh[11]
                    + kSH_C3[3] * z * (2.0f * zz - 3.0f * xx - 3.0f * yy) * sh[12]
                    + kSH_C3[4] * x * (4.0f * zz - xx - yy) * sh[13]
                    + kSH_C3[5] * z * (xx - yy) * sh[14]
                    + kSH_C3[6] * x * (xx - 3.0f * yy) * sh[15];
            }
        }
    }
    return result + GfVec3f(0.5f);
}

inline float Clamp01(float v)
{
    // NaN becomes 0.
    return v > 0.0f ? (v < 1.0f ? v : 1.0f) : 0.0f;
}

//! Values of the sort mode hint.
constexpr const char* kCameraDistanceHint = "cameraDistance";
constexpr const char* kRayHitDistanceHint = "rayHitDistance";

} // namespace

SortMode SortModeFromHint(const TfToken& hint)
{
    if (hint == kCameraDistanceHint) {
        return SortMode::CameraDistance;
    }
    if (hint == kRayHitDistanceHint) {
        return SortMode::RayHitDistance;
    }
    return SortMode::ZDepth;
}

void SortBackToFront(
    const SplatData&       data,
    const GfMatrix4d&      viewMatrix,
    std::vector<uint32_t>& order)
{
    const size_t      n = data.Count();
    const SortMode    mode = SortModeFromHint(data.sortingModeHint);
    const LocalCamera cam = ToLocalCamera(data, viewMatrix);
    const GfVec3f*    points = data.points.data();

    std::vector<uint32_t> keys(n);
    WorkParallelForN(
        n,
        [&](size_t begin, size_t end) {
            for (size_t i = begin; i != end; ++i) {
                const GfVec3f off = points[i] - cam.position;
                float         d;
                if (mode == SortMode::ZDepth) {
                    d = GfDot(off, cam.forward);
                } else {
                    // Distance in world space, also with a non-uniform scale or a shear.
                    const GfVec3f w(
                        off[0] * cam.linear[0][0] + off[1] * cam.linear[1][0]
                            + off[2] * cam.linear[2][0],
                        off[0] * cam.linear[0][1] + off[1] * cam.linear[1][1]
                            + off[2] * cam.linear[2][1],
                        off[0] * cam.linear[0][2] + off[1] * cam.linear[1][2]
                            + off[2] * cam.linear[2][2]);
                    d = GfDot(w, w);
                }
                keys[i] = DescendingKey(d);
            }
        },
        kKeyGrain);

    RadixSortIndices(keys, order);
}

void ComputeColors(const SplatData& data, const GfMatrix4d& viewMatrix, std::vector<float>& rgba)
{
    const size_t      n = data.Count();
    const int         degree = UsableShDegree(data);
    const LocalCamera cam = ToLocalCamera(data, viewMatrix);
    const GfVec3f*    points = data.points.data();
    const GfVec3f*    sh = data.shCoefficients.data();
    const size_t      stride = data.shStride;
    const GfVec3f     neutral = data.defaultColor;

    rgba.resize(n * 4);
    float* out = rgba.data();
    WorkParallelForN(
        n,
        [&](size_t begin, size_t end) {
            for (size_t i = begin; i != end; ++i) {
                GfVec3f color = neutral;
                if (degree >= 0) {
                    GfVec3f     dir = points[i] - cam.position;
                    const float len = dir.GetLength();
                    // A splat at the camera has no direction: use band 0 only.
                    if (len > 1e-12f) {
                        dir /= len;
                    } else {
                        dir = GfVec3f(0.0f);
                    }
                    color = EvaluateSh(degree, sh + i * stride, dir);
                }
                out[4 * i + 0] = Clamp01(color[0]);
                out[4 * i + 1] = Clamp01(color[1]);
                out[4 * i + 2] = Clamp01(color[2]);
                out[4 * i + 3] = 1.0f;
            }
        },
        kColorGrain);
}

} // namespace GaussianSplatViewCore

} // namespace MAYAUSD_NS_DEF
