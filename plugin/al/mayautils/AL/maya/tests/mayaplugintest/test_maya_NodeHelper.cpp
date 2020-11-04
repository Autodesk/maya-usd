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
//#include "test_usdmaya.h"

#include "AL/maya/utils/NodeHelper.h"

#include <maya/MAngle.h>
#include <maya/MDGModifier.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatVector.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MLibrary.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>
#include <maya/MVector.h>

#include <gtest/gtest.h>

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the AL::maya::utils::NodeHelper.
//----------------------------------------------------------------------------------------------------------------------

namespace {
MObject m_node = MObject::kNullObj;
void    setUp()
{
    MFnDependencyNode fn;
    m_node = fn.create("transform");
}

void tearDown()
{
    MDGModifier mod;
    mod.deleteNode(m_node);
    mod.doIt();
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

/// \brief  A set of bit flags you can apply to an attribute
enum AttributeFlags
{
    kCached = AL::maya::utils::NodeHelper::kCached,
    kReadable = AL::maya::utils::NodeHelper::kReadable,
    kWritable = AL::maya::utils::NodeHelper::kWritable,
    kStorable = AL::maya::utils::NodeHelper::kStorable,
    kAffectsAppearance = AL::maya::utils::NodeHelper::kAffectsAppearance,
    kKeyable = AL::maya::utils::NodeHelper::kKeyable,
    kConnectable = AL::maya::utils::NodeHelper::kConnectable,
    kArray = AL::maya::utils::NodeHelper::kArray,
    kColour = AL::maya::utils::NodeHelper::kColour,
    kHidden = AL::maya::utils::NodeHelper::kHidden,
    kInternal = AL::maya::utils::NodeHelper::kInternal,
    kAffectsWorldSpace = AL::maya::utils::NodeHelper::kAffectsWorldSpace,
    kUsesArrayDataBuilder = AL::maya::utils::NodeHelper::kUsesArrayDataBuilder,
    kDontAddToNode = AL::maya::utils::NodeHelper::kDontAddToNode,
    kDynamic = AL::maya::utils::NodeHelper::kDynamic
};

void checkAttributeFlags(MPlug attr, const uint32_t flags)
{
    MFnAttribute fn(attr.attribute());
    EXPECT_EQ((flags & kCached) != 0, fn.isCached());
    EXPECT_EQ((flags & kReadable) != 0, fn.isReadable());
    EXPECT_EQ((flags & kStorable) != 0, fn.isStorable());
    EXPECT_EQ((flags & kWritable) != 0, fn.isWritable());
    EXPECT_EQ((flags & kAffectsAppearance) != 0, fn.affectsAppearance());
    EXPECT_EQ((flags & kKeyable) != 0, fn.isKeyable());
    EXPECT_EQ((flags & kConnectable) != 0, fn.isConnectable());
    EXPECT_EQ((flags & kArray) != 0, fn.isArray());
    EXPECT_EQ((flags & kColour) != 0, fn.isUsedAsColor());
    EXPECT_EQ((flags & kHidden) != 0, fn.isHidden());
    EXPECT_EQ((flags & kInternal) != 0, fn.internal());
    EXPECT_EQ((flags & kAffectsWorldSpace) != 0, fn.isAffectsWorldSpace());
    EXPECT_EQ((flags & kUsesArrayDataBuilder) != 0, fn.usesArrayDataBuilder());
}

} // namespace

TEST(maya_NodeHelper, addStringAttr)
{
    setUp();
    const char* const longName = "longStringName";
    const char* const shortName = "lsn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addStringAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    tearDown();
}

TEST(maya_NodeHelper, addFilePathAttr)
{
    setUp();
    const char* const longName = "longFileName";
    const char* const shortName = "lfn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addFilePathAttr(
            m_node,
            longName,
            shortName,
            flags,
            AL::maya::utils::NodeHelper::kSave,
            "All files (*.*) (*.*)"));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    tearDown();
}

TEST(maya_NodeHelper, addInt8Attr)
{
    setUp();
    const char* const longName = "longInt8Name";
    const char* const shortName = "li8n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const int8_t      defaultValue = 19;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addInt8Attr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_EQ(char(defaultValue), findPlug(longName).asChar());

    // This test will always fail, so don't run it. It turns out that if you create a char attribute
    // type in Maya, it will always set a min value of 0, and a max value of 255 (which is pretty
    // dumb given that char is a signed type)
    if (0) {
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_FALSE(fn.hasMin());
        EXPECT_FALSE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
        double minMax[2];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        EXPECT_EQ(4.0, minMax[0]);
        EXPECT_EQ(40.0, minMax[1]);
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40, 5, 39);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_TRUE(fn.hasSoftMax());
        EXPECT_TRUE(fn.hasSoftMin());
        double minMax[4];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        fn.getSoftMin(minMax[2]);
        fn.getSoftMax(minMax[3]);
        EXPECT_NEAR(4.0, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0, minMax[1], 1e-5f);
        EXPECT_NEAR(5, minMax[2], 1e-5f);
        EXPECT_NEAR(39.0, minMax[3], 1e-5f);
    }
    tearDown();
}

TEST(maya_NodeHelper, addInt16Attr)
{
    setUp();
    const char* const longName = "longInt16Name";
    const char* const shortName = "li16n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const int         defaultValue = 67;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addInt16Attr(
            m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_EQ(defaultValue, findPlug(longName).asInt());
    {
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_FALSE(fn.hasMin());
        EXPECT_FALSE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
        double minMax[2];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        EXPECT_EQ(4.0, minMax[0]);
        EXPECT_EQ(40.0, minMax[1]);
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40, 5, 39);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_TRUE(fn.hasSoftMax());
        EXPECT_TRUE(fn.hasSoftMin());
        double minMax[4];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        fn.getSoftMin(minMax[2]);
        fn.getSoftMax(minMax[3]);
        EXPECT_NEAR(4.0, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0, minMax[1], 1e-5f);
        EXPECT_NEAR(5, minMax[2], 1e-5f);
        EXPECT_NEAR(39.0, minMax[3], 1e-5f);
    }
    tearDown();
}

TEST(maya_NodeHelper, addInt32Attr)
{
    setUp();
    const char* const longName = "longInt32Name";
    const char* const shortName = "li32n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const int         defaultValue = 23;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addInt32Attr(
            m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_EQ(defaultValue, findPlug(longName).asInt());
    {
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_FALSE(fn.hasMin());
        EXPECT_FALSE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
        double minMax[2];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        EXPECT_EQ(4.0, minMax[0]);
        EXPECT_EQ(40.0, minMax[1]);
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40, 5, 39);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_TRUE(fn.hasSoftMax());
        EXPECT_TRUE(fn.hasSoftMin());
        double minMax[4];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        fn.getSoftMin(minMax[2]);
        fn.getSoftMax(minMax[3]);
        EXPECT_NEAR(4.0, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0, minMax[1], 1e-5f);
        EXPECT_NEAR(5, minMax[2], 1e-5f);
        EXPECT_NEAR(39.0, minMax[3], 1e-5f);
    }
    tearDown();
}

TEST(maya_NodeHelper, addInt64Attr)
{
    setUp();
    const char* const longName = "longInt64Name";
    const char* const shortName = "li64n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const int64_t     defaultValue = 23;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addInt64Attr(
            m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_EQ(defaultValue, findPlug(longName).asInt64());
    {
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_FALSE(fn.hasMin());
        EXPECT_FALSE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
        double minMax[2];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        EXPECT_EQ(4.0, minMax[0]);
        EXPECT_EQ(40.0, minMax[1]);
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4, 40, 5, 39);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_TRUE(fn.hasSoftMax());
        EXPECT_TRUE(fn.hasSoftMin());
        double minMax[4];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        fn.getSoftMin(minMax[2]);
        fn.getSoftMax(minMax[3]);
        EXPECT_NEAR(4.0, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0, minMax[1], 1e-5f);
        EXPECT_NEAR(5, minMax[2], 1e-5f);
        EXPECT_NEAR(39.0, minMax[3], 1e-5f);
    }
    tearDown();
}

TEST(maya_NodeHelper, addFloatAttr)
{
    setUp();
    const char* const longName = "longFloatName";
    const char* const shortName = "lFn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float       defaultValue = 23.1f;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addFloatAttr(
            m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValue, findPlug(longName).asFloat(), 1e-5f);
    {
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_FALSE(fn.hasMin());
        EXPECT_FALSE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4.0f, 40.0f);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
        double minMax[2];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        EXPECT_NEAR(4.0f, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0f, minMax[1], 1e-5f);
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(
            findPlug(longName).attribute(), 4.0f, 40.0f, 4.1f, 39.0f);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_TRUE(fn.hasSoftMax());
        EXPECT_TRUE(fn.hasSoftMin());
        double minMax[4];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        fn.getSoftMin(minMax[2]);
        fn.getSoftMax(minMax[3]);
        EXPECT_NEAR(4.0f, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0f, minMax[1], 1e-5f);
        EXPECT_NEAR(4.1f, minMax[2], 1e-5f);
        EXPECT_NEAR(39.0f, minMax[3], 1e-5f);
    }
    tearDown();
}

TEST(maya_NodeHelper, addDoubleAttr)
{
    setUp();
    const char* const longName = "longDoubleName";
    const char* const shortName = "lDn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const double      defaultValue = 23.2f;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addDoubleAttr(
            m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValue, findPlug(longName).asDouble(), 1e-5f);
    {
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_FALSE(fn.hasMin());
        EXPECT_FALSE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(findPlug(longName).attribute(), 4.0f, 40.0f);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_FALSE(fn.hasSoftMax());
        EXPECT_FALSE(fn.hasSoftMin());
        double minMax[2];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        EXPECT_NEAR(4.0f, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0f, minMax[1], 1e-5f);
    }
    {
        AL::maya::utils::NodeHelper::setMinMax(
            findPlug(longName).attribute(), 4.0f, 40.0f, 4.1f, 39.0f);
        MFnNumericAttribute fn(findPlug(longName).attribute());
        EXPECT_TRUE(fn.hasMin());
        EXPECT_TRUE(fn.hasMax());
        EXPECT_TRUE(fn.hasSoftMax());
        EXPECT_TRUE(fn.hasSoftMin());
        double minMax[4];
        fn.getMin(minMax[0]);
        fn.getMax(minMax[1]);
        fn.getSoftMin(minMax[2]);
        fn.getSoftMax(minMax[3]);
        EXPECT_NEAR(4.0f, minMax[0], 1e-5f);
        EXPECT_NEAR(40.0f, minMax[1], 1e-5f);
        EXPECT_NEAR(4.1f, minMax[2], 1e-5f);
        EXPECT_NEAR(39.0f, minMax[3], 1e-5f);
    }
    tearDown();
}

TEST(maya_NodeHelper, addTimeAttr)
{
    setUp();
    const char* const longName = "longTimeName";
    const char* const shortName = "lTn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const MTime       defaultValue = 23.3;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addTimeAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValue.value(), findPlug(longName).asMTime().value(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addBoolAttr)
{
    setUp();
    const char* const longName = "longBoolName";
    const char* const shortName = "lBn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const bool        defaultValue = true;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addBoolAttr(m_node, longName, shortName, defaultValue, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_EQ(defaultValue, findPlug(longName).asBool());
    tearDown();
}

TEST(maya_NodeHelper, addFloat3Attr)
{
    setUp();
    const char* const longName = "longFloat3Name";
    const char* const shortName = "lf3n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float       defaultValueX = 31.1f;
    const float       defaultValueY = 31.2f;
    const float       defaultValueZ = 31.3f;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addFloat3Attr(
            m_node, longName, shortName, defaultValueX, defaultValueY, defaultValueZ, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValueX, findPlug(longName).child(0).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueY, findPlug(longName).child(1).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueZ, findPlug(longName).child(2).asFloat(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addAngle3Attr)
{
    setUp();
    const char* const longName = "longAngle3Name";
    const char* const shortName = "la3n";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float       defaultValueX = 33.1f;
    const float       defaultValueY = 33.2f;
    const float       defaultValueZ = 33.3f;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addAngle3Attr(
            m_node, longName, shortName, defaultValueX, defaultValueY, defaultValueZ, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValueX, findPlug(longName).child(0).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueY, findPlug(longName).child(1).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueZ, findPlug(longName).child(2).asFloat(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addPointAttr)
{
    setUp();
    const char* const longName = "longPointName";
    const char* const shortName = "lpn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const double      defaultValueX = 41.1;
    const double      defaultValueY = 41.2;
    const double      defaultValueZ = 41.3;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addPointAttr(
            m_node,
            longName,
            shortName,
            MPoint(defaultValueX, defaultValueY, defaultValueZ),
            flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValueX, findPlug(longName).child(0).asDouble(), 1e-5f);
    EXPECT_NEAR(defaultValueY, findPlug(longName).child(1).asDouble(), 1e-5f);
    EXPECT_NEAR(defaultValueZ, findPlug(longName).child(2).asDouble(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addVectorAttr)
{
    setUp();
    const char* const longName = "longVectorName";
    const char* const shortName = "lVn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const double      defaultValueX = 51.1;
    const double      defaultValueY = 51.2;
    const double      defaultValueZ = 51.3;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addVectorAttr(
            m_node,
            longName,
            shortName,
            MVector(defaultValueX, defaultValueY, defaultValueZ),
            flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValueX, findPlug(longName).child(0).asDouble(), 1e-5f);
    EXPECT_NEAR(defaultValueY, findPlug(longName).child(1).asDouble(), 1e-5f);
    EXPECT_NEAR(defaultValueZ, findPlug(longName).child(2).asDouble(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addFloatPointAttr)
{
    setUp();
    const char* const longName = "longFPointName";
    const char* const shortName = "lFpn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float       defaultValueX = 61.1;
    const float       defaultValueY = 61.2;
    const float       defaultValueZ = 61.3;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addFloatPointAttr(
            m_node,
            longName,
            shortName,
            MFloatPoint(defaultValueX, defaultValueY, defaultValueZ),
            flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValueX, findPlug(longName).child(0).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueY, findPlug(longName).child(1).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueZ, findPlug(longName).child(2).asFloat(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addFloatVectorAttr)
{
    setUp();
    const char* const longName = "longFVectorName";
    const char* const shortName = "lFVn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    const float       defaultValueX = 71.1;
    const float       defaultValueY = 71.2;
    const float       defaultValueZ = 71.3;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addFloatVectorAttr(
            m_node,
            longName,
            shortName,
            MFloatVector(defaultValueX, defaultValueY, defaultValueZ),
            flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValueX, findPlug(longName).child(0).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueY, findPlug(longName).child(1).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueZ, findPlug(longName).child(2).asFloat(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addColourAttr)
{
    setUp();
    const char* const longName = "longColourName";
    const char* const shortName = "lCn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable | kColour;
    const float       defaultValueX = 0.441;
    const float       defaultValueY = 0.442;
    const float       defaultValueZ = 0.443;
    const float       defaultValueW = 0.444;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addColourAttr(
            m_node,
            longName,
            shortName,
            MColor(defaultValueX, defaultValueY, defaultValueZ, defaultValueW),
            flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    EXPECT_NEAR(defaultValueX, findPlug(longName).child(0).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueY, findPlug(longName).child(1).asFloat(), 1e-5f);
    EXPECT_NEAR(defaultValueZ, findPlug(longName).child(2).asFloat(), 1e-5f);
    tearDown();
}

TEST(maya_NodeHelper, addMatrixAttr)
{
    setUp();
    double            D[4][4] = { { 1.0, 2.0, 3.0, 4.0f },
                       { 11.0, 12.0, 13.0, 14.0f },
                       { 21.0, 22.0, 23.0, 24.0f },
                       { 31.0, 32.0, 33.0, 34.0f } };
    MMatrix           MM(D);
    const char* const longName = "longMatrixName";
    const char* const shortName = "lMn";
    const uint32_t    flags = kCached | kReadable | kWritable | kStorable;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addMatrixAttr(m_node, longName, shortName, MM, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    MObject attrData;
    findPlug(longName).getValue(attrData);
    MFnMatrixData fnData(attrData);
    EXPECT_EQ(MM, fnData.matrix());
    tearDown();
}

TEST(maya_NodeHelper, addDataAttr)
{
    setUp();
    const uint32_t flags = kCached | kReadable | kWritable | kHidden | kStorable;
    {
        const char* const                longName = "longDataName1";
        const char* const                shortName = "lDDn1";
        MFnAttribute::DisconnectBehavior behaviour = MFnAttribute::kNothing;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addDataAttr(
                m_node, longName, shortName, MFnData::kVectorArray, flags, behaviour));
        EXPECT_EQ(findPlug(longName), findPlug(shortName));
        checkAttributeFlags(findPlug(longName), flags);
        MFnAttribute fn(findPlug(longName).attribute());
        EXPECT_EQ(behaviour, fn.disconnectBehavior());
        EXPECT_EQ(true, fn.accepts(MFnData::kVectorArray));
    }
    {
        const char* const                longName = "longDataName2";
        const char* const                shortName = "lDDn2";
        MFnAttribute::DisconnectBehavior behaviour = MFnAttribute::kDelete;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addDataAttr(
                m_node, longName, shortName, MFnData::kMesh, flags, behaviour));
        EXPECT_EQ(findPlug(longName), findPlug(shortName));
        checkAttributeFlags(findPlug(longName), flags);
        MFnAttribute fn(findPlug(longName).attribute());
        EXPECT_EQ(behaviour, fn.disconnectBehavior());
        EXPECT_EQ(true, fn.accepts(MFnData::kMesh));
    }
    {
        const char* const                longName = "longDataName3";
        const char* const                shortName = "lDDn3";
        MFnAttribute::DisconnectBehavior behaviour = MFnAttribute::kReset;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addDataAttr(
                m_node, longName, shortName, MFnData::kLattice, flags, behaviour));
        EXPECT_EQ(findPlug(longName), findPlug(shortName));
        checkAttributeFlags(findPlug(longName), flags);
        MFnAttribute fn(findPlug(longName).attribute());
        EXPECT_EQ(behaviour, fn.disconnectBehavior());
        EXPECT_EQ(true, fn.accepts(MFnData::kLattice));
    }
    tearDown();
}

TEST(maya_NodeHelper, addMessageAttr)
{
    setUp();
    const char* const longName = "longMessageName";
    const char* const shortName = "lMNn";
    const uint32_t    flags = kCached | kReadable | kWritable | kHidden;
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        AL::maya::utils::NodeHelper::addMessageAttr(m_node, longName, shortName, flags));
    EXPECT_EQ(findPlug(longName), findPlug(shortName));
    checkAttributeFlags(findPlug(longName), flags);
    tearDown();
}

TEST(maya_NodeHelper, testAttributeFlags)
{
    setUp();
    {
        const char* const longName = "testAttrFlags1";
        const char* const shortName = "taf1";
        const uint32_t    flags = kReadable | kWritable | kCached;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags2";
        const char* const shortName = "taf2";
        const uint32_t    flags = kReadable | kWritable | kStorable;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags3";
        const char* const shortName = "taf3";
        const uint32_t    flags = kReadable | kWritable | kAffectsAppearance;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags4";
        const char* const shortName = "taf4";
        const uint32_t    flags = kReadable | kWritable | kKeyable;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags5";
        const char* const shortName = "taf5";
        const uint32_t    flags = kReadable | kWritable | kConnectable;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
/// \todo Investigate.
///       So when I'm *not* setting kUsesArrayDataBuilder, kUsesArrayDataBuilder gets set anyway
///       whenever you use kArray? WAT?
#if 0
  {
    const char* const longName = "testAttrFlags6";
    const char* const shortName = "taf6";
    const uint32_t flags = kReadable | kWritable | kArray;
    EXPECT_EQ(MStatus(MS::kSuccess), AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
    checkAttributeFlags(findPlug(longName), flags);
  }
#endif
    {
        const char* const longName = "testAttrFlags7";
        const char* const shortName = "taf7";
        const uint32_t    flags = kReadable | kWritable | kColour;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags8";
        const char* const shortName = "taf8";
        const uint32_t    flags = kReadable | kWritable | kHidden;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags9";
        const char* const shortName = "taf9";
        const uint32_t    flags = kReadable | kWritable | kInternal;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags10";
        const char* const shortName = "taf10";
        const uint32_t    flags = kReadable | kWritable | kAffectsWorldSpace;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    {
        const char* const longName = "testAttrFlags11";
        const char* const shortName = "taf11";
        const uint32_t    flags = kReadable | kWritable | kUsesArrayDataBuilder | kArray;
        EXPECT_EQ(
            MStatus(MS::kSuccess),
            AL::maya::utils::NodeHelper::addInt32Attr(m_node, longName, shortName, 0, flags));
        checkAttributeFlags(findPlug(longName), flags);
    }
    tearDown();
}
