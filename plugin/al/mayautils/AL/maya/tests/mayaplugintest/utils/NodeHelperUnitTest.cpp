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
#include "AL/maya/tests/mayaplugintest/utils/NodeHelperUnitTest.h"

#include <maya/MColor.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatVector.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>
#include <maya/MString.h>
#include <maya/MVector.h>

#include <iostream>

namespace AL {
namespace maya {
namespace tests {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
const MString NodeHelperUnitTest::kTypeName = "AL_usdmaya_NodeHelperUnitTest";
const MTypeId NodeHelperUnitTest::kTypeId(0x4321);
MObject       NodeHelperUnitTest::loadFilename;
MObject       NodeHelperUnitTest::saveFilename;
MObject       NodeHelperUnitTest::directoryWithFile;
MObject       NodeHelperUnitTest::directory;
MObject       NodeHelperUnitTest::multiFile;
MObject       NodeHelperUnitTest::inPreFrame;
MObject       NodeHelperUnitTest::inBool;
MObject       NodeHelperUnitTest::inInt8;
MObject       NodeHelperUnitTest::inInt16;
MObject       NodeHelperUnitTest::inInt32;
MObject       NodeHelperUnitTest::inInt64;
MObject       NodeHelperUnitTest::inFloat;
MObject       NodeHelperUnitTest::inDouble;
MObject       NodeHelperUnitTest::inPoint;
MObject       NodeHelperUnitTest::inFloatPoint;
MObject       NodeHelperUnitTest::inVector;
MObject       NodeHelperUnitTest::inFloatVector;
MObject       NodeHelperUnitTest::inString;
MObject       NodeHelperUnitTest::inColour;
MObject       NodeHelperUnitTest::inMatrix;
MObject       NodeHelperUnitTest::inBoolHidden;
MObject       NodeHelperUnitTest::inInt8Hidden;
MObject       NodeHelperUnitTest::inInt16Hidden;
MObject       NodeHelperUnitTest::inInt32Hidden;
MObject       NodeHelperUnitTest::inInt64Hidden;
MObject       NodeHelperUnitTest::inFloatHidden;
MObject       NodeHelperUnitTest::inDoubleHidden;
MObject       NodeHelperUnitTest::inPointHidden;
MObject       NodeHelperUnitTest::inFloatPointHidden;
MObject       NodeHelperUnitTest::inVectorHidden;
MObject       NodeHelperUnitTest::inFloatVectorHidden;
MObject       NodeHelperUnitTest::inStringHidden;
MObject       NodeHelperUnitTest::inColourHidden;
MObject       NodeHelperUnitTest::inMatrixHidden;
MObject       NodeHelperUnitTest::outBool;
MObject       NodeHelperUnitTest::outInt8;
MObject       NodeHelperUnitTest::outInt16;
MObject       NodeHelperUnitTest::outInt32;
MObject       NodeHelperUnitTest::outInt64;
MObject       NodeHelperUnitTest::outFloat;
MObject       NodeHelperUnitTest::outDouble;
MObject       NodeHelperUnitTest::outPoint;
MObject       NodeHelperUnitTest::outFloatPoint;
MObject       NodeHelperUnitTest::outVector;
MObject       NodeHelperUnitTest::outFloatVector;
MObject       NodeHelperUnitTest::outString;
MObject       NodeHelperUnitTest::outColour;
MObject       NodeHelperUnitTest::outMatrix;

//----------------------------------------------------------------------------------------------------------------------
void* NodeHelperUnitTest::creator() { return new NodeHelperUnitTest; }

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelperUnitTest::initialise()
{
    try {
        setNodeType(kTypeName);

        addFrame("Fabrice");
        // just for fabrice :)
        inPreFrame = addBoolAttr(
            "perFrameAttr",
            "pfaattrh",
            true,
            kReadable | kWritable | kStorable | kKeyable | kHidden);

        addFrame("file");
        loadFilename = addFilePathAttr(
            "loadFilename",
            "lfp",
            kReadable | kWritable | kStorable,
            kLoad,
            "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc) (*.abc);;All files (*.*) (*.*)");
        saveFilename = addFilePathAttr(
            "saveFilename",
            "sfp",
            kReadable | kWritable | kStorable,
            kSave,
            "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc) (*.abc)");
        directoryWithFile = addFilePathAttr(
            "directoryWithFile", "dwf", kReadable | kWritable | kStorable, kDirectoryWithFiles);
        directory
            = addFilePathAttr("directory", "dir", kReadable | kWritable | kStorable, kDirectory);
        multiFile
            = addFilePathAttr("multiFile", "mf", kReadable | kWritable | kStorable, kMultiLoad);

        addFrame("hello");
        inBool
            = addBoolAttr("boolAttr", "battr", true, kReadable | kWritable | kStorable | kKeyable);
        inBoolHidden = addBoolAttr(
            "boolAttrHidden",
            "battrh",
            true,
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inInt8
            = addInt8Attr("int8Attr", "i8attr", 69, kReadable | kWritable | kStorable | kKeyable);
        inInt8Hidden = addInt8Attr(
            "int8AttrHidden",
            "i8attrh",
            69,
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inInt16 = addInt16Attr(
            "int16Attr", "i16attr", 69, kReadable | kWritable | kStorable | kKeyable);
        inInt16Hidden = addInt16Attr(
            "int16AttrHidden",
            "i16attrh",
            69,
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inInt32 = addInt32Attr(
            "int32Attr", "i32attr", 69, kReadable | kWritable | kStorable | kKeyable);
        inInt32Hidden = addInt32Attr(
            "int32AttrHidden",
            "i32attrh",
            69,
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inInt64 = addInt64Attr(
            "int64Attr", "i64attr", 69, kReadable | kWritable | kStorable | kKeyable);
        inInt64Hidden = addInt64Attr(
            "int64AttrHidden",
            "i64attrh",
            69,
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inFloat = addFloatAttr(
            "floatAttr", "fattr", 42.0f, kReadable | kWritable | kStorable | kKeyable);
        inFloatHidden = addFloatAttr(
            "floatAttrHidden",
            "fattrh",
            42.0f,
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inDouble = addDoubleAttr(
            "doubleAttr", "dattr", 21.0f, kReadable | kWritable | kStorable | kKeyable);
        inDoubleHidden = addDoubleAttr(
            "doubleAttrHidden",
            "dattrh",
            21.0f,
            kReadable | kWritable | kStorable | kKeyable | kHidden);

        addFrame("world");
        inPoint = addPointAttr(
            "pointAttr", "pattr", MPoint(2, 3, 4), kReadable | kWritable | kStorable | kKeyable);
        inPointHidden = addPointAttr(
            "pointAttrHidden",
            "pattrh",
            MPoint(2, 3, 4),
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inFloatPoint = addFloatPointAttr(
            "floatPointAttr",
            "fpattr",
            MFloatPoint(1, 2, 3),
            kReadable | kWritable | kStorable | kKeyable);
        inFloatPointHidden = addFloatPointAttr(
            "floatPointAttrHidden",
            "fpattrh",
            MFloatPoint(1, 2, 3),
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inVector = addVectorAttr(
            "vecAttr", "vattr", MPoint(2, 3, 4), kReadable | kWritable | kStorable | kKeyable);
        inVectorHidden = addVectorAttr(
            "vecAttrHidden",
            "vattrh",
            MPoint(2, 3, 4),
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inFloatVector = addFloatVectorAttr(
            "floatVecAttr",
            "fvattr",
            MFloatPoint(1, 2, 3),
            kReadable | kWritable | kStorable | kKeyable);
        inFloatVectorHidden = addFloatVectorAttr(
            "floatVecAttrHidden",
            "fvattrh",
            MFloatPoint(1, 2, 3),
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        inString
            = addStringAttr("stringAttr", "sattr", kReadable | kWritable | kStorable | kKeyable);
        inStringHidden = addStringAttr(
            "stringAttrHidden", "sattrh", kReadable | kWritable | kStorable | kKeyable | kHidden);
        inColour = addColourAttr(
            "colourAttr",
            "cattr",
            MColor(0.1f, 0.2f, 0.9f),
            kReadable | kWritable | kStorable | kKeyable);
        inColourHidden = addColourAttr(
            "colourAttrHidden",
            "cattrh",
            MColor(0.1f, 0.2f, 0.9f),
            kReadable | kWritable | kStorable | kKeyable | kHidden);
        double fm[4][4] = { { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 }, { 13, 14, 15, 16 } };
        MMatrix m(fm);
        inMatrix
            = addMatrixAttr("matrixAttr", "mattr", m, kReadable | kWritable | kStorable | kKeyable);
        inMatrixHidden = addMatrixAttr(
            "matrixAttrHidden",
            "mattrh",
            m,
            kReadable | kWritable | kStorable | kKeyable | kHidden);

        outBool = addBoolAttr("boolAttrOut", "obattr", false, kReadable);
        outInt8 = addInt8Attr("int8AttrOut", "oi8attr", 0, kReadable);
        outInt16 = addInt16Attr("int16AttrOut", "oi16attr", 0, kReadable);
        outInt32 = addInt32Attr("int32AttrOut", "oi32attr", 0, kReadable);
        outInt64 = addInt64Attr("int64AttrOut", "oi64attr", 0, kReadable);
        outFloat = addFloatAttr("floatAttrOut", "ofattr", 0, kReadable);
        outDouble = addDoubleAttr("doubleAttrOut", "odattr", 0, kReadable);
        outPoint = addPointAttr("pointAttrOut", "opattr", MPoint(), kReadable);
        outFloatPoint = addFloatPointAttr("floatPointAttrOut", "ofpattr", MFloatPoint(), kReadable);
        outVector = addVectorAttr("vecAttrOut", "ovattr", MPoint(), kReadable);
        outFloatVector = addFloatVectorAttr("floatVecAttrOut", "ofvattr", MFloatPoint(), kReadable);
        outString = addStringAttr("stringAttrOut", "osattr", kReadable);
        outColour = addColourAttr("colourAttrOut", "ocattr", MColor(), kReadable);
        outMatrix = addMatrixAttr("matrixAttrOut", "omattr", MMatrix(), kReadable);
    } catch (const MStatus& status) {
        return status;
    }

    attributeAffects(inBool, outBool);
    attributeAffects(inInt8, outInt8);
    attributeAffects(inInt16, outInt16);
    attributeAffects(inInt32, outInt32);
    attributeAffects(inInt64, outInt64);
    attributeAffects(inFloat, outFloat);
    attributeAffects(inDouble, outDouble);
    attributeAffects(inPoint, outPoint);
    attributeAffects(inFloatPoint, outFloatPoint);
    attributeAffects(inVector, outVector);
    attributeAffects(inFloatVector, outFloatVector);
    attributeAffects(inString, outString);
    attributeAffects(inColour, outColour);
    attributeAffects(inMatrix, outMatrix);

    generateAETemplate();
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelperUnitTest::compute(const MPlug& plug, MDataBlock& datablock)
{
    if (plug == outBool) {
        return outputBoolValue(datablock, outBool, inputBoolValue(datablock, inBool));
    } else if (plug == outInt8) {
        return outputInt8Value(datablock, outInt8, inputInt8Value(datablock, inInt8));
    } else if (plug == outInt16) {
        return outputInt16Value(datablock, outInt16, inputInt16Value(datablock, inInt16));
    } else if (plug == outInt32) {
        return outputInt32Value(datablock, outInt32, inputInt32Value(datablock, inInt32));
    } else if (plug == outInt64) {
        return outputInt64Value(datablock, outInt64, inputInt64Value(datablock, inInt64));
    } else if (plug == outFloat) {
        return outputFloatValue(datablock, outFloat, inputFloatValue(datablock, inFloat));
    } else if (plug == outDouble) {
        return outputDoubleValue(datablock, outDouble, inputDoubleValue(datablock, inDouble));
    } else if (plug == outPoint) {
        return outputPointValue(datablock, outPoint, inputPointValue(datablock, inPoint));
    } else if (plug == outFloatPoint) {
        return outputFloatPointValue(
            datablock, outFloatPoint, inputFloatPointValue(datablock, inFloatPoint));
    } else if (plug == outVector) {
        return outputVectorValue(datablock, outVector, inputVectorValue(datablock, inVector));
    } else if (plug == outFloatVector) {
        return outputFloatVectorValue(
            datablock, outFloatVector, inputFloatVectorValue(datablock, inFloatVector));
    } else if (plug == outString) {
        return outputStringValue(datablock, outString, inputStringValue(datablock, inString));
    } else if (plug == outColour) {
        return outputColourValue(datablock, outColour, inputColourValue(datablock, inColour));
    } else if (plug == outMatrix) {
        return outputMatrixValue(datablock, outMatrix, inputMatrixValue(datablock, inMatrix));
    }
    return MS::kInvalidParameter;
}

//----------------------------------------------------------------------------------------------------------------------
#define MEL_SCRIPT(TEXT) #TEXT
static const char* const testScript = MEL_SCRIPT(proc string compareAttributes() {
    string $result = "";
    string $n = `createNode "NodeHelperUnitTest"`;
    $boolIn = `getAttr($n + ".boolAttr")`;
    $boolOut = `getAttr($n + ".boolAttrOut")`;
    if ($boolIn != $boolOut)
        $result += "boolAttr failed\n";

    $intIn8 = `getAttr($n + ".int8Attr")`;
    $intOut8 = `getAttr($n + ".int8AttrOut")`;
    if ($intIn8 != $intOut8)
        $result += "int8Attr failed\n";

    $intIn16 = `getAttr($n + ".int16Attr")`;
    $intOut16 = `getAttr($n + ".int16AttrOut")`;
    if ($intIn16 != $intOut16)
        $result += "int16Attr failed\n";

    $intIn32 = `getAttr($n + ".int32Attr")`;
    $intOut32 = `getAttr($n + ".int32AttrOut")`;
    if ($intIn32 != $intOut32)
        $result += "intAttr32 failed\n";

    $intIn64 = `getAttr($n + ".intAttr64")`;
    $intOut64 = `getAttr($n + ".intAttr64Out")`;
    if ($intIn64 != $intOut64)
        $result += "intAttr64 failed\n";

    $floatIn = `getAttr($n + ".floatAttr")`;
    $floatOut = `getAttr($n + ".floatAttrOut")`;
    if ($floatIn != $floatOut)
        $result += "floatAttr failed\n";

    $doubleIn = `getAttr($n + ".doubleAttr")`;
    $doubleOut = `getAttr($n + ".doubleAttrOut")`;
    if ($doubleIn != $doubleOut)
        $result += "doubleAttr failed\n";

    $pointAttrIn = `getAttr($n + ".pointAttr")`;
    $pointAttrOut = `getAttr($n + ".pointAttrOut")`;
    if ($pointAttrIn[0] != $pointAttrOut[0] || $pointAttrIn[1] != $pointAttrOut[1]
        || $pointAttrIn[2] != $pointAttrOut[2])
        $result += "pointAttr failed\n";

    $fpointAttrIn = `getAttr($n + ".floatPointAttr")`;
    $fpointAttrOut = `getAttr($n + ".floatPointAttrOut")`;
    if ($fpointAttrIn[0] != $fpointAttrOut[0] || $fpointAttrIn[1] != $fpointAttrOut[1]
        || $fpointAttrIn[2] != $fpointAttrOut[2])
        $result += "floatPointAttr failed\n";

    $fpointAttrIn = `getAttr($n + ".vecAttr")`;
    $fpointAttrOut = `getAttr($n + ".vecAttrOut")`;
    if ($fpointAttrIn[0] != $fpointAttrOut[0] || $fpointAttrIn[1] != $fpointAttrOut[1]
        || $fpointAttrIn[2] != $fpointAttrOut[2])
        $result += "vecAttr failed\n";

    $fpointAttrIn = `getAttr($n + ".floatVecAttr")`;
    $fpointAttrOut = `getAttr($n + ".floatVecAttrOut")`;
    if ($fpointAttrIn[0] != $fpointAttrOut[0] || $fpointAttrIn[1] != $fpointAttrOut[1]
        || $fpointAttrIn[2] != $fpointAttrOut[2])
        $result += "floatVecAttr failed\n";

    setAttr - type "string"($n + ".stringAttr") "someText";
    $stringAttrIn = `getAttr($n + ".stringAttr")`;
    $stringAttrOut = `getAttr($n + ".stringAttrOut")`;
    if ($stringAttrIn != $stringAttrOut)
        $result += "stringAttr failed\n";

    $fpointAttrIn = `getAttr($n + ".colourAttr")`;
    $fpointAttrOut = `getAttr($n + ".colourAttrOut")`;
    if ($fpointAttrIn[0] != $fpointAttrOut[0] || $fpointAttrIn[1] != $fpointAttrOut[1]
        || $fpointAttrIn[2] != $fpointAttrOut[2])
        $result += "colourAttr failed\n";

    $mIn = `getAttr($n + ".matrixAttr")`;
    $mOut = `getAttr($n + ".matrixAttrOut")`;
    for ($i = 0; $i < 16; $i++) {
        if ($mIn[$i] != $mOut[$i]) {
            $result += "matrixAttr Failed\n";
        }
    }
    delete $n;
    return $result;
} compareAttributes(););

//----------------------------------------------------------------------------------------------------------------------
void NodeHelperUnitTest::runUnitTest()
{
    MString result;
    MGlobal::executeCommand(testScript, result);
    if (result.length()) {
        std::cout << "NodeHelperUnitTest failed\n" << result.length() << "\n";
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace tests
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
