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

namespace {

std::string dataPath(const std::string& name) { return std::string(GSPLY_TEST_DATA) + "/" + name; }

std::string outPath(const std::string& name) { return std::string(GSPLY_TEST_OUTPUT) + "/" + name; }

#include "test_gaussianSplatPlyExpected.inc"

void expectCloudEq(const SplatCloud& actual, const SplatCloud& expected)
{
    EXPECT_EQ(actual.shDegree, expected.shDegree);
    ASSERT_EQ(actual.count(), expected.count());
    ASSERT_EQ(actual.positions.size(), expected.positions.size());
    ASSERT_EQ(actual.scales.size(), expected.scales.size());
    ASSERT_EQ(actual.rotations.size(), expected.rotations.size());
    ASSERT_EQ(actual.opacities.size(), expected.opacities.size());
    ASSERT_EQ(actual.shCoeffs.size(), expected.shCoeffs.size());
    for (size_t i = 0; i < actual.positions.size(); ++i)
        EXPECT_NEAR(actual.positions[i], expected.positions[i], 1e-3f) << "positions[" << i << "]";
    for (size_t i = 0; i < actual.scales.size(); ++i)
        EXPECT_NEAR(actual.scales[i], expected.scales[i], 1e-4f) << "scales[" << i << "]";
    for (size_t i = 0; i < actual.rotations.size(); ++i)
        EXPECT_NEAR(actual.rotations[i], expected.rotations[i], 1e-4f) << "rotations[" << i << "]";
    for (size_t i = 0; i < actual.opacities.size(); ++i)
        EXPECT_NEAR(actual.opacities[i], expected.opacities[i], 1e-4f) << "opacities[" << i << "]";
    for (size_t i = 0; i < actual.shCoeffs.size(); ++i)
        EXPECT_NEAR(actual.shCoeffs[i], expected.shCoeffs[i], 1e-3f) << "shCoeffs[" << i << "]";
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

std::string RoundTripTestName(const ::testing::TestParamInfo<std::tuple<Sample, bool>>& info)
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
    RoundTripTestName);
