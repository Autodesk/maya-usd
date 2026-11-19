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
#include <mayaUsd/render/vp2RenderDelegate/gaussianSplatViewCore.h>

#include <pxr/base/gf/rotation.h>
#include <pxr/base/work/threadLimits.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace MAYAUSD_NS::GaussianSplatViewCore;

namespace {

constexpr float kC0 = 0.28209479177387814f;
constexpr float kC1 = 0.4886025119029199f;

using Order = std::vector<uint32_t>;

std::shared_ptr<SplatData> MakeData(const std::vector<GfVec3f>& points, const char* hint = "")
{
    auto data = std::make_shared<SplatData>();
    data->points = VtVec3fArray(points.begin(), points.end());
    data->sortingModeHint = TfToken(hint);
    return data;
}

// Camera at `eye` looking along -Z.
GfMatrix4d ViewAt(const GfVec3d& eye)
{
    GfMatrix4d m;
    m.SetTranslate(-eye);
    return m;
}

// World-to-camera matrix looking from eye to center.
GfMatrix4d LookAt(const GfVec3d& eye, const GfVec3d& center)
{
    GfMatrix4d m;
    m.SetLookAt(eye, center, GfVec3d(0, 1, 0));
    return m;
}

Order Sorted(const SplatData& d, const GfMatrix4d& view)
{
    Order o;
    SortBackToFront(d, view, o);
    return o;
}

// Two splats where depth and distance disagree: camera at the origin looking down -Z.
// p0 has depth 1 but distance ~100. p1 has depth 5 and distance 5.
std::vector<GfVec3f> DepthVsDistancePoints() { return { GfVec3f(100, 0, -1), GfVec3f(0, 0, -5) }; }

struct Lcg
{
    uint32_t s = 12345;
    float    Next()
    {
        s = s * 1664525u + 1013904223u;
        return float(s >> 8) / float(1 << 24);
    }
};

// SH data for one splat at (0,0,-5): band 0 zero, bands 1 only authored by the caller.
std::shared_ptr<SplatData> MakeShData(int degree, size_t stride, const std::vector<GfVec3f>& coeffs)
{
    auto data = MakeData({ GfVec3f(0, 0, -5) });
    data->shStride = stride;
    data->shDegree = degree;
    data->shCoefficients = VtVec3fArray(coeffs.begin(), coeffs.end());
    return data;
}

} // namespace

TEST(GaussianSplatViewCore, ZdepthOrdersBackToFront)
{
    // Overlapping along the view axis. A point behind the camera has a negative depth.
    auto data
        = MakeData({ GfVec3f(0, 0, -1), GfVec3f(0, 0, -5), GfVec3f(0, 0, -3), GfVec3f(0, 0, 2) });
    Order o = Sorted(*data, ViewAt(GfVec3d(0)));
    EXPECT_EQ(o, (Order { 1, 2, 0, 3 }));

    // Equal keys keep index order (stable), NaN keys come last.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    data = MakeData({ GfVec3f(0, 0, -2),
                      GfVec3f(nan, 0, 0),
                      GfVec3f(0, 0, -2),
                      GfVec3f(0, 0, -9),
                      GfVec3f(0, nan, 0),
                      GfVec3f(0, 0, -2) });
    o = Sorted(*data, ViewAt(GfVec3d(0)));
    EXPECT_EQ(o, (Order { 3, 0, 2, 5, 1, 4 }));

    // Empty data is fine.
    data = MakeData({});
    EXPECT_TRUE(Sorted(*data, ViewAt(GfVec3d(0))).empty());
}

TEST(GaussianSplatViewCore, DefaultHintUsesZdepth)
{
    EXPECT_EQ(SortModeFromHint(TfToken()), SortMode::ZDepth);
    EXPECT_EQ(SortModeFromHint(TfToken("zDepth")), SortMode::ZDepth);

    auto data = MakeData(DepthVsDistancePoints());
    EXPECT_EQ(Sorted(*data, ViewAt(GfVec3d(0))), (Order { 1, 0 }));
    data = MakeData(DepthVsDistancePoints(), "zDepth");
    EXPECT_EQ(Sorted(*data, ViewAt(GfVec3d(0))), (Order { 1, 0 }));
}

TEST(GaussianSplatViewCore, DistanceHintsUseCameraDistance)
{
    EXPECT_EQ(SortModeFromHint(TfToken("cameraDistance")), SortMode::CameraDistance);
    EXPECT_EQ(SortModeFromHint(TfToken("rayHitDistance")), SortMode::RayHitDistance);

    for (const char* hint : { "cameraDistance", "rayHitDistance" }) {
        auto data = MakeData(DepthVsDistancePoints(), hint);
        EXPECT_EQ(Sorted(*data, ViewAt(GfVec3d(0))), (Order { 0, 1 })) << hint;
    }

    // Non-uniform scale: the distance is measured in world space. In local space p0 (3,0,0) is
    // closer than p1 (0,0,-4), with x scaled by 3 it is at world distance 9 against 4.
    for (const char* hint : { "cameraDistance", "rayHitDistance" }) {
        auto data = MakeData({ GfVec3f(3, 0, 0), GfVec3f(0, 0, -4) }, hint);
        EXPECT_EQ(Sorted(*data, ViewAt(GfVec3d(0))), (Order { 1, 0 })) << hint;
        data->localToWorld.SetScale(GfVec3d(3, 1, 1));
        EXPECT_EQ(Sorted(*data, ViewAt(GfVec3d(0))), (Order { 0, 1 })) << hint;
        data->localToWorld.SetScale(GfVec3d(1, 1, 3));
        EXPECT_EQ(Sorted(*data, ViewAt(GfVec3d(0))), (Order { 1, 0 })) << hint;
    }
}

TEST(GaussianSplatViewCore, UnknownHintUsesZdepth)
{
    EXPECT_EQ(SortModeFromHint(TfToken("somethingElse")), SortMode::ZDepth);
    auto data = MakeData(DepthVsDistancePoints(), "somethingElse");
    EXPECT_EQ(Sorted(*data, ViewAt(GfVec3d(0))), (Order { 1, 0 }));
}

TEST(GaussianSplatViewCore, ShColorVariesWithDirection)
{
    // Only the third degree 1 coefficient (x term) is set. Splat at (0,0,-5).
    auto data = MakeShData(1, 4, { GfVec3f(0), GfVec3f(0), GfVec3f(0), GfVec3f(1, 1, 1) });

    std::vector<float> front, side;
    ComputeColors(*data, ViewAt(GfVec3d(0, 0, 0)), front);  // direction (0,0,-1)
    ComputeColors(*data, ViewAt(GfVec3d(-5, 0, -5)), side); // direction (1,0,0)
    ASSERT_EQ(front.size(), 4u);
    ASSERT_EQ(side.size(), 4u);
    EXPECT_NEAR(front[0], 0.5f, 1e-5f);
    EXPECT_NEAR(side[0], 0.5f - kC1, 1e-5f);
    EXPECT_NEAR(side[1], 0.5f - kC1, 1e-5f);
    EXPECT_NEAR(side[2], 0.5f - kC1, 1e-5f);
    EXPECT_FLOAT_EQ(side[3], 1.0f);

    // Degree 2 and 3 coefficients are read too: xy term (band 2, index 4) and y*(3x^2-y^2) term
    // (band 3, index 9). Direction (1,0,0): xy=0, so use a diagonal direction.
    std::vector<GfVec3f> c(16, GfVec3f(0));
    c[4] = GfVec3f(0.5f);
    auto               d2 = MakeShData(2, 9, std::vector<GfVec3f>(c.begin(), c.begin() + 9));
    std::vector<float> diag;
    // Camera at (-5/sqrt2*..): direction (sqrt.5, sqrt.5, 0) from camera (-a,-a,-5) with a>0.
    ComputeColors(*d2, ViewAt(GfVec3d(-5, -5, -5)), diag);
    // dir = (5,5,0)/sqrt(50) so x*y = 0.5, C2 for xy is 1.0925484305920792.
    EXPECT_NEAR(diag[0], 0.5f + 1.0925484305920792f * 0.5f * 0.5f, 1e-4f);

    // Degree 3 term index 9: C3 = -0.5900435899266435 * y * (3x^2 - y^2).
    c[4] = GfVec3f(0);
    c[9] = GfVec3f(1, 1, 1);
    auto               d3 = MakeShData(3, 16, c);
    std::vector<float> d3Color;
    ComputeColors(*d3, ViewAt(GfVec3d(-5, -5, -5)), d3Color);
    // x = y = sqrt(0.5): y*(3x^2-y^2) = sqrt(.5) * (1.5 - .5) = sqrt(.5)
    EXPECT_NEAR(d3Color[0], 0.5f - 0.5900435899266435f * std::sqrt(0.5f), 1e-4f);
}

TEST(GaussianSplatViewCore, ShDegreeZeroIsViewIndependent)
{
    // Degree 0 authored: band 1 coefficients present in the array must be ignored.
    auto data = MakeShData(0, 4, { GfVec3f(1, 0, -1), GfVec3f(5), GfVec3f(5), GfVec3f(5) });
    std::vector<float> a, b;
    ComputeColors(*data, ViewAt(GfVec3d(0, 0, 0)), a);
    ComputeColors(*data, ViewAt(GfVec3d(-5, 3, -5)), b);
    EXPECT_EQ(a, b);
    EXPECT_NEAR(a[0], 0.5f + kC0, 1e-5f);
    EXPECT_NEAR(a[1], 0.5f, 1e-5f);
    EXPECT_NEAR(a[2], 0.5f - kC0, 1e-5f);

    // Stride 1: degree above 0 authored but only band 0 available, still view independent.
    auto d1 = MakeShData(3, 1, { GfVec3f(0.5f) });
    ComputeColors(*d1, ViewAt(GfVec3d(0, 0, 0)), a);
    ComputeColors(*d1, ViewAt(GfVec3d(-5, 3, -5)), b);
    EXPECT_EQ(a, b);
}

TEST(GaussianSplatViewCore, MissingShGivesNeutralColor)
{
    auto               data = MakeData({ GfVec3f(0, 0, -5), GfVec3f(1, 0, -5) });
    std::vector<float> c;
    ComputeColors(*data, ViewAt(GfVec3d(0)), c);
    ASSERT_EQ(c.size(), 8u);
    for (size_t i = 0; i < 2; ++i) {
        EXPECT_FLOAT_EQ(c[4 * i + 0], 0.5f);
        EXPECT_FLOAT_EQ(c[4 * i + 1], 0.5f);
        EXPECT_FLOAT_EQ(c[4 * i + 2], 0.5f);
        EXPECT_FLOAT_EQ(c[4 * i + 3], 1.0f);
    }

    // Coefficient count that does not match the splat count: neutral too, no crash.
    data->shStride = 4;
    data->shDegree = 1;
    data->shCoefficients = { GfVec3f(1), GfVec3f(1), GfVec3f(1) };
    data->defaultColor = GfVec3f(0.25f, 0.5f, 0.75f);
    ComputeColors(*data, ViewAt(GfVec3d(0)), c);
    ASSERT_EQ(c.size(), 8u);
    EXPECT_FLOAT_EQ(c[0], 0.25f);
    EXPECT_FLOAT_EQ(c[1], 0.5f);
    EXPECT_FLOAT_EQ(c[2], 0.75f);
    EXPECT_FLOAT_EQ(c[7], 1.0f);
}

TEST(GaussianSplatViewCore, LocalSpaceSortMatchesWorldSpace)
{
    // Rotation, non-uniform scale and translation: zDepth in local space must equal the world
    // space depth ordering. 50000 points exercise several radix chunks.
    GfMatrix4d rot, scale, trans;
    rot.SetRotate(GfRotation(GfVec3d(1, 2, 3), 40.0));
    scale.SetScale(GfVec3d(2.0, 0.5, 3.0));
    trans.SetTranslate(GfVec3d(10, -4, 7));
    const GfMatrix4d l2w = scale * rot * trans;

    Lcg  rng;
    auto data = std::make_shared<SplatData>();
    for (int i = 0; i < 50000; ++i)
        data->points.push_back(
            GfVec3f(rng.Next() * 20 - 10, rng.Next() * 20 - 10, rng.Next() * 20 - 10));
    data->localToWorld = l2w;

    const GfMatrix4d view = LookAt(GfVec3d(30, 20, 40), GfVec3d(5, 0, 0));
    const Order      order = Sorted(*data, view);
    ASSERT_EQ(order.size(), data->points.size());

    Order perm = order;
    std::sort(perm.begin(), perm.end());
    for (size_t i = 0; i < perm.size(); ++i)
        ASSERT_EQ(perm[i], i);

    auto worldDepth = [&](uint32_t i) {
        const GfVec3d p = view.Transform(l2w.Transform(GfVec3d(data->points[i])));
        return -p[2];
    };
    size_t inversions = 0;
    for (size_t k = 1; k < order.size(); ++k)
        if (worldDepth(order[k - 1]) < worldDepth(order[k]) - 1e-3)
            ++inversions;
    EXPECT_EQ(inversions, 0u);

    // Distance hint with a uniform scale: farthest first in world space.
    GfMatrix4d uscale;
    uscale.SetScale(2.5);
    data->localToWorld = uscale * rot * trans;
    data->sortingModeHint = TfToken("cameraDistance");
    const Order   dOrder = Sorted(*data, view);
    const GfVec3d camWorld(30, 20, 40);
    auto          worldDist = [&](uint32_t i) {
        return (data->localToWorld.Transform(GfVec3d(data->points[i])) - camWorld).GetLengthSq();
    };
    inversions = 0;
    for (size_t k = 1; k < dOrder.size(); ++k)
        if (worldDist(dOrder[k - 1]) < worldDist(dOrder[k]) * (1.0 - 1e-5))
            ++inversions;
    EXPECT_EQ(inversions, 0u);
}

// Measurement only. Disabled: remove the DISABLED_ prefix locally to run it.
TEST(GaussianSplatViewCore, DISABLED_SortMeasurement)
{
    for (size_t n : { size_t(1000000), size_t(5000000) }) {
        SplatData data;
        Lcg       rng;
        data.points.reserve(n);
        for (size_t i = 0; i < n; ++i)
            data.points.push_back(GfVec3f(rng.Next() * 100, rng.Next() * 100, rng.Next() * 100));
        std::vector<double> ms;
        Order               o;
        for (int run = 0; run < 12; ++run) {
            const GfMatrix4d view = ViewAt(GfVec3d(50 + run, 20, 300));
            const auto       t0 = std::chrono::steady_clock::now();
            SortBackToFront(data, view, o);
            const auto t1 = std::chrono::steady_clock::now();
            if (run > 0) // first run warms up the thread pool
                ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        }
        std::sort(ms.begin(), ms.end());
        printf(
            "MEASURE n=%zu threads=%u min=%.2f median=%.2f max=%.2f ms\n",
            n,
            WorkGetConcurrencyLimit(),
            ms.front(),
            ms[ms.size() / 2],
            ms.back());
    }
}
