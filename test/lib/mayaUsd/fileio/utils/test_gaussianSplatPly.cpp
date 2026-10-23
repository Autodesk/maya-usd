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
#include <mayaUsd/fileio/utils/gaussianSplatPly.h>

#include <gtest/gtest.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using namespace MAYAUSD_NS::utils;

namespace {

std::string dataPath(const std::string& name) { return std::string(GSPLY_TEST_DATA) + "/" + name; }

std::string outPath(const std::string& name) { return std::string(GSPLY_TEST_OUTPUT) + "/" + name; }

#include "test_gaussianSplatPlyExpected.inc"

void expectCloudEq(const SplatCloud& actual, const SplatCloud& expected)
{
    EXPECT_EQ(actual._shDegree, expected._shDegree);
    ASSERT_EQ(actual.count(), expected.count());
    ASSERT_EQ(actual._positions.size(), expected._positions.size());
    ASSERT_EQ(actual._scales.size(), expected._scales.size());
    ASSERT_EQ(actual._rotations.size(), expected._rotations.size());
    ASSERT_EQ(actual._opacities.size(), expected._opacities.size());
    ASSERT_EQ(actual._shCoeffs.size(), expected._shCoeffs.size());
    for (size_t i = 0; i < actual._positions.size(); ++i)
        EXPECT_NEAR(actual._positions[i], expected._positions[i], 1e-3f)
            << "_positions[" << i << "]";
    for (size_t i = 0; i < actual._scales.size(); ++i)
        EXPECT_NEAR(actual._scales[i], expected._scales[i], 1e-4f) << "_scales[" << i << "]";
    for (size_t i = 0; i < actual._rotations.size(); ++i)
        EXPECT_NEAR(actual._rotations[i], expected._rotations[i], 1e-4f)
            << "_rotations[" << i << "]";
    for (size_t i = 0; i < actual._opacities.size(); ++i)
        EXPECT_NEAR(actual._opacities[i], expected._opacities[i], 1e-4f)
            << "_opacities[" << i << "]";
    for (size_t i = 0; i < actual._shCoeffs.size(); ++i)
        EXPECT_NEAR(actual._shCoeffs[i], expected._shCoeffs[i], 1e-3f) << "_shCoeffs[" << i << "]";
}

struct Sample
{
    const char*       name;
    const SplatCloud* expected;
};

const Sample kSamples[] = {
    { "tennisball_degree0_ascii_10.ply", &kExpectedDegree0 },
    { "tennisball_degree0_binary_10.ply", &kExpectedDegree0 },
    { "tennisball_degree1_ascii_10.ply", &kExpectedDegree1 },
    { "tennisball_degree1_binary_10.ply", &kExpectedDegree1 },
    { "tennisball_degree2_ascii_10.ply", &kExpectedDegree2 },
    { "tennisball_degree2_binary_10.ply", &kExpectedDegree2 },
    { "tennisball_degree3_binary_10.ply", &kExpectedDegree3 },
};

std::string roundTripTestName(const ::testing::TestParamInfo<std::tuple<Sample, bool>>& info)
{
    std::string name = std::get<0>(info.param).name;
    for (auto& c : name)
        if (!std::isalnum(static_cast<unsigned char>(c)))
            c = '_';
    name += std::get<1>(info.param) ? "_to_binary" : "_to_ascii";
    return name;
}

} // namespace

class SplatPlyRoundTrip : public ::testing::TestWithParam<std::tuple<Sample, bool>>
{
};

TEST_P(SplatPlyRoundTrip, LoadMatchesExpectedAndRoundTrips)
{
    const Sample sample = std::get<0>(GetParam());
    const bool   binaryOut = std::get<1>(GetParam());

    SplatCloud  loaded;
    std::string err;
    ASSERT_TRUE(loadSplatPly(dataPath(sample.name), &loaded, &err)) << err;
    expectCloudEq(loaded, *sample.expected);

    const std::string tmpPath = outPath(
        std::string("roundtrip_") + sample.name + (binaryOut ? "_bin.ply" : "_ascii.ply"));
    ASSERT_TRUE(saveSplatPly(loaded, tmpPath, &err, binaryOut)) << err;

    SplatCloud reloaded;
    ASSERT_TRUE(loadSplatPly(tmpPath, &reloaded, &err)) << err;
    expectCloudEq(reloaded, *sample.expected);
}

INSTANTIATE_TEST_SUITE_P(
    Samples,
    SplatPlyRoundTrip,
    ::testing::Combine(::testing::ValuesIn(kSamples), ::testing::Bool()),
    roundTripTestName);
