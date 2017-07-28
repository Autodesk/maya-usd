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

#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"

#include "maya/MDagModifier.h"
#include "maya/MFnDagNode.h"

using AL::usdmaya::fileio::ExporterParams;
using AL::usdmaya::fileio::ImporterParams;
using AL::usdmaya::fileio::translators::DagNodeTranslator;
using AL::usdmaya::fileio::translators::TransformTranslator;
using AL::usdmaya::fileio::AnimationTranslator;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the alUsdNodeHelper.
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_TranformTranslator, io)
{
  DagNodeTranslator::registerType();
  TransformTranslator::registerType();
  for(int i = 0; i < 100; ++i)
  {
    MFnDagNode fn;

    MObject node = fn.create("transform");

    const char* const attributeNames[] = {
        "rotate",
        "rotateAxis",
        "rotatePivot",
        "rotatePivotTranslate",
        "scale",
        "scalePivot",
        "scalePivotTranslate",
        "shear",
        "inheritsTransform",
        "translate",
        "rotateOrder"
    };
    const uint32_t numAttributes = sizeof(attributeNames) / sizeof(const char* const);

    randomNode(node, attributeNames, numAttributes);

    // generate a prim for testing
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/hello"));
    UsdPrim prim = xform.GetPrim();

    ExporterParams eparams;
    ImporterParams iparams;
    TransformTranslator xlator;

    EXPECT_EQ(MStatus(MS::kSuccess), TransformTranslator::copyAttributes(node, prim, eparams));

    MObject nodeB = xlator.createNode(prim, MObject::kNullObj,
        "transform", iparams);

    EXPECT_TRUE(nodeB != MObject::kNullObj);

    // now make sure the imported node matches the one we started with
    compareNodes(node, nodeB, attributeNames, numAttributes, true);

    MDagModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.deleteNode(node));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.deleteNode(nodeB));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());
  }
}

TEST(translators_TranformTranslator, animated_io)
{
  const double startFrame = 1.0;
  const double endFrame = 20.0;

  DagNodeTranslator::registerType();
  TransformTranslator::registerType();
  for(int i = 0; i < 100; ++i)
  {
    MFnDagNode fn;

    MObject node = fn.create("transform");

    const char* const attributeNames[] = {
        "rotate",
        "rotateAxis",
        "rotatePivot",
        "rotatePivotTranslate",
        "scale",
        "scalePivot",
        "scalePivotTranslate",
        "shear",
        "inheritsTransform",
        "translate",
        "rotateOrder"
    };
    const uint32_t numAttributes = sizeof(attributeNames) / sizeof(const char* const);

    randomAnimatedNode(node, attributeNames, numAttributes, startFrame, endFrame);

    // generate a prim for testing
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/hello"));
    UsdPrim prim = xform.GetPrim();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Export animation
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    ExporterParams eparams;
    eparams.m_minFrame = startFrame;
    eparams.m_maxFrame = endFrame;
    eparams.m_animation = true;
    eparams.m_animTranslator = new AnimationTranslator;

    EXPECT_EQ(MStatus(MS::kSuccess), TransformTranslator::copyAttributes(node, prim, eparams));
    eparams.m_animTranslator->exportAnimation(eparams);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Import animation
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    ImporterParams iparams;
    TransformTranslator xlator;
    MObject nodeB = xlator.createNode(prim, MObject::kNullObj, "transform", iparams);

    EXPECT_TRUE(nodeB != MObject::kNullObj);

    // now make sure the imported node matches the one we started with
    for(double t = eparams.m_minFrame, e = eparams.m_maxFrame + 1e-3f; t < e; t += 1.0)
    {
      MGlobal::viewFrame(t);
      compareNodes(node, nodeB, attributeNames, numAttributes, true);
    }

    MDagModifier mod;
    EXPECT_EQ(MStatus(MS::kSuccess), mod.deleteNode(node));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.deleteNode(nodeB));
    EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());
  }
}
