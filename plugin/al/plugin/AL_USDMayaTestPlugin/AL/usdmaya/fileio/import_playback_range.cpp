//
// Copyright 2019 Animal Logic
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
#include "test_usdmaya.h"

#include <maya/MAnimControl.h>
#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

#include <fstream>
#include <set>
#include <string>

using AL::maya::test::buildTempPath;

static const char* const g_staticUsd =
    R"(#usda 1.0
(
    defaultPrim = "root"
)
def Xform "root"
{
}
)";

static const char* const g_animatedUsd =
    R"(#usda 1.0
(
    defaultPrim = "root"
    endTimeCode = 5
    startTimeCode = 1
)
def Xform "root"
{
}
)";

// set default testing frame range and construct the import command
MString setupScene(
    const char* const sourceUsd,
    const char* const testName,
    const char* const options,
    const char* const timeRangeOption)
{
    MFileIO::newFile(true);
    // set a playback range different from the usd file
    MAnimControl::setMinMaxTime(10.0, 20.0);
    EXPECT_EQ(MAnimControl::minTime(), MTime(10.0));
    EXPECT_EQ(MAnimControl::maxTime(), MTime(20.0));

    std::string usdFileName(
        std::string("AL_USDMayaTests_import_playback_range_") + testName + ".usda");
    const std::string tempPath = buildTempPath(usdFileName.c_str());
    {
        std::ofstream ofs(tempPath);
        ofs << sourceUsd;
    }

    MString importCmd = "file -import -type \"AL usdmaya import\" -ignoreVersion -pr ";
    importCmd += MString("-options \"") + options + "\" ";
    if (timeRangeOption) {
        importCmd += MString("-importTimeRange \"") + timeRangeOption + "\" ";
    }
    importCmd += "\"";
    importCmd += tempPath.c_str();
    importCmd += "\";";
    return importCmd;
}

// test importing a static USD
// no playback range should be touched
TEST(import_playback_range, static_scene)
{
    MString importCmd = setupScene(g_staticUsd, "static_scene", "Import_Animations=1;", "override");
    ASSERT_TRUE(MS::kSuccess == MGlobal::executeCommand(importCmd, true));
    // expect to match the min time from scene - no animation from USD
    EXPECT_EQ(MAnimControl::minTime(), MTime(10.0));
    // expect to match the max time from scene - no animation from USD
    EXPECT_EQ(MAnimControl::maxTime(), MTime(20.0));
}

// test importing an animated USD but without the animation option
// no playback range should be touched
TEST(import_playback_range, no_animation)
{
    MString importCmd
        = setupScene(g_animatedUsd, "no_animation", "Import_Animations=0;", "override");
    ASSERT_TRUE(MS::kSuccess == MGlobal::executeCommand(importCmd, true));
    // expect to match the min time from scene - no changes
    EXPECT_EQ(MAnimControl::minTime(), MTime(10.0));
    // expect to match the max time from scene - no changes
    EXPECT_EQ(MAnimControl::maxTime(), MTime(20.0));
}

// test importing an animated USD but maintain the playback range
// no playback range should be touched
TEST(import_playback_range, unchanged_range)
{
    MString importCmd
        = setupScene(g_animatedUsd, "unchanged_range", "Import_Animations=1;", nullptr);
    ASSERT_TRUE(MS::kSuccess == MGlobal::executeCommand(importCmd, true));
    // expect to match the min time from scene - no changes
    EXPECT_EQ(MAnimControl::minTime(), MTime(10.0));
    // expect to match the max time from scene - no changes
    EXPECT_EQ(MAnimControl::maxTime(), MTime(20.0));
}

// test importing an animated USD and override playback range
// playback range should match imported USD
TEST(import_playback_range, override_animation_range)
{
    MString importCmd
        = setupScene(g_animatedUsd, "override_animation_range", "Import_Animations=1;", "override");
    ASSERT_TRUE(MS::kSuccess == MGlobal::executeCommand(importCmd, true));
    // expect to match the min time from USD
    EXPECT_EQ(MAnimControl::minTime(), MTime(1.0));
    // expect to match the max time from USD
    EXPECT_EQ(MAnimControl::maxTime(), MTime(5.0));
}

// test importing an animated USD and combine playback range
// playback range should be extended
TEST(import_playback_range, combine_animation_range)
{
    MString importCmd
        = setupScene(g_animatedUsd, "combine_animation_range", "Import_Animations=1;", "combine");
    ASSERT_TRUE(MS::kSuccess == MGlobal::executeCommand(importCmd, true));
    // expect to match the min time from USD
    EXPECT_EQ(MAnimControl::minTime(), MTime(1.0));
    // expect to match the max time from scene
    EXPECT_EQ(MAnimControl::maxTime(), MTime(20.0));
}
