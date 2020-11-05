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
#include "testHelpers.h"

#include <pxr/base/arch/fileSystem.h>
#include <pxr/base/tf/pathUtils.h>

#include <maya/MFileIO.h>
#include <maya/MFloatMatrix.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>

#include <algorithm>
#include <cstring>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace maya {
namespace test {

//----------------------------------------------------------------------------------------------------------------------
const char* buildTempPath(const char* const filename)
{
    static std::string _temp_subdir;
    if (_temp_subdir.empty()) {
        _temp_subdir = ArchMakeTmpSubdir(TfRealPath(ArchGetTmpDir()), "AL_USDMaya");

        if (_temp_subdir.empty()) {
            return nullptr;
        }

        _temp_subdir += AL_PATH_CHAR;
    }

    std::string full_path = _temp_subdir + filename;
    std::replace(full_path.begin(), full_path.end(), '\\', '/');

    static char temp_file[512];
    std::strcpy(temp_file, full_path.data());

    return temp_file;
}

//----------------------------------------------------------------------------------------------------------------------
void compareTempPaths(std::string pathA, std::string pathB)
{
    std::replace(pathA.begin(), pathA.end(), '\\', '/');
    std::replace(pathB.begin(), pathB.end(), '\\', '/');

    EXPECT_EQ(pathA, pathB);
}

//----------------------------------------------------------------------------------------------------------------------
void comparePlugs(const MPlug& plugA, const MPlug& plugB, bool usdTesting)
{
    SCOPED_TRACE(MString("plugA: ") + plugA.name() + " - plugB: " + plugB.name());
    EXPECT_EQ(plugA.isArray(), plugB.isArray());
    EXPECT_EQ(plugA.isElement(), plugB.isElement());
    EXPECT_EQ(plugA.isCompound(), plugB.isCompound());
    EXPECT_EQ(plugA.isChild(), plugB.isChild());
    EXPECT_EQ(
        plugA.partialName(false, true, true, true, true, true),
        plugB.partialName(false, true, true, true, true, true));

    /// I need to special case the testing of the Time, Angle, and Distance attribute types. These
    /// are converted to doubles in USD, so if plugA is one of those types, plugB should be a
    /// double. \TODO I would have thought that there would be a way to flag the units used in the
    /// USD attributes?
    if (usdTesting && (plugA.attribute().apiType() != plugB.attribute().apiType())) {
        if (plugA.attribute().apiType() == MFn::kTimeAttribute
            || plugA.attribute().apiType() == MFn::kDoubleAngleAttribute
            || plugA.attribute().apiType() == MFn::kDoubleLinearAttribute) {
            if (plugA.isArray()) {
                EXPECT_EQ(plugA.numElements(), plugB.numElements());
                for (uint32_t i = 0; i < plugA.numElements(); ++i) {
                    EXPECT_NEAR(
                        plugA.elementByLogicalIndex(i).asDouble(),
                        plugB.elementByLogicalIndex(i).asDouble(),
                        1e-5f);
                }
            } else {
                EXPECT_NEAR(plugA.asDouble(), plugB.asDouble(), 1e-5f);
            }
        }
        return;
    }

    // make sure the unit types match
    EXPECT_EQ(plugA.attribute().apiType(), plugB.attribute().apiType());
    if (plugB.isArray()) {
        // for arrays, just make sure the array sizes match, and then compare each of the element
        // plugs
        EXPECT_EQ(plugA.numElements(), plugB.numElements());
        for (uint32_t i = 0; i < plugA.numElements(); ++i) {
            comparePlugs(plugA.elementByLogicalIndex(i), plugB.elementByLogicalIndex(i));
        }
    } else if (plugB.isCompound()) {
        // for compound attrs, make sure child counts match, and then compare each of the child
        // plugs
        EXPECT_EQ(plugA.numChildren(), plugB.numChildren());
        for (uint32_t i = 0; i < plugA.numChildren(); ++i) {
            comparePlugs(plugA.child(i), plugB.child(i));
        }
    } else {
        switch (plugA.attribute().apiType()) {
        case MFn::kTypedAttribute: {
            MFnTypedAttribute fnA(plugA.attribute());
            MFnTypedAttribute fnB(plugB.attribute());
            EXPECT_EQ(fnA.attrType(), fnB.attrType());
            switch (fnA.attrType()) {
            case MFnData::kString: EXPECT_EQ(plugA.asString(), plugB.asString()); break;

            default:
                std::cout << ("Unknown typed attribute type \"") << plugA.name().asChar() << "\" "
                          << fnA.attrType() << std::endl;
                break;
            }
        } break;
        case MFn::kNumericAttribute: {
            // when we get here, the attributes represent a single value
            // make sure the types match, and compare the values to make sure they
            // are the same.
            MFnNumericAttribute unAttrA(plugA.attribute());
            MFnNumericAttribute unAttrB(plugB.attribute());
            EXPECT_EQ(unAttrA.unitType(), unAttrB.unitType());

            switch (unAttrA.unitType()) {
            case MFnNumericData::kBoolean: EXPECT_EQ(plugA.asBool(), plugB.asBool()); break;

            case MFnNumericData::kByte: EXPECT_EQ(plugA.asChar(), plugB.asChar()); break;

            case MFnNumericData::kChar: EXPECT_EQ(plugA.asChar(), plugB.asChar()); break;

            case MFnNumericData::kShort: EXPECT_EQ(plugA.asShort(), plugB.asShort()); break;

            case MFnNumericData::k2Short:
                EXPECT_EQ(plugA.child(0).asShort(), plugB.child(0).asShort());
                EXPECT_EQ(plugA.child(1).asShort(), plugB.child(1).asShort());
                break;

            case MFnNumericData::k3Short:
                EXPECT_EQ(plugA.child(0).asShort(), plugB.child(0).asShort());
                EXPECT_EQ(plugA.child(1).asShort(), plugB.child(1).asShort());
                EXPECT_EQ(plugA.child(2).asShort(), plugB.child(2).asShort());
                break;

            case MFnNumericData::kLong: EXPECT_EQ(plugA.asInt(), plugB.asInt()); break;

            case MFnNumericData::kInt64: EXPECT_EQ(plugA.asInt64(), plugB.asInt64()); break;

            case MFnNumericData::k2Long:
                EXPECT_EQ(plugA.child(0).asInt(), plugB.child(0).asInt());
                EXPECT_EQ(plugA.child(1).asInt(), plugB.child(1).asInt());
                break;

            case MFnNumericData::k3Long:
                EXPECT_EQ(plugA.child(0).asInt(), plugB.child(0).asInt());
                EXPECT_EQ(plugA.child(1).asInt(), plugB.child(1).asInt());
                EXPECT_EQ(plugA.child(2).asInt(), plugB.child(2).asInt());
                break;

            case MFnNumericData::kFloat:
                EXPECT_NEAR(plugA.asFloat(), plugB.asFloat(), 1e-5f);
                break;

            case MFnNumericData::k2Float:
                EXPECT_NEAR(plugA.child(0).asFloat(), plugB.child(0).asFloat(), 1e-5f);
                EXPECT_NEAR(plugA.child(1).asFloat(), plugB.child(1).asFloat(), 1e-5f);
                break;

            case MFnNumericData::k3Float:
                EXPECT_NEAR(plugA.child(0).asFloat(), plugB.child(0).asFloat(), 1e-5f);
                EXPECT_NEAR(plugA.child(1).asFloat(), plugB.child(1).asFloat(), 1e-5f);
                EXPECT_NEAR(plugA.child(2).asFloat(), plugB.child(2).asFloat(), 1e-5f);
                break;

            case MFnNumericData::kDouble:
                EXPECT_NEAR(plugA.asDouble(), plugB.asDouble(), 1e-5f);
                break;

            case MFnNumericData::k2Double:
                EXPECT_NEAR(plugA.child(0).asDouble(), plugB.child(0).asDouble(), 1e-5f);
                EXPECT_NEAR(plugA.child(1).asDouble(), plugB.child(1).asDouble(), 1e-5f);
                break;

            case MFnNumericData::k3Double:
                EXPECT_NEAR(plugA.child(0).asDouble(), plugB.child(0).asDouble(), 1e-5f);
                EXPECT_NEAR(plugA.child(1).asDouble(), plugB.child(1).asDouble(), 1e-5f);
                EXPECT_NEAR(plugA.child(2).asDouble(), plugB.child(2).asDouble(), 1e-5f);
                break;

            case MFnNumericData::k4Double:
                EXPECT_NEAR(plugA.child(0).asDouble(), plugB.child(0).asDouble(), 1e-5f);
                EXPECT_NEAR(plugA.child(1).asDouble(), plugB.child(1).asDouble(), 1e-5f);
                EXPECT_NEAR(plugA.child(2).asDouble(), plugB.child(2).asDouble(), 1e-5f);
                EXPECT_NEAR(plugA.child(3).asDouble(), plugB.child(3).asDouble(), 1e-5f);
                break;

            default:
                std::cout << ("Unknown numeric attribute type \"") << plugA.name().asChar() << "\" "
                          << std::endl;
                break;
            }
        } break;

        case MFn::kUnitAttribute: {
            MFnUnitAttribute unAttrA(plugA.attribute());
            MFnUnitAttribute unAttrB(plugB.attribute());
            EXPECT_EQ(unAttrA.unitType(), unAttrB.unitType());
            switch (unAttrA.unitType()) {
            case MFnUnitAttribute::kAngle:
                EXPECT_NEAR(
                    plugA.asMAngle().as(MAngle::kRadians),
                    plugB.asMAngle().as(MAngle::kRadians),
                    1e-5f);
                break;

            case MFnUnitAttribute::kDistance:
                EXPECT_NEAR(
                    plugA.asMDistance().as(MDistance::kFeet),
                    plugB.asMDistance().as(MDistance::kFeet),
                    1e-5f);
                break;

            case MFnUnitAttribute::kTime:
                EXPECT_NEAR(
                    plugA.asMTime().as(MTime::kSeconds),
                    plugB.asMTime().as(MTime::kSeconds),
                    1e-5f);
                break;

            default:
                std::cout << ("Unknown unit attribute type \"") << plugA.name().asChar() << "\" "
                          << std::endl;
                break;
            }
        } break;

        case MFn::kGenericAttribute:
        case MFn::kMessageAttribute: {
        } break;

        case MFn::kMatrixAttribute:
        case MFn::kFloatMatrixAttribute: {
            MFnMatrixData fnA(plugA.asMObject());
            MFnMatrixData fnB(plugB.asMObject());
            MMatrix       vA = fnA.matrix();
            MMatrix       vB = fnB.matrix();
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    EXPECT_NEAR(vA[i][j], vB[i][j], 1e-5f);
                }
            }
        } break;

        case MFn::kEnumAttribute: EXPECT_EQ(plugA.asShort(), plugB.asShort()); break;

        case MFn::kTimeAttribute:
            EXPECT_NEAR(
                plugA.asMTime().as(MTime::kSeconds), plugB.asMTime().as(MTime::kSeconds), 1e-5f);
            break;

        case MFn::kFloatAngleAttribute:
        case MFn::kDoubleAngleAttribute:
            EXPECT_NEAR(
                plugA.asMAngle().as(MAngle::kRadians),
                plugB.asMAngle().as(MAngle::kRadians),
                1e-5f);
            break;

        case MFn::kFloatLinearAttribute:
        case MFn::kDoubleLinearAttribute:
            EXPECT_NEAR(
                plugA.asMDistance().as(MDistance::kFeet),
                plugB.asMDistance().as(MDistance::kFeet),
                1e-5f);
            break;

        default:
            std::cout << ("Unknown attribute type \"") << plugA.name().asChar() << "\" "
                      << plugA.attribute().apiTypeStr() << std::endl;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void compareNodes(
    const MObject& nodeA,
    const MObject& nodeB,
    bool           includeDefaultAttrs,
    bool           includeDynamicAttrs,
    bool           usdTesting)
{
    MFnDependencyNode fnA(nodeA);
    MFnDependencyNode fnB(nodeB);
    for (uint32_t i = 0; i < fnA.attributeCount(); ++i) {
        MPlug plugA(nodeA, fnA.attribute(i)), plugB;

        // we only want to process high level attributes, e.g. translate, and not it's kids
        // translateX, translateY, translateZ
        if (plugA.isChild()) {
            continue;
        }

        if (plugA.isDynamic()) {
            if (!includeDynamicAttrs)
                continue;
        } else {
            if (!includeDefaultAttrs)
                continue;
        }

        // can we find the attribute on the second node?
        MStatus status;
        plugB = fnB.findPlug(plugA.partialName(false, true, true, true, true, true), true, &status);
        EXPECT_EQ(MStatus(MS::kSuccess), status);

        // compare the plug values to be ensure they match
        comparePlugs(plugA, plugB, usdTesting);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void compareNodes(
    const MObject&    nodeA,
    const MObject&    nodeB,
    const char* const attributes[],
    uint32_t          attributeCount,
    bool              usdTesting)
{
    MFnDependencyNode fnA(nodeA);
    MFnDependencyNode fnB(nodeB);
    for (uint32_t i = 0; i < attributeCount; ++i) {
        MPlug plugA = fnA.findPlug(attributes[i]);
        MPlug plugB = fnB.findPlug(attributes[i]);

        // compare the plug values to be ensure they match
        comparePlugs(plugA, plugB, usdTesting);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void randomPlug(MPlug plug)
{
    // make sure the unit types match
    if (plug.isArray()) {
        if (plug.attribute().apiType() == MFn::kMatrixAttribute
            || plug.attribute().apiType() == MFn::kFloatMatrixAttribute) {
            for (int i = 0; i < 511; ++i) {
                char tempStr[2048];
                snprintf(
                    tempStr,
                    2048,
                    "setAttr \"%s[%d]\" -type \"matrix\" %lf %lf %lf %lf  %lf %lf %lf %lf  %lf %lf "
                    "%lf %lf  %lf %lf %lf %lf;",
                    plug.name().asChar(),
                    i,
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble(),
                    randDouble());
                MGlobal::executeCommand(tempStr);
            }
        } else {
            // for arrays, just make sure the array sizes match, and then compare each of the
            // element plugs
            plug.setNumElements(511);
            for (uint32_t i = 0; i < plug.numElements(); ++i) {
                randomPlug(plug.elementByLogicalIndex(i));
            }
        }
    } else if (plug.isCompound()) {
        // for compound attrs, make sure child counts match, and then compare each of the child
        // plugs
        for (uint32_t i = 0; i < plug.numChildren(); ++i) {
            randomPlug(plug.child(i));
        }
    } else {
        switch (plug.attribute().apiType()) {
        case MFn::kTypedAttribute: {
            MFnTypedAttribute fn(plug.attribute());
            switch (fn.attrType()) {
            case MFnData::kString: randomString(plug); break;

            default: std::cout << ("Unknown typed attribute type") << std::endl; break;
            }
        } break;

        case MFn::kNumericAttribute: {
            // when we get here, the attributes represent a single value
            // make sure the types match, and compare the values to make sure they
            // are the same.
            MFnNumericAttribute unAttr(plug.attribute());
            switch (unAttr.unitType()) {
            case MFnNumericData::kBoolean: randomBool(plug); break;

            case MFnNumericData::kByte: randomInt8(plug); break;

            case MFnNumericData::kChar: randomInt8(plug); break;

            case MFnNumericData::kShort: randomInt16(plug); break;

            case MFnNumericData::k2Short:
                randomInt16(plug.child(0));
                randomInt16(plug.child(1));
                break;

            case MFnNumericData::k3Short:
                randomInt16(plug.child(0));
                randomInt16(plug.child(1));
                randomInt16(plug.child(2));
                break;

            case MFnNumericData::kLong: randomInt32(plug); break;

            case MFnNumericData::kInt64: randomInt64(plug); break;

            case MFnNumericData::k2Long:
                randomInt32(plug.child(0));
                randomInt32(plug.child(1));
                break;

            case MFnNumericData::k3Long:
                randomInt32(plug.child(0));
                randomInt32(plug.child(1));
                randomInt32(plug.child(2));
                break;

            case MFnNumericData::kFloat: randomFloat(plug); break;

            case MFnNumericData::k2Float:
                randomFloat(plug.child(0));
                randomFloat(plug.child(1));
                break;

            case MFnNumericData::k3Float:
                randomFloat(plug.child(0));
                randomFloat(plug.child(1));
                randomFloat(plug.child(2));
                break;

            case MFnNumericData::kDouble: randomDouble(plug); break;

            case MFnNumericData::k2Double:
                randomDouble(plug.child(0));
                randomDouble(plug.child(1));
                break;

            case MFnNumericData::k3Double:
                randomDouble(plug.child(0));
                randomDouble(plug.child(1));
                randomDouble(plug.child(2));
                break;

            case MFnNumericData::k4Double:
                randomDouble(plug.child(0));
                randomDouble(plug.child(1));
                randomDouble(plug.child(2));
                randomDouble(plug.child(3));
                break;

            default: std::cout << ("Unknown numeric attribute type") << std::endl; break;
            }
        } break;

        case MFn::kUnitAttribute: {
            MFnUnitAttribute unAttr(plug.attribute());
            switch (unAttr.unitType()) {
            case MFnUnitAttribute::kAngle: randomAngle(plug); break;

            case MFnUnitAttribute::kDistance: randomDistance(plug); break;

            case MFnUnitAttribute::kTime: randomTime(plug); break;

            default: std::cout << ("Unknown unit attribute type") << std::endl; break;
            }
        } break;

        case MFn::kMatrixAttribute:
        case MFn::kFloatMatrixAttribute: {
        } break;

        case MFn::kMessageAttribute: {
        } break;

        case MFn::kEnumAttribute: {
            MFnEnumAttribute unAttr(plug.attribute());
            short            minVal;
            unAttr.getMin(minVal);
            short maxVal;
            unAttr.getMax(maxVal);
            short   random = 0;
            MStatus status = MS::kFailure;
            while (!status) {
                random = (rand() % (maxVal - minVal + 1)) + minVal;
                unAttr.fieldName(random, &status);
            }
            EXPECT_EQ(MStatus(MS::kSuccess), plug.setShort(random));
        } break;

        case MFn::kGenericAttribute: {
        } break;

        case MFn::kTimeAttribute: randomTime(plug); break;

        case MFn::kFloatAngleAttribute:
        case MFn::kDoubleAngleAttribute: randomAngle(plug); break;

        case MFn::kFloatLinearAttribute:
        case MFn::kDoubleLinearAttribute: randomDistance(plug); break;

        default:
            std::cout << ("Unknown attribute type \"") << plug.name().asChar() << "\" "
                      << plug.attribute().apiTypeStr() << std::endl;
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void randomNode(MObject node, const char* const attributeNames[], const uint32_t attributeCount)
{
    MFnDependencyNode fn(node);
    for (uint32_t i = 0; i < attributeCount; ++i) {
        MStatus status;
        MPlug   plug = fn.findPlug(attributeNames[i], &status);
        EXPECT_EQ(MStatus(MS::kSuccess), status);
        randomPlug(plug);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void randomAnimatedValue(MPlug plug, double startFrame, double endFrame, bool forceKeyframe)
{
    // If value is not keyable, set it to be a random value
    if (!forceKeyframe && !plug.isKeyable()) {
        randomPlug(plug);
        return;
    }

    MStatus status;
    // Create animation curve and set keys for current attribute in time range
    MFnAnimCurve fnCurve;
    fnCurve.create(plug, NULL, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    for (double t = startFrame, e = endFrame + 1e-3f; t < e; t += 1.0) {
        MTime tm(t, MTime::kFilm);

        switch (fnCurve.animCurveType()) {
        case MFnAnimCurve::kAnimCurveTL:
        case MFnAnimCurve::kAnimCurveTA:
        case MFnAnimCurve::kAnimCurveTU: {
            double value = randDouble();
            fnCurve.addKey(
                tm,
                value,
                MFnAnimCurve::kTangentGlobal,
                MFnAnimCurve::kTangentGlobal,
                NULL,
                &status);
            EXPECT_EQ(MStatus(MS::kSuccess), status);
            break;
        }
        default: {
            std::cout << "[DgNodeTranslator::setAngleAnim] Unexpected anim curve type: "
                      << fnCurve.animCurveType() << std::endl;
            break;
        }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void randomAnimatedNode(
    MObject           node,
    const char* const attributeNames[],
    const uint32_t    attributeCount,
    double            startFrame,
    double            endFrame,
    bool              forceKeyframe)
{
    MStatus           status;
    MFnDependencyNode fn(node);

    for (uint32_t i = 0; i < attributeCount; ++i) {
        MPlug plug = fn.findPlug(attributeNames[i], &status);
        EXPECT_EQ(MStatus(MS::kSuccess), status);

        switch (plug.attribute().apiType()) {
        case MFn::kNumericAttribute: {
            MFnNumericAttribute unAttr(plug.attribute());
            switch (unAttr.unitType()) {
            case MFnNumericData::kDouble:
            case MFnNumericData::kBoolean: {
                randomAnimatedValue(plug, startFrame, endFrame, forceKeyframe);
                break;
            }
            case MFnNumericData::k3Float:
            case MFnNumericData::k3Double: {
                randomAnimatedValue(plug.child(0), startFrame, endFrame, forceKeyframe);
                randomAnimatedValue(plug.child(1), startFrame, endFrame, forceKeyframe);
                randomAnimatedValue(plug.child(2), startFrame, endFrame, forceKeyframe);
                break;
            }
            default: {
                std::cout << ("Unknown numeric attribute type") << std::endl;
                break;
            }
            }
            break;
        }
        case MFn::kFloatLinearAttribute:
        case MFn::kDoubleLinearAttribute: {
            randomAnimatedValue(plug, startFrame, endFrame, forceKeyframe);
            break;
        }
        case MFn::kAttribute3Double:
        case MFn::kAttribute3Float: {
            randomAnimatedValue(plug.child(0), startFrame, endFrame, forceKeyframe);
            randomAnimatedValue(plug.child(1), startFrame, endFrame, forceKeyframe);
            randomAnimatedValue(plug.child(2), startFrame, endFrame, forceKeyframe);
            break;
        }
        case MFn::kEnumAttribute:
        case MFn::kMessageAttribute: {
            break;
        }
        default: {
            std::cout << ("Unknown attribute type \"") << plug.name().asChar() << "\" "
                      << plug.attribute().apiTypeStr() << std::endl;
            break;
        }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace test
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
