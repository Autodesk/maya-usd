//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include "AL/usdmaya/fileio/AnimationTranslator.h"

#include "maya/MDGModifier.h"
#include "maya/MDoubleArray.h"
#include "maya/MFnAnimCurve.h"
#include "maya/MFileIO.h"
#include "maya/MFnTransform.h"
#include "maya/MFnNurbsCurve.h"
#include "maya/MFnExpression.h"
#include "maya/MGlobal.h"
#include "maya/MPointArray.h"
#include "maya/MSelectionList.h"

using AL::usdmaya::fileio::AnimationTranslator;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test USD to attribute enum mappings
//----------------------------------------------------------------------------------------------------------------------
namespace
{
MPlug m_outTime;

void setUp()
{
  AL_OUTPUT_TEST_NAME("test_translators_AnimationTranslator");
  MGlobal::selectByName("time1", MGlobal::kReplaceList);
  MSelectionList sl;
  MObject obj;
  MGlobal::getActiveSelectionList(sl);
  sl.getDependNode(0, obj);
  MFnDependencyNode fn(obj);
  m_outTime = fn.findPlug("outTime");
}

void tearDown()
{
}

}


//----------------------------------------------------------------------------------------------------------------------
TEST(translators_AnimationTranslator, animationDrivenPlug)
{
  MFileIO::newFile(true);
  setUp();
  MStatus status;

  MFnDependencyNode fnb;
  MObject addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
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
  MObject addDoubleLinear1 = fnb.create("vectorProduct", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnAnimCurve fna;
  MObject animCurve = fna.create(fnb.findPlug("input1").child(1), MFnAnimCurve::kAnimCurveTL, 0, &status);
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
  MDoubleArray knots;
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
  MObject transform = fnt.create();

  MObject addDoubleLinear1 = fnb.create(points, knots, 3, MFnNurbsCurve::kOpen, false, false, transform, &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnAnimCurve fna;
  MObject animCurve = fna.create(fnb.findPlug("cp").elementByLogicalIndex(2).child(1), MFnAnimCurve::kAnimCurveTL, 0, &status);
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
  MObject addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnDependencyNode fnc;
  MObject addDoubleLinear2 = fnc.create("addDoubleLinear", &status);
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
  MStatus status;
  MFnDependencyNode fnb;
  MObject addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnExpression fna;
  MObject expression = fna.create("input1 = frame;", addDoubleLinear1, &status);
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
  MStatus status;
  MFnDependencyNode fnb;
  MObject addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnDependencyNode fnc;
  MObject addDoubleLinear2 = fnc.create("addDoubleLinear", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnExpression fna;
  MObject expression = fna.create("input1 = frame;", addDoubleLinear1, &status);
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
  MStatus status;
  MFnDependencyNode fnb;
  MObject addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnExpression fna;
  MObject expression = fna.create("input1 = 4;", addDoubleLinear1, &status);
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
  MStatus status;
  MFnDependencyNode fnb;
  MObject addDoubleLinear1 = fnb.create("addDoubleLinear", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnDependencyNode fnc;
  MObject addDoubleLinear2 = fnc.create("addDoubleLinear", &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MFnExpression fna;
  MObject expression = fna.create("input1 = 4;", addDoubleLinear1, &status);
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
