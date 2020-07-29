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
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "test_usdmaya.h"

#include <maya/MDGModifier.h>
#include <maya/MDoubleArray.h>
#include <maya/MFileIO.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnExpression.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>

using AL::usdmaya::fileio::AnimationTranslator;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test USD to attribute enum mappings
//----------------------------------------------------------------------------------------------------------------------
namespace {
MPlug m_outTime;
void  setUp()
{
    AL_OUTPUT_TEST_NAME("test_translators_AnimationTranslator");
    MGlobal::selectByName("time1", MGlobal::kReplaceList);
    MSelectionList sl;
    MObject        obj;
    MGlobal::getActiveSelectionList(sl);
    sl.getDependNode(0, obj);
    MFnDependencyNode time1Fn(obj);
    m_outTime = time1Fn.findPlug("outTime");
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, animationDrivenPlug)
{
    MFileIO::newFile(true);
    setUp();
    MStatus status;

    MFnDependencyNode fnb;
    MObject           addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnAnimCurve fna;
    MObject animCurve = fna.create(fnb.findPlug("input1"), MFnAnimCurve::kAnimCurveTL, 0, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(m_outTime, fna.findPlug("input")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    // anim curves with zero keyframes should be ignored
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));

    fna.addKey(MTime(0.0), 1.0);

    // anim curves with one keyframe should be ignored
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));

    fna.addKey(MTime(2.0), 2.0);

    // anim curves with two keyframes should be exported
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));

    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(animCurve);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, animationDrivenChildPlug)
{
    MFileIO::newFile(true);
    MStatus status;

    MFnDependencyNode fnb;
    MObject           addDoubleLinear1 = fnb.create("vectorProduct", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnAnimCurve fna;
    MObject      animCurve
        = fna.create(fnb.findPlug("input1").child(1), MFnAnimCurve::kAnimCurveTL, 0, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(m_outTime, fna.findPlug("input")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    // anim curves with zero keyframes should be ignored
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));

    fna.addKey(MTime(0.0), 1.0);

    // anim curves with one keyframe should be ignored
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));

    fna.addKey(MTime(2.0), 2.0);

    // anim curves with two keyframes should be exported
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));

    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(animCurve);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, animationDrivenElementPlug)
{
    MFileIO::newFile(true);
    setUp();
    MStatus status;

    MFnNurbsCurve fnb;
    MDoubleArray  knots;
    knots.append(0);
    knots.append(0);
    knots.append(0);
    knots.append(1);
    knots.append(1);
    knots.append(1);
    MPointArray points;
    points.append(MPoint());
    points.append(MPoint());
    points.append(MPoint());
    points.append(MPoint());
    MFnTransform fnt;
    MObject      transform = fnt.create();

    MObject addDoubleLinear1
        = fnb.create(points, knots, 3, MFnNurbsCurve::kOpen, false, false, transform, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnAnimCurve fna;
    MObject      animCurve = fna.create(
        fnb.findPlug("cp").elementByLogicalIndex(2).child(1),
        MFnAnimCurve::kAnimCurveTL,
        0,
        &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(m_outTime, fna.findPlug("input")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    // anim curves with zero keyframes should be ignored
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("cp"), true));
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("cp"), false));

    fna.addKey(MTime(0.0), 1.0);

    // anim curves with one keyframe should be ignored
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("cp"), true));
    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("cp"), false));

    fna.addKey(MTime(2.0), 2.0);

    // anim curves with two keyframes should be exported
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("cp"), true));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("cp"), false));

    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(animCurve);
    mod.deleteNode(transform);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, animationDrivenIndirectPlug)
{
    MFileIO::newFile(true);
    setUp();
    MStatus status;

    MFnDependencyNode fnb;
    MObject           addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnDependencyNode fnc;
    MObject           addDoubleLinear2 = fnc.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnAnimCurve fna;
    MObject animCurve = fna.create(fnb.findPlug("input1"), MFnAnimCurve::kAnimCurveTL, 0, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(m_outTime, fna.findPlug("input")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(fnb.findPlug("output"), fnc.findPlug("input1")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    EXPECT_TRUE(AnimationTranslator::isAnimated(fnc.findPlug("input1"), true));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnc.findPlug("input1"), false));

    mod.deleteNode(addDoubleLinear2);
    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(animCurve);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, expressionDrivenPlug)
{
    MFileIO::newFile(true);
    setUp();
    MStatus           status;
    MFnDependencyNode fnb;
    MObject           addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnExpression fna;
    MObject       expression = fna.create("input1 = frame;", addDoubleLinear1, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;

    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));

    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(expression);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, expressionDrivenIndirectPlug)
{
    MFileIO::newFile(true);
    setUp();
    MStatus           status;
    MFnDependencyNode fnb;
    MObject           addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnDependencyNode fnc;
    MObject           addDoubleLinear2 = fnc.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnExpression fna;
    MObject       expression = fna.create("input1 = frame;", addDoubleLinear1, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(fnb.findPlug("output"), fnc.findPlug("input1")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    EXPECT_TRUE(AnimationTranslator::isAnimated(fnc.findPlug("input1"), false));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnc.findPlug("input1"), true));

    mod.deleteNode(addDoubleLinear2);
    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(expression);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, expressionDrivenPlugNoTimeInput)
{
    MFileIO::newFile(true);
    setUp();
    MStatus           status;
    MFnDependencyNode fnb;
    MObject           addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnExpression fna;
    MObject       expression = fna.create("input1 = 4;", addDoubleLinear1, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;

    EXPECT_FALSE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), false));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnb.findPlug("input1"), true));

    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(expression);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, expressionDrivenIndirectPlugNoTimeInput)
{
    MFileIO::newFile(true);
    setUp();
    MStatus           status;
    MFnDependencyNode fnb;
    MObject           addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnDependencyNode fnc;
    MObject           addDoubleLinear2 = fnc.create("addDoubleLinear", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnExpression fna;
    MObject       expression = fna.create("input1 = 4;", addDoubleLinear1, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(fnb.findPlug("output"), fnc.findPlug("input1")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

    EXPECT_FALSE(AnimationTranslator::isAnimated(fnc.findPlug("input1"), false));
    EXPECT_TRUE(AnimationTranslator::isAnimated(fnc.findPlug("input1"), true));

    mod.deleteNode(addDoubleLinear2);
    mod.deleteNode(addDoubleLinear1);
    mod.deleteNode(expression);
    mod.doIt();
}

//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, considerToBeAnimationForNodeType)
{
    MFileIO::newFile(true);
    setUp();
    MStatus status;

    MFnDependencyNode animCurveTUFN;
    MObject           animCurveTU = animCurveTUFN.create("animCurveTU", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnDependencyNode animCurveTAFN;
    MObject           animCurveTA = animCurveTAFN.create("animCurveTA", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnDependencyNode animCurveTLFN;
    MObject           animCurveTL = animCurveTLFN.create("animCurveTL", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnDependencyNode animCurveTTFN;
    MObject           animCurveTT = animCurveTTFN.create("animCurveTT", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MFnDependencyNode transformFN;
    MObject           transform = transformFN.create("transform", &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MDGModifier mod;

    EXPECT_FALSE(AnimationTranslator::isAnimated(transformFN.findPlug("translateX"), false));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        mod.connect(animCurveTUFN.findPlug("output"), transformFN.findPlug("translateX")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());
    EXPECT_FALSE(AnimationTranslator::isAnimated(transformFN.findPlug("translateX"), true));

    EXPECT_FALSE(AnimationTranslator::isAnimated(transformFN.findPlug("rotateX"), false));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        mod.connect(animCurveTAFN.findPlug("output"), transformFN.findPlug("rotateX")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());
    EXPECT_FALSE(AnimationTranslator::isAnimated(transformFN.findPlug("rotateX"), true));

    MFnDependencyNode time1Fn(m_outTime.node());
    EXPECT_FALSE(AnimationTranslator::isAnimated(time1Fn.findPlug("enableTimewarp"), false));
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        mod.connect(animCurveTLFN.findPlug("output"), time1Fn.findPlug("enableTimewarp")));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());
    EXPECT_FALSE(AnimationTranslator::isAnimated(time1Fn.findPlug("enableTimewarp"), true));

    EXPECT_FALSE(AnimationTranslator::isAnimated(m_outTime, false));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(animCurveTTFN.findPlug("output"), m_outTime));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());
    EXPECT_FALSE(AnimationTranslator::isAnimated(m_outTime, true));

    mod.deleteNode(animCurveTU);
    mod.deleteNode(animCurveTL);
    mod.deleteNode(animCurveTT);
    mod.deleteNode(animCurveTA);
    mod.deleteNode(transform);

    mod.doIt();
}

TEST(translators_AnimationTranslator, isAnimatedTransform)
{
    MFileIO::newFile(true);
    setUp();
    MStatus status;

    MFnDagNode transformFN;
    MObject    root = transformFN.create("transform", MObject::kNullObj, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MObject parent = transformFN.create("transform", root, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MObject child = transformFN.create("transform", parent, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    MObject master = transformFN.create("transform", MObject::kNullObj, &status);
    EXPECT_EQ(MStatus(MS::kSuccess), status);

    EXPECT_FALSE(AnimationTranslator::isAnimatedTransform(child));

    transformFN.setObject(master);
    MPlug sourceTx = transformFN.findPlug("translateX");
    MPlug sourceR = transformFN.findPlug("rotate");
    MPlug sourceSz = transformFN.findPlug("scaleZ");
    MPlug suorceRO = transformFN.findPlug("rotateOrder");

    MDGModifier mod;
    transformFN.setObject(child);
    MPlug targetTx = transformFN.findPlug("translateX");

    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(sourceTx, targetTx));
    mod.doIt();
    EXPECT_TRUE(AnimationTranslator::isAnimatedTransform(child));
    mod.undoIt();
    EXPECT_FALSE(AnimationTranslator::isAnimatedTransform(child));

    transformFN.setObject(parent);
    MPlug targetR = transformFN.findPlug("rotate");
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(sourceR, targetR));
    mod.doIt();
    EXPECT_TRUE(AnimationTranslator::isAnimatedTransform(child));
    mod.undoIt();
    EXPECT_FALSE(AnimationTranslator::isAnimatedTransform(child));

    transformFN.setObject(root);
    MPlug targetSz = transformFN.findPlug("scaleZ");
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(sourceSz, targetSz));
    mod.doIt();
    EXPECT_TRUE(AnimationTranslator::isAnimatedTransform(child));
    mod.undoIt();
    EXPECT_FALSE(AnimationTranslator::isAnimatedTransform(child));

    MPlug targetRO = transformFN.findPlug("rotateOrder");
    EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(suorceRO, targetRO));
    mod.doIt();
    EXPECT_TRUE(AnimationTranslator::isAnimatedTransform(child));
    mod.undoIt();
    EXPECT_FALSE(AnimationTranslator::isAnimatedTransform(child));

    mod.deleteNode(master);
    mod.deleteNode(child);
    mod.deleteNode(parent);
    mod.deleteNode(root);
    mod.doIt();
}
