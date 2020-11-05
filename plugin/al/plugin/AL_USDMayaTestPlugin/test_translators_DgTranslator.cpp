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
#include "AL/maya/utils/NodeHelper.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MAngle.h>
#include <maya/MDGModifier.h>
#include <maya/MFloatMatrix.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatVector.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MLibrary.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>
#include <maya/MVector.h>

#include <cstring>

using AL::maya::test::buildTempPath;
using AL::maya::test::comparePlugs;
using AL::maya::test::randBool;
using AL::maya::test::randDouble;
using AL::maya::test::randFloat;
using AL::maya::test::randInt16;
using AL::maya::test::randInt32;
using AL::maya::test::randInt64;
using AL::maya::test::randInt8;
using AL::maya::utils::NodeHelper;
using AL::usdmaya::fileio::ExporterParams;
using AL::usdmaya::fileio::ImporterParams;
using AL::usdmaya::fileio::translators::DgNodeTranslator;

namespace {

static MObject m_node = MObject::kNullObj;
static MObject m_nodeB = MObject::kNullObj;

// Array size chosen to be deliberately annoying!
//
//   511 % 16 == 15, which translates to:
//
//     1 x AVX256 op (8)    +
//     1 x AVX128 op (4)    +
//     3 x FPU ops   (3)
//
// This should ensure ALL code paths are executed that handle the remainders left at the end of an
// array, even if we were to add an AVX512 codepath in the future, this should still handle the
// messy end conditions It would probably be a good idea to one day also add tests for multiples of:
//
// 512
// 510
// 509
// 508
//
// That would handle all permutations of end conditions within the AVX2 code paths
#define SIZE 511

void setUp()
{
    AL_OUTPUT_TEST_NAME("test_translators_DgNodeTranslator");
    // A bit nasty. cppunit runs a setup and teardown between each unit test.
    // I actually want to run ALL of these tests in order, so that I can test the Maya <--> USD
    // conversions.
    if (m_node == MObject::kNullObj) {
        MFnDependencyNode fn;
        m_node = fn.create("transform");
        m_nodeB = fn.create("transform");
    }
}

MPlug findPlug(const char* name)
{
    MStatus           status;
    MFnDependencyNode fn(m_node, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);
    MPlug plug = fn.findPlug(name, true, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);
    return plug;
}

MObject findAttribute(const char* name) { return findPlug(name).attribute(); }

/// \brief  A set of bit flags you can apply to an attribute
enum AttributeFlags
{
    kCached = NodeHelper::kCached,
    kReadable = NodeHelper::kReadable,
    kWritable = NodeHelper::kWritable,
    kStorable = NodeHelper::kStorable,
    kAffectsAppearance = NodeHelper::kAffectsAppearance,
    kKeyable = NodeHelper::kKeyable,
    kConnectable = NodeHelper::kConnectable,
    kArray = NodeHelper::kArray,
    kColour = NodeHelper::kColour,
    kHidden = NodeHelper::kHidden,
    kInternal = NodeHelper::kInternal,
    kAffectsWorldSpace = NodeHelper::kAffectsWorldSpace,
    kUsesArrayDataBuilder = NodeHelper::kUsesArrayDataBuilder,
    kDontAddToNode = NodeHelper::kDontAddToNode,
    kDynamic = NodeHelper::kDynamic
};

} // namespace

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the alUsdNodeHelper.
//----------------------------------------------------------------------------------------------------------------------

// static MStatus getBoolArray(MObject node, MObject attr, bool* values, const size_t count);
// template<template<typename> class bool_container>
// static MStatus getBoolArray(MObject node, MObject attr, bool_container<bool>& values);
// static MStatus setBoolArray(MObject node, MObject attr, const bool* const values, size_t count);
// template<template<typename> class bool_container>
// static MStatus setBoolArray(MObject node, MObject attr, const bool_container<bool>& values);
TEST(translators_DgNodeTranslator, bool_array)
{
    setUp();
    bool*             orig = new bool[SIZE];
    bool*             result = new bool[SIZE];
    std::vector<bool> container, container2;
    VtArray<bool>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(bool) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randBool();
        container[i] = randBool();
        vcontainer[i] = randBool();
    }
    const char* const longName = "longBoolArrayName";
    const char* const shortName = "lBan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const bool defaultValue = true;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addBoolAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setBoolArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getBoolArray(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setBoolArray(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getBoolArray(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdBoolArray(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdBoolArray(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getInt8Array(MObject node, MObject attr, uint8_t* values, size_t count);
// static MStatus setInt8Array(MObject node, MObject attr, const uint8_t* values, size_t count);
TEST(translators_DgNodeTranslator, int8_array)
{
    setUp();
    int8_t*             orig = new int8_t[SIZE];
    int8_t*             result = new int8_t[SIZE];
    std::vector<int8_t> container, container2;
    VtArray<int8_t>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(int8_t) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randInt8();
        container[i] = randInt8();
        vcontainer[i] = randInt8();
    }
    const char* const longName = "longInt8ArrayName";
    const char* const shortName = "li8an";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const int8_t defaultValue = 99;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt8Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt8Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt8Array(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt8Array(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt8Array(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdInt8Array(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdInt8Array(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

TEST(translators_DgNodeTranslator, int16_array)
{
    setUp();
    int16_t*             orig = new int16_t[SIZE];
    int16_t*             result = new int16_t[SIZE];
    std::vector<int16_t> container, container2;
    VtArray<int16_t>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(int16_t) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randInt16();
        container[i] = randInt16();
        vcontainer[i] = randInt16();
    }
    const char* const longName = "longInt16ArrayName";
    const char* const shortName = "li16an";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const int16_t defaultValue = 99;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt16Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt16Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt16Array(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt16Array(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt16Array(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdInt16Array(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdInt16Array(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getInt32Array(MObject node, MObject attr, int32_t* values, size_t count);
// static MStatus setInt32Array(MObject node, MObject attr, const int32_t* values, size_t count);
TEST(translators_DgNodeTranslator, int32_array)
{
    setUp();
    int32_t*             orig = new int32_t[SIZE];
    int32_t*             result = new int32_t[SIZE];
    std::vector<int32_t> container, container2;
    VtArray<int32_t>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(int32_t) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randInt32();
        container[i] = randInt32();
        vcontainer[i] = randInt32();
    }
    const char* const longName = "longInt32ArrayName";
    const char* const shortName = "li32an";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const int32_t defaultValue = 99;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt32Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt32Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt32Array(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt32Array(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt32Array(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdInt32Array(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdInt32Array(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getInt64Array(MObject node, MObject attr, int64_t* values, size_t count);
// static MStatus setInt64Array(MObject node, MObject attr, const int64_t* values, size_t count);
TEST(translators_DgNodeTranslator, int64_array)
{
    setUp();
    int64_t*             orig = new int64_t[SIZE];
    int64_t*             result = new int64_t[SIZE];
    std::vector<int64_t> container, container2;
    VtArray<int64_t>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(int64_t) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randInt64();
        container[i] = randInt64();
        vcontainer[i] = randInt64();
    }
    const char* const longName = "longInt64ArrayName";
    const char* const shortName = "li64an";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const int64_t defaultValue = 99;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt64Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt64Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt64Array(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setInt64Array(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getInt64Array(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdInt64Array(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdInt64Array(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getHalfArray(MObject node, MObject attr, half* values, size_t count);
// static MStatus setHalfArray(MObject node, MObject attr, const half* values, size_t count);
TEST(translators_DgNodeTranslator, half_array)
{
    setUp();
    GfHalf*             orig = new GfHalf[SIZE];
    GfHalf*             result = new GfHalf[SIZE];
    std::vector<GfHalf> container, container2;
    VtArray<GfHalf>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(GfHalf) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randFloat();
        container[i] = randFloat();
        vcontainer[i] = randFloat();
    }
    const char* const longName = "longHalfArrayName";
    const char* const shortName = "lhan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const GfHalf defaultValue = 0.1f;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addFloatAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setHalfArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getHalfArray(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setHalfArray(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getHalfArray(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdHalfArray(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdHalfArray(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getFloatArray(MObject node, MObject attr, float* values, size_t count);
// static MStatus setFloatArray(MObject node, MObject attr, const float* values, size_t count);
TEST(translators_DgNodeTranslator, float_array)
{
    setUp();
    float*             orig = new float[SIZE];
    float*             result = new float[SIZE];
    std::vector<float> container, container2;
    VtArray<float>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(float) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randFloat();
        container[i] = randFloat();
        vcontainer[i] = randFloat();
    }
    const char* const longName = "longFloatArrayName";
    const char* const shortName = "lfan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const float defaultValue = 99.091f;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addFloatAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setFloatArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getFloatArray(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setFloatArray(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getFloatArray(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdFloatArray(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdFloatArray(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getDoubleArray(MObject node, MObject attr, double* values, size_t count);
// static MStatus setDoubleArray(MObject node, MObject attr, const double* values, size_t count);
TEST(translators_DgNodeTranslator, double_array)
{
    setUp();
    double*             orig = new double[SIZE];
    double*             result = new double[SIZE];
    std::vector<double> container, container2;
    VtArray<double>     vcontainer, vcontainer2;
    container.resize(SIZE);
    vcontainer.resize(SIZE);
    std::memset(result, 0, sizeof(double) * SIZE);
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = randDouble();
        container[i] = randDouble();
        vcontainer[i] = randDouble();
    }
    const char* const longName = "longDoubleArrayName";
    const char* const shortName = "ldan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const double defaultValue = 99.09;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addDoubleAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setDoubleArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getDoubleArray(m_node, findAttribute(longName), result, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setDoubleArray(m_node, findAttribute(longName), container));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getDoubleArray(m_node, findAttribute(longName), container2));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setUsdDoubleArray(m_node, findAttribute(longName), vcontainer));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getUsdDoubleArray(m_node, findAttribute(longName), vcontainer2));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
        EXPECT_EQ(container[i], container2[i]);
        EXPECT_EQ(vcontainer[i], vcontainer2[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getVec2Array(MObject node, MObject attr, half* values, size_t count);
// static MStatus getVec2Array(MObject node, MObject attr, float* values, size_t count);
// static MStatus getVec2Array(MObject node, MObject attr, double* values, size_t count);
// static MStatus getVec2Array(MObject node, MObject attr, int32_t* values, size_t count);
// static MStatus setVec2Array(MObject node, MObject attr, const half* values, size_t count);
// static MStatus setVec2Array(MObject node, MObject attr, const float* values, size_t count);
// static MStatus setVec2Array(MObject node, MObject attr, const double* values, size_t count);
// static MStatus setVec2Array(MObject node, MObject attr, const int32_t* values, size_t count);
#define VEC_SIZE 2
TEST(translators_DgNodeTranslator, vec2h_array)
{
    setUp();
    GfHalf* orig = new GfHalf[SIZE * VEC_SIZE];
    GfHalf* result = new GfHalf[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(GfHalf) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec2hArrayName";
    const char* const shortName = "lv2han";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec2Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec2Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec2i_array)
{
    setUp();
    int32_t* orig = new int32_t[SIZE * VEC_SIZE];
    int32_t* result = new int32_t[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(int32_t) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randInt32();
    }
    const char* const longName = "longVec2iArrayName";
    const char* const shortName = "lv2ian";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2iAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec2Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec2Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec2f_array)
{
    setUp();
    float* orig = new float[SIZE * VEC_SIZE];
    float* result = new float[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(float) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec2fArrayName";
    const char* const shortName = "lv2fan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec2Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec2Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec2d_array)
{
    setUp();
    double* orig = new double[SIZE * VEC_SIZE];
    double* result = new double[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(double) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec2dArrayName";
    const char* const shortName = "lv2dan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec2Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec2Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
#undef VEC_SIZE

// static MStatus getVec3Array(MObject node, MObject attr, half* values, size_t count);
// static MStatus getVec3Array(MObject node, MObject attr, float* values, size_t count);
// static MStatus getVec3Array(MObject node, MObject attr, double* values, size_t count);
// static MStatus getVec3Array(MObject node, MObject attr, int32_t* values, size_t count);
// static MStatus setVec3Array(MObject node, MObject attr, const half* values, size_t count);
// static MStatus setVec3Array(MObject node, MObject attr, const float* values, size_t count);
// static MStatus setVec3Array(MObject node, MObject attr, const double* values, size_t count);
// static MStatus setVec3Array(MObject node, MObject attr, const int32_t* values, size_t count);
#define VEC_SIZE 3
TEST(translators_DgNodeTranslator, vec3h_array)
{
    setUp();
    GfHalf* orig = new GfHalf[SIZE * VEC_SIZE];
    GfHalf* result = new GfHalf[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(GfHalf) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec3hArrayName";
    const char* const shortName = "lv3han";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec3Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec3Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec3i_array)
{
    setUp();
    int32_t* orig = new int32_t[SIZE * VEC_SIZE];
    int32_t* result = new int32_t[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(int32_t) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randInt32();
    }
    const char* const longName = "longVec3iArrayName";
    const char* const shortName = "lv3ian";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3iAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec3Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec3Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec3f_array)
{
    setUp();
    float* orig = new float[SIZE * VEC_SIZE];
    float* result = new float[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(float) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec3fArrayName";
    const char* const shortName = "lv3fan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec3Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec3Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec3d_array)
{
    setUp();
    double* orig = new double[SIZE * VEC_SIZE];
    double* result = new double[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(double) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec3dArrayName";
    const char* const shortName = "lv3dan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec3Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec3Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
#undef VEC_SIZE

// static MStatus getVec4Array(MObject node, MObject attr, half* values, size_t count);
// static MStatus getVec4Array(MObject node, MObject attr, float* values, size_t count);
// static MStatus getVec4Array(MObject node, MObject attr, double* values, size_t count);
// static MStatus getVec4Array(MObject node, MObject attr, int32_t* values, size_t count);
// static MStatus setVec4Array(MObject node, MObject attr, const half* values, size_t count);
// static MStatus setVec4Array(MObject node, MObject attr, const float* values, size_t count);
// static MStatus setVec4Array(MObject node, MObject attr, const double* values, size_t count);
// static MStatus setVec4Array(MObject node, MObject attr, const int32_t* values, size_t count);
#define VEC_SIZE 4
TEST(translators_DgNodeTranslator, vec4h_array)
{
    setUp();
    GfHalf* orig = new GfHalf[SIZE * VEC_SIZE];
    GfHalf* result = new GfHalf[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(GfHalf) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec4hArrayName";
    const char* const shortName = "lv4han";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec4Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec4Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec4i_array)
{
    setUp();
    int32_t* orig = new int32_t[SIZE * VEC_SIZE];
    int32_t* result = new int32_t[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(int32_t) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randInt32();
    }
    const char* const longName = "longVec4iArrayName";
    const char* const shortName = "lv4ian";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4iAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec4Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec4Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec4f_array)
{
    setUp();
    float* orig = new float[SIZE * VEC_SIZE];
    float* result = new float[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(float) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec4fArrayName";
    const char* const shortName = "lv4fan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec4Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec4Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, vec4d_array)
{
    setUp();
    double* orig = new double[SIZE * VEC_SIZE];
    double* result = new double[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(double) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longVec4dArrayName";
    const char* const shortName = "lv4dan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setVec4Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getVec4Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
#undef VEC_SIZE

// static MStatus getQuatArray(MObject node, MObject attr, half* values, size_t count);
// static MStatus getQuatArray(MObject node, MObject attr, float* values, size_t count);
// static MStatus getQuatArray(MObject node, MObject attr, double* values, size_t count);
// static MStatus setQuatArray(MObject node, MObject attr, const half* values, size_t count);
// static MStatus setQuatArray(MObject node, MObject attr, const float* values, size_t count);
// static MStatus setQuatArray(MObject node, MObject attr, const double* values, size_t count);
#define VEC_SIZE 4
TEST(translators_DgNodeTranslator, quath_array)
{
    setUp();
    GfHalf* orig = new GfHalf[SIZE * VEC_SIZE];
    GfHalf* result = new GfHalf[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(GfHalf) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longQuathArrayName";
    const char* const shortName = "lqhan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setQuatArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getQuatArray(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, quatf_array)
{
    setUp();
    float* orig = new float[SIZE * VEC_SIZE];
    float* result = new float[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(float) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longQuatfArrayName";
    const char* const shortName = "lqfan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setQuatArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getQuatArray(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, quatd_array)
{
    setUp();
    double* orig = new double[SIZE * VEC_SIZE];
    double* result = new double[SIZE * VEC_SIZE];
    std::memset(result, 0, sizeof(double) * SIZE * VEC_SIZE);
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longQuatdArrayName";
    const char* const shortName = "lqdan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setQuatArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getQuatArray(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * VEC_SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}
#undef VEC_SIZE

// static MStatus getMatrix2x2Array(MObject node, MObject attr, float* values, size_t count);
// static MStatus getMatrix2x2Array(MObject node, MObject attr, double* values, size_t count);
// static MStatus setMatrix2x2Array(MObject node, MObject attr, const float* values, size_t count);
// static MStatus setMatrix2x2Array(MObject node, MObject attr, const double* values, size_t count);
TEST(translators_DgNodeTranslator, matrix2x2f_array)
{
    setUp();
    float* orig = new float[SIZE * 4];
    float* result = new float[SIZE * 4];
    ;
    for (int i = 0; i < SIZE * 4; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longMatrix2x2fArrayName";
    const char* const shortName = "lM22fan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const float defaultValue[2][2] = {
        { randFloat(), randFloat() },
        { randFloat(), randFloat() },
    };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix2x2Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix2x2Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix2x2Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * 4; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, matrix2x2d_array)
{
    setUp();
    double* orig = new double[SIZE * 4];
    double* result = new double[SIZE * 4];
    ;
    for (int i = 0; i < SIZE * 4; ++i) {
        orig[i] = randDouble();
    }
    const char* const longName = "longMatrix2x2dArrayName";
    const char* const shortName = "lM22dan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const float defaultValue[2][2] = {
        { randFloat(), randFloat() },
        { randFloat(), randFloat() },
    };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix2x2Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix2x2Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix2x2Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * 4; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getMatrix3x3Array(MObject node, MObject attr, float* values, size_t count);
// static MStatus getMatrix3x3Array(MObject node, MObject attr, double* values, size_t count);
// static MStatus setMatrix3x3Array(MObject node, MObject attr, const float* values, size_t count);
// static MStatus setMatrix3x3Array(MObject node, MObject attr, const double* values, size_t count);
TEST(translators_DgNodeTranslator, matrix3x3f_array)
{
    setUp();
    float* orig = new float[SIZE * 9];
    float* result = new float[SIZE * 9];
    ;
    for (int i = 0; i < SIZE * 9; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longMatrix3x3fArrayName";
    const char* const shortName = "lM33fan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const float defaultValue[3][3] = { { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() } };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix3x3Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix3x3Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix3x3Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * 9; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, matrix3x3d_array)
{
    setUp();
    double* orig = new double[SIZE * 9];
    double* result = new double[SIZE * 9];
    ;
    for (int i = 0; i < SIZE * 9; ++i) {
        orig[i] = randDouble();
    }
    const char* const longName = "longMatrix3x3dArrayName";
    const char* const shortName = "lM33dan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const float defaultValue[3][3] = { { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() } };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix3x3Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix3x3Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix3x3Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * 9; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getMatrix4x4Array(MObject node, MObject attr, float* values, size_t count);
// static MStatus getMatrix4x4Array(MObject node, MObject attr, double* values, size_t count);
// static MStatus setMatrix4x4Array(MObject node, MObject attr, const float* values, size_t count);
// static MStatus setMatrix4x4Array(MObject node, MObject attr, const double* values, size_t count);
TEST(translators_DgNodeTranslator, matrix4x4f_array)
{
    setUp();
    float* orig = new float[SIZE * 16];
    float* result = new float[SIZE * 16];
    ;
    for (int i = 0; i < SIZE * 16; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longMatrix4x4fArrayName";
    const char* const shortName = "lM44fan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    MMatrix defaultValue;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrixAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix4x4Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix4x4Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * 9; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}
TEST(translators_DgNodeTranslator, matrix4x4d_array)
{
    setUp();
    double* orig = new double[SIZE * 16];
    double* result = new double[SIZE * 16];
    ;
    for (int i = 0; i < SIZE * 16; ++i) {
        orig[i] = randFloat();
    }
    const char* const longName = "longMatrix4x4dArrayName";
    const char* const shortName = "lM44dan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    MMatrix defaultValue;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrixAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix4x4Array(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix4x4Array(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE * 9; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getStringArray(MObject node, MObject attr, std::string* values, size_t count);
// static MStatus setStringArray(MObject node, MObject attr, const std::string* values, size_t
// count);
TEST(translators_DgNodeTranslator, string_array)
{
    setUp();
    const char* const text[] = { "mouse", "cat", "dog", "rabbit", "dinosaur" };
    std::string*      orig = new std::string[SIZE];
    std::string*      result = new std::string[SIZE];
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = text[randInt32() % 5];
    }
    const char* const longName = "longStringArrayName";
    const char* const shortName = "lsan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addStringAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setStringArray(m_node, findAttribute(longName), orig, SIZE));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getStringArray(m_node, findAttribute(longName), result, SIZE));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_EQ(orig[i], result[i]);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getTimeArray(MObject node, MObject attr, float* values, size_t count, MTime::Unit
// unit); static MStatus setTimeArray(MObject node, MObject attr, const float* values, size_t count,
// MTime::Unit unit);
TEST(translators_DgNodeTranslator, time_array)
{
    setUp();
    float* orig = new float[SIZE];
    float* result = new float[SIZE];
    ;
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = MTime(randFloat(), MTime::kSeconds).value();
    }
    const char* const longName = "longTimeArrayName";
    const char* const shortName = "ltan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const MTime defaultValue(99.091f, MTime::kSeconds);
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addTimeAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setTimeArray(
            m_node, findAttribute(longName), orig, SIZE, MTime::kSeconds));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getTimeArray(
            m_node, findAttribute(longName), result, SIZE, MTime::kSeconds));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getAngleArray(MObject node, MObject attr, float* values, size_t count,
// MAngle::Unit unit); static MStatus setAngleArray(MObject node, MObject attr, const float* values,
// size_t count, MAngle::Unit unit);
TEST(translators_DgNodeTranslator, angle_array)
{
    setUp();
    float* orig = new float[SIZE];
    float* result = new float[SIZE];
    ;
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = MAngle(randFloat(), MAngle::kRadians).value();
    }
    const char* const longName = "longAngleArrayName";
    const char* const shortName = "laan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const MAngle defaultValue(99.091f, MAngle::kRadians);
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addAngleAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setAngleArray(
            m_node, findAttribute(longName), orig, SIZE, MAngle::kRadians));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getAngleArray(
            m_node, findAttribute(longName), result, SIZE, MAngle::kRadians));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getDistanceArray(MObject node, MObject attr, float* values, size_t count,
// MDistance::Unit unit); static MStatus setDistanceArray(MObject node, MObject attr, const float*
// values, size_t count, MDistance::Unit unit);
TEST(translators_DgNodeTranslator, distance_array)
{
    setUp();
    float* orig = new float[SIZE];
    float* result = new float[SIZE];
    ;
    for (int i = 0; i < SIZE; ++i) {
        orig[i] = MDistance(randFloat(), MDistance::kInches).value();
    }
    const char* const longName = "longDistanceArrayName";
    const char* const shortName = "lDan";
    const uint32_t    flags
        = kCached | kReadable | kWritable | kStorable | kArray | kUsesArrayDataBuilder;
    const MDistance   defaultValue(99.091f, MDistance::kInches);
    MFnDependencyNode fn(m_node);
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addDistanceAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setDistanceArray(
            m_node, findAttribute(longName), orig, SIZE, MDistance::kInches));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getDistanceArray(
            m_node, findAttribute(longName), result, SIZE, MDistance::kInches));
    for (int i = 0; i < SIZE; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getHalf(MObject node, MObject attr, float& value);
// static MStatus setHalf(MObject node, MObject attr, float value);
TEST(translators_DgNodeTranslator, half_test)
{
    setUp();
    GfHalf            orig = randFloat(), result, defaultValue = 0.1f;
    const char* const longName = "longHalfName";
    const char* const shortName = "lhn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addFloatAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setHalf(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getHalf(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}

// static MStatus getFloat(MObject node, MObject attr, float& value);
// static MStatus setFloat(MObject node, MObject attr, float value);
TEST(translators_DgNodeTranslator, float_test)
{
    setUp();
    float             orig = randFloat(), result, defaultValue = 0.1f;
    const char* const longName = "longFloatName";
    const char* const shortName = "lfn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addFloatAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setFloat(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getFloat(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}

// static MStatus getDouble(MObject node, MObject attr, double& value);
// static MStatus setDouble(MObject node, MObject attr, double value);
TEST(translators_DgNodeTranslator, double_test)
{
    setUp();
    double            orig = randFloat(), result, defaultValue = 0.1f;
    const char* const longName = "longDoubleName";
    const char* const shortName = "ldn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addDoubleAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setDouble(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getDouble(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}

// static MStatus getTime(MObject node, MObject attr, MTime& value);
// static MStatus setTime(MObject node, MObject attr, MTime value);
TEST(translators_DgNodeTranslator, time_test)
{
    setUp();
    MTime             orig(123.9f, MTime::kSeconds), result;
    MTime             defaultValue(1123.9f, MTime::kSeconds);
    const char* const longName = "longTimeName";
    const char* const shortName = "lTn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addTimeAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setTime(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getTime(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig.as(MTime::kSeconds), result.as(MTime::kSeconds));
}

// static MStatus getDistance(MObject node, MObject attr, MDistance& value);
// static MStatus setDistance(MObject node, MObject attr, MDistance value);
TEST(translators_DgNodeTranslator, distance_test)
{
    setUp();
    MDistance         orig(123.9f, MDistance::kFeet), result;
    MDistance         defaultValue(1123.9f, MDistance::kFeet);
    const char* const longName = "longDistanceName";
    const char* const shortName = "lDn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addDistanceAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setDistance(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getDistance(m_node, findAttribute(longName), result));
    EXPECT_NEAR(orig.as(MDistance::kFeet), result.as(MDistance::kFeet), 1e-5f);
}

// static MStatus getAngle(MObject node, MObject attr, MAngle& value);
// static MStatus setAngle(MObject node, MObject attr, MAngle value);
TEST(translators_DgNodeTranslator, angle_test)
{
    setUp();
    MAngle            orig(123.9f, MAngle::kRadians), result;
    MAngle            defaultValue(1123.9f, MAngle::kRadians);
    const char* const longName = "longAngleName";
    const char* const shortName = "lAn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addAngleAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setAngle(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getAngle(m_node, findAttribute(longName), result));
    EXPECT_NEAR(orig.as(MAngle::kDegrees), result.as(MAngle::kDegrees), 1e-5f);
}

// static MStatus getBool(MObject node, MObject attr, bool& value);
// static MStatus setBool(MObject node, MObject attr, bool value);
TEST(translators_DgNodeTranslator, bool_test)
{
    setUp();
    bool              orig = true, result, defaultValue = false;
    const char* const longName = "longBoolName";
    const char* const shortName = "lbn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addBoolAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setBool(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getBool(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}

// static MStatus getInt16(MObject node, MObject attr, int16_t& value);
// static MStatus getInt32(MObject node, MObject attr, int32_t& value);
// static MStatus setInt(MObject node, MObject attr, int32_t value);
TEST(translators_DgNodeTranslator, int8_test)
{
    setUp();
    int8_t            orig = randInt8(), result, defaultValue = randInt8();
    const char* const longName = "longInt8Name";
    const char* const shortName = "li8n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt8Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setInt8(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getInt8(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}
TEST(translators_DgNodeTranslator, int16_test)
{
    setUp();
    int16_t           orig = randInt16(), result, defaultValue = randInt16();
    const char* const longName = "longInt16Name";
    const char* const shortName = "li16n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt16Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setInt16(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getInt16(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}
TEST(translators_DgNodeTranslator, int32_test)
{
    setUp();
    int32_t           orig = randInt32(), result, defaultValue = randInt32();
    const char* const longName = "longInt32Name";
    const char* const shortName = "li32n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt32Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setInt32(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getInt32(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}

// static MStatus getInt64(MObject node, MObject attr, int64_t& value);
// static MStatus setInt64(MObject node, MObject attr, int64_t value);
TEST(translators_DgNodeTranslator, int64_test)
{
    setUp();
    int64_t           orig = randInt64(), result, defaultValue = randInt64();
    const char* const longName = "longInt64Name";
    const char* const shortName = "li64n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addInt64Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setInt64(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getInt64(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}

// static MStatus getMatrix2x2(MObject node, MObject attr, float* values);
// static MStatus setMatrix2x2(MObject node, MObject attr, const float* values);
TEST(translators_DgNodeTranslator, matrix2x2f_test)
{
    setUp();
    float* orig = new float[4];
    float* result = new float[4];
    for (int i = 0; i < 4; ++i) {
        orig[i] = randDouble();
    }
    const char* const longName = "longMatrix2x2fName";
    const char* const shortName = "lM22fn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float defaultValue[2][2] = { { randFloat(), randFloat() }, { randFloat(), randFloat() } };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix2x2Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix2x2(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix2x2(m_node, findAttribute(longName), result));
    for (int i = 0; i < 4; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getMatrix3x3(MObject node, MObject attr, float* values);
// static MStatus setMatrix3x3(MObject node, MObject attr, const float* values);
TEST(translators_DgNodeTranslator, matrix3x3f_test)
{
    setUp();
    float* orig = new float[9];
    float* result = new float[9];
    for (int i = 0; i < 9; ++i) {
        orig[i] = randDouble();
    }
    const char* const longName = "longMatrix3x3fName";
    const char* const shortName = "lM33fn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float       defaultValue[3][3] = { { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() } };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix3x3Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix3x3(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix3x3(m_node, findAttribute(longName), result));
    for (int i = 0; i < 9; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getMatrix4x4(MObject node, MObject attr, float* values);
// static MStatus setMatrix4x4(MObject node, MObject attr, const float* values);
TEST(translators_DgNodeTranslator, matrix4x4f_test)
{
    setUp();
    MFloatMatrix orig, result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            orig[i][j] = randFloat();
        }
    }
    const char* const longName = "longMatrix4x4fName";
    const char* const shortName = "lM44fn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const MMatrix     defaultValue;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrixAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix4x4(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix4x4(m_node, findAttribute(longName), result));
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(orig[i][j], result[i][j], 1e-5f);
        }
    }
}

// static MStatus getMatrix2x2(MObject node, MObject attr, double* values);
// static MStatus setMatrix2x2(MObject node, MObject attr, const double* values);
TEST(translators_DgNodeTranslator, matrix2x2d_test)
{
    setUp();
    double* orig = new double[4];
    double* result = new double[4];
    for (int i = 0; i < 4; ++i) {
        orig[i] = randDouble();
    }
    const char* const longName = "longMatrix2x2dName";
    const char* const shortName = "lM22dn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float defaultValue[2][2] = { { randFloat(), randFloat() }, { randFloat(), randFloat() } };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix2x2Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix2x2(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix2x2(m_node, findAttribute(longName), result));
    for (int i = 0; i < 4; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getMatrix3x3(MObject node, MObject attr, double* values);
// static MStatus setMatrix3x3(MObject node, MObject attr, const double* values);
TEST(translators_DgNodeTranslator, matrix3x3d_test)
{
    setUp();
    double* orig = new double[9];
    double* result = new double[9];
    for (int i = 0; i < 9; ++i) {
        orig[i] = randDouble();
    }
    const char* const longName = "longMatrix3x3dName";
    const char* const shortName = "lM33dn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float       defaultValue[3][3] = { { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() },
                                       { randFloat(), randFloat(), randFloat() } };
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrix3x3Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix3x3(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix3x3(m_node, findAttribute(longName), result));
    for (int i = 0; i < 9; ++i) {
        EXPECT_NEAR(orig[i], result[i], 1e-5f);
    }
    delete[] orig;
    delete[] result;
}

// static MStatus getMatrix4x4(MObject node, MObject attr, double* values);
// static MStatus setMatrix4x4(MObject node, MObject attr, const double* values);
TEST(translators_DgNodeTranslator, matrix4x4d_test)
{
    setUp();
    MMatrix orig, result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            orig[i][j] = randFloat();
        }
    }
    const char* const longName = "longMatrix4x4dName";
    const char* const shortName = "lM44dn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const MMatrix     defaultValue;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        NodeHelper::addMatrixAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::setMatrix4x4(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getMatrix4x4(m_node, findAttribute(longName), result));
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(orig[i][j], result[i][j], 1e-5f);
        }
    }
}

// static MStatus getString(MObject node, MObject attr, std::string& str);
// static MStatus setString(MObject node, MObject attr, const char* str);
// static MStatus setString(MObject node, MObject attr, const std::string& str);
TEST(translators_DgNodeTranslator, string_test)
{
    setUp();
    std::string       orig = "dinosaur", result;
    const char* const longName = "longStringName";
    const char* const shortName = "lsn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addStringAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setString(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        DgNodeTranslator::getString(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig, result);
}

// static MStatus getVec2(MObject node, MObject attr, int32_t* xyz);
// static MStatus getVec2(MObject node, MObject attr, float* xyx);
// static MStatus getVec2(MObject node, MObject attr, double* xyz);
// static MStatus getVec2(MObject node, MObject attr, half* xyz);
// static MStatus setVec2(MObject node, MObject attr, const int32_t* xy);
// static MStatus setVec2(MObject node, MObject attr, const float* xy);
// static MStatus setVec2(MObject node, MObject attr, const double* xy);
// static MStatus setVec2(MObject node, MObject attr, const half* xy);
TEST(translators_DgNodeTranslator, vec2i_test)
{
    setUp();
    int32_t           orig[2] = { randInt32(), randInt32() }, result[2];
    const char* const longName = "longVec2iName";
    const char* const shortName = "lv2in";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2iAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec2(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec2(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
}
TEST(translators_DgNodeTranslator, vec2h_test)
{
    setUp();
    GfHalf            orig[2] = { randFloat(), randFloat() }, result[2];
    const char* const longName = "longVec2hName";
    const char* const shortName = "lv2hn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec2(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec2(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
}
TEST(translators_DgNodeTranslator, vec2f_test)
{
    setUp();
    float             orig[2] = { randFloat(), randFloat() }, result[2];
    const char* const longName = "longVec2fName";
    const char* const shortName = "lv2fn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec2(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec2(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
}
TEST(translators_DgNodeTranslator, vec2d_test)
{
    setUp();
    double            orig[2] = { randDouble(), randDouble() }, result[2];
    const char* const longName = "longVec2dName";
    const char* const shortName = "lv2dn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec2dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec2(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec2(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
}

// static MStatus setVec3(MObject node, MObject attr, float x, float y, float z);
// static MStatus setVec3(MObject node, MObject attr, double x, double y, double z);
// static MStatus setVec3(MObject node, MObject attr, MAngle x, MAngle y, MAngle z);
// static MStatus setVec3(MObject node, MObject attr, const int32_t* xyz);
// static MStatus setVec3(MObject node, MObject attr, const float* xyx);
// static MStatus setVec3(MObject node, MObject attr, const double* xyz);
// static MStatus setVec3(MObject node, MObject attr, const half* xyz);
// static MStatus getVec3(MObject node, MObject attr, int32_t* xyz);
// static MStatus getVec3(MObject node, MObject attr, float* xyx);
// static MStatus getVec3(MObject node, MObject attr, double* xyz);
// static MStatus getVec3(MObject node, MObject attr, half* xyz);
TEST(translators_DgNodeTranslator, vec3i_test)
{
    setUp();
    int32_t           orig[3] = { randInt32(), randInt32(), randInt32() }, result[3];
    const char* const longName = "longVec3iName";
    const char* const shortName = "lvvin";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3iAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec3(m_node, findAttribute(shortName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec3(m_node, findAttribute(shortName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
}
TEST(translators_DgNodeTranslator, vec3h_test)
{
    setUp();
    GfHalf            orig[3] = { randFloat(), randFloat(), randFloat() }, result[3];
    const char* const longName = "longVec3hName";
    const char* const shortName = "lv3hn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec3(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec3(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
}
TEST(translators_DgNodeTranslator, vec3f_test)
{
    setUp();
    float             orig[3] = { randFloat(), randFloat(), randFloat() }, result[3];
    const char* const longName = "longVec3fName";
    const char* const shortName = "lv3fn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec3(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec3(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
}
TEST(translators_DgNodeTranslator, vec3d_test)
{
    setUp();
    double            orig[3] = { randDouble(), randDouble(), randDouble() }, result[3];
    const char* const longName = "longVec3dName";
    const char* const shortName = "lv3dn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec3dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec3(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec3(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
}

// static MStatus setVec4(MObject node, MObject attr, float x, float y, float z);
// static MStatus getVec4(MObject node, MObject attr, int32_t* xyz);
// static MStatus getVec4(MObject node, MObject attr, float* xyx);
// static MStatus getVec4(MObject node, MObject attr, double* xyz);
// static MStatus getVec4(MObject node, MObject attr, half* xyz);
// static MStatus setVec4(MObject node, MObject attr, const int32_t* xyzw);
// static MStatus setVec4(MObject node, MObject attr, const float* xyzw);
// static MStatus setVec4(MObject node, MObject attr, const double* xyzw);
// static MStatus setVec4(MObject node, MObject attr, const half* xyzw);
TEST(translators_DgNodeTranslator, vec4i_test)
{
    setUp();
    int32_t           orig[4] = { randInt32(), randInt32(), randInt32(), randInt32() }, result[4];
    const char* const longName = "longVec4iName";
    const char* const shortName = "lv4in";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4iAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec4(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec4(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
    EXPECT_EQ(orig[3], result[3]);
}
TEST(translators_DgNodeTranslator, vec4h_test)
{
    setUp();
    GfHalf            orig[4] = { randFloat(), randFloat(), randFloat(), randFloat() }, result[4];
    const char* const longName = "longVec4hName";
    const char* const shortName = "lv4hn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec4(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec4(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
    EXPECT_EQ(orig[3], result[3]);
}
TEST(translators_DgNodeTranslator, vec4f_test)
{
    setUp();
    float             orig[4] = { randFloat(), randFloat(), randFloat(), randFloat() }, result[4];
    const char* const longName = "longVec4fName";
    const char* const shortName = "lv4fn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec4(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec4(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
    EXPECT_EQ(orig[3], result[3]);
}
TEST(translators_DgNodeTranslator, vec4d_test)
{
    setUp();
    double orig[4] = { randDouble(), randDouble(), randDouble(), randDouble() }, result[4];
    const char* const longName = "longVec4dName";
    const char* const shortName = "lv4dn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setVec4(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getVec4(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
    EXPECT_EQ(orig[3], result[3]);
}

// static MStatus getQuat(MObject node, MObject attr, float* xyx);
// static MStatus getQuat(MObject node, MObject attr, double* xyx);
// static MStatus getQuat(MObject node, MObject attr, half* xyx);
// static MStatus setQuat(MObject node, MObject attr, const float* xyzw);
// static MStatus setQuat(MObject node, MObject attr, const double* xyzw);
// static MStatus setQuat(MObject node, MObject attr, const half* xyzw);
TEST(translators_DgNodeTranslator, quath_test)
{
    setUp();
    GfHalf            orig[4] = { randFloat(), randFloat(), randFloat(), randFloat() }, result[4];
    const char* const longName = "longQuathName";
    const char* const shortName = "lqhn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4hAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setQuat(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getQuat(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
    EXPECT_EQ(orig[3], result[3]);
}
TEST(translators_DgNodeTranslator, quatf_test)
{
    setUp();
    float             orig[4] = { randFloat(), randFloat(), randFloat(), randFloat() }, result[4];
    const char* const longName = "longQuatfName";
    const char* const shortName = "lqfn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4fAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setQuat(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getQuat(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
    EXPECT_EQ(orig[3], result[3]);
}
TEST(translators_DgNodeTranslator, quatd_test)
{
    setUp();
    double orig[4] = { randDouble(), randDouble(), randDouble(), randDouble() }, result[4];
    const char* const longName = "longQuatdName";
    const char* const shortName = "lqdn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(MStatus(MS::kSuccess), NodeHelper::addVec4dAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::setQuat(m_node, findAttribute(longName), orig));
    EXPECT_EQ(
        MStatus(MS::kSuccess), DgNodeTranslator::getQuat(m_node, findAttribute(longName), result));
    EXPECT_EQ(orig[0], result[0]);
    EXPECT_EQ(orig[1], result[1]);
    EXPECT_EQ(orig[2], result[2]);
    EXPECT_EQ(orig[3], result[3]);
}

// !!! THIS TEST MUST BE EXECUTED LAST !!!
TEST(translators_DgNodeTranslator, dynamicAttributesTest)
{
    setUp();
    /// uint16_t attributes are not supported in USD, remove the tests for those types.
    /// \todo Add support for int16 so we can serialise 16bit integer values into USD as 32bit types
    if (1) {
        MFnDependencyNode fnA(m_node);
        MPlug             aplug = fnA.findPlug("longInt16ArrayName");
        MObject           aattr = aplug.attribute();
        MPlug             bplug = fnA.findPlug("longInt16Name");
        MObject           battr = bplug.attribute();
        fnA.removeAttribute(aattr);
        fnA.removeAttribute(battr);
    }

    // generate a prim for testing
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform   xform = UsdGeomXform::Define(stage, SdfPath("/hello"));
    UsdPrim        prim = xform.GetPrim();

    // at this point, we should have created all possible dynamic attributes via the unit tests
    // above. That being the case, can we should now be able to copy all of those values onto a
    // UsdPrim.
    EXPECT_EQ(MStatus(MS::kSuccess), DgNodeTranslator::copyDynamicAttributes(m_node, prim));

    // On the assumption that worked, can we copy all of them from the prim
    // onto a new node?
    ImporterParams   importParams;
    DgNodeTranslator translator;
    MObject m_nodeOut = translator.createNode(prim, MObject::kNullObj, "transform", importParams);
    EXPECT_TRUE(m_nodeOut != MObject::kNullObj);

    // validate that the attribute counts match
    MFnDependencyNode fnA(m_node);
    MFnDependencyNode fnB(m_nodeOut);
    EXPECT_EQ(fnA.attributeCount(), fnB.attributeCount());

    for (uint32_t i = 0; i < fnA.attributeCount(); ++i) {
        MPlug plugA(m_node, fnA.attribute(i));
        if (plugA.isDynamic()) {
            // we only want to process high level attributes, e.g. translate, and not it's kids
            // translateX, translateY, translateZ
            if (plugA.isChild()) {
                continue;
            }
            MStatus status;

            // can we find the attribute on the second node?
            MPlug plugB = fnB.findPlug(
                plugA.partialName(false, true, true, true, true, true), true, &status);
            EXPECT_EQ(MStatus(MS::kSuccess), status);

            // compare the plug values to be ensure they match
            comparePlugs(plugA, plugB, true);
        }
    }

    if (m_node != MObject::kNullObj) {
        MGlobal::deleteNode(m_node);
        MGlobal::deleteNode(m_nodeB);
    }
}
