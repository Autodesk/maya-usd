//
// Copyright 2017 Animal Logic
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
#pragma once

#include "AL/maya/test/Api.h"

#include <maya/MAngle.h>
#include <maya/MDistance.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MTime.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <iostream>

namespace AL {
namespace maya {
namespace test {

#ifdef TRACE_ASSIGNMENT
#define TRACE(X) X;
#else
#define TRACE(X)
#endif

#if 0
#define AL_OUTPUT_TEST_NAME(X) std::cerr << X << std::endl;
#else
#define AL_OUTPUT_TEST_NAME(X)
#endif

#if 0
#define AL_USDMAYA_UNTESTED EXPECT_TRUE(false)
#else
#define AL_USDMAYA_UNTESTED
#endif

// comparisons between MPlugs require stream operators for cpp unit
inline std::ostream& operator<<(std::ostream& os, const MPlug& plug)
{
    return os << plug.name().asChar();
}

// comparisons between MPlugs require stream operators for cpp unit
inline std::ostream& operator<<(std::ostream& os, const MObject& obj)
{
    if (obj == MObject::kNullObj)
        os << "kNullObj";
    else
        os << obj.apiTypeStr();
    return os;
}

#define AL_PATH_CHAR "/"

/// \brief  Used to generate a temporary filepath from the given filename.
/// \param  filename the filename to append to the end of the OS temp dir
/// \return the full path name
AL_MAYA_TEST_PUBLIC const char* buildTempPath(const char* const filename);

/// \brief  Compares two paths to make sure they are the same.  On Windows this will
///         convert all backslashes to forward slashes before doing a string comparison.
/// \param  pathA  the first path to compare
/// \param  pathB  the second path to compare
AL_MAYA_TEST_PUBLIC void compareTempPaths(std::string pathA, std::string pathB);

/// \brief  A method that compares a pair of plugs (on different nodes) that test for equiality.
/// \param  plugA  the first plug to compare
/// \param  plugB  the second plug to compare
/// \param  usdTesting  if true, when plugA refers to a Time, Angle, or Distance attribute, then
///         plugB is assumed to be a double. This is to work around the inability of USD attributes
///         to understand time values. In most cases you should be able to leave this value as
///         false.
AL_MAYA_TEST_PUBLIC void
comparePlugs(const MPlug& plugA, const MPlug& plugB, bool usdTesting = false);

/// \brief  Compares all of the attributes contained on the two nodes to see if the values and types
/// match. This is
///         a way to help test the importers / exporters. i.e. Create a node of a given type, assign
///         random attribute values to it, export, re-import, and then see if the imported matches
///         the exported.
/// \param  nodeA the first node to test
/// \param  nodeB the second node to test
/// \param  includeDefaultAttrs  if true attributes on the node will be tested
/// \param  includeDynamicAttrs  if true dynamically added attributes on the node will be tested
/// \param  usdTesting  if true, when plugA refers to a Time, Angle, or Distance attribute, then
///         plugB is assumed to be a double. This is to work around the inability of USD attributes
///         to understand time values. In most cases you should be able to leave this value as
///         false.
AL_MAYA_TEST_PUBLIC void compareNodes(
    const MObject& nodeA,
    const MObject& nodeB,
    bool           includeDefaultAttrs,
    bool           includeDynamicAttrs,
    bool           usdTesting = false);

/// \brief  Compares all of the attributes contained on the two nodes to see if the values and types
/// match. This is
///         a way to help test the importers / exporters. i.e. Create a node of a given type, assign
///         random attribute values to it, export, re-import, and then see if the imported matches
///         the exported.
/// \param  nodeA the first node to test
/// \param  nodeB the second node to test
/// \param  includeDefaultAttrs  if true attributes on the node will be tested
/// \param  includeDynamicAttrs  if true dynamically added attributes on the node will be tested
/// \param  usdTesting  if true, when plugA refers to a Time, Angle, or Distance attribute, then
///         plugB is assumed to be a double. This is to work around the inability of USD attributes
///         to understand time values. In most cases you should be able to leave this value as
///         false.
AL_MAYA_TEST_PUBLIC void compareNodes(
    const MObject&    nodeA,
    const MObject&    nodeB,
    const char* const attributes[],
    uint32_t          attributeCount,
    bool              usdTesting = false);

// some random number generators
inline bool    randBool() { return (rand() % 2) ? true : false; }
inline float   randFloat() { return float(rand()) / RAND_MAX; }
inline double  randDouble() { return double(rand()) / RAND_MAX; }
inline int8_t  randInt8() { return int8_t(rand()); }
inline int16_t randInt16() { return int16_t(rand()); }
inline int32_t randInt32() { return int32_t(rand()); }
inline int64_t randInt64() { return int64_t(rand()); }

//----------------------------------------------------------------------------------------------------------------------
// some random number generators
//----------------------------------------------------------------------------------------------------------------------
inline void randomBool(MPlug plug)
{
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setBool(randBool()));
    TRACE(std::cout << "randomBool " << plug.name().asChar() << " " << plug.asBool() << std::endl);
}
inline void randomInt8(MPlug plug)
{
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setChar(randInt8()));
    TRACE(std::cout << "randomInt8 " << plug.name().asChar() << " " << plug.asChar() << std::endl);
}
inline void randomInt16(MPlug plug)
{
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setShort(randInt16()));
    TRACE(
        std::cout << "randomInt16 " << plug.name().asChar() << " " << plug.asShort() << std::endl);
}
inline void randomInt32(MPlug plug)
{
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setInt(randInt32()));
    TRACE(std::cout << "randomInt32 " << plug.name().asChar() << " " << plug.asInt() << std::endl);
}
inline void randomInt64(MPlug plug)
{
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setInt(randInt64()));
    TRACE(
        std::cout << "randomInt64 " << plug.name().asChar() << " " << plug.asInt64() << std::endl);
}
inline void randomFloat(MPlug plug)
{
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setFloat(randFloat()));
    TRACE(
        std::cout << "randomFloat " << plug.name().asChar() << " " << plug.asFloat() << std::endl);
}
inline void randomDouble(MPlug plug)
{
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setDouble(randDouble()));
    TRACE(
        std::cout << "randomDouble " << plug.name().asChar() << " " << plug.asDouble()
                  << std::endl);
}
inline void randomAngle(MPlug plug)
{
    MAngle angle(randDouble(), MAngle::kRadians);
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setMAngle(angle));
    TRACE(
        std::cout << "randomAngle " << plug.name().asChar() << " " << plug.asDouble() << std::endl);
}
inline void randomTime(MPlug plug)
{
    MTime mtime(randDouble(), MTime::kSeconds);
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setMTime(mtime));
    TRACE(
        std::cout << "randomTime " << plug.name().asChar() << " " << plug.asDouble() << std::endl);
}
inline void randomDistance(MPlug plug)
{
    MDistance dist(randFloat(), MDistance::kInches);
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setMDistance(dist));
    TRACE(
        std::cout << "randomDistance " << plug.name().asChar() << " " << plug.asDouble()
                  << std::endl);
}
inline void randomString(MPlug plug)
{
    const char* const strings[] = { "dinosaurs", "rock", "and", "so", "do", "cats" };
    EXPECT_EQ(MStatus(MS::kSuccess), plug.setString(strings[rand() % 6]));
    TRACE(
        std::cout << "randomString " << plug.name().asChar() << " " << plug.asString()
                  << std::endl);
}

//----------------------------------------------------------------------------------------------------------------------
// make a random plug full of random data
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_TEST_PUBLIC void randomPlug(MPlug plug);

//----------------------------------------------------------------------------------------------------------------------
// make a random plug full of random data
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_TEST_PUBLIC void
randomNode(MObject node, const char* const attributeNames[], const uint32_t attributeCount);

//----------------------------------------------------------------------------------------------------------------------
// make a random plug full of random data that is animated in the range startFrame -> endFrame
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_TEST_PUBLIC void randomAnimatedNode(
    MObject           node,
    const char* const attributeNames[],
    const uint32_t    attributeCount,
    double            startFrame,
    double            endFrame,
    bool              forceKeyframe = false);
AL_MAYA_TEST_PUBLIC void
randomAnimatedValue(MPlug plug, double startFrame, double endFrame, bool forceKeyframe = false);

//----------------------------------------------------------------------------------------------------------------------
} // namespace test
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
