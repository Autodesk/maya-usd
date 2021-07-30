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
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"
#include "test_usdmaya.h"

#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MObjectArray.h>
#include <maya/MObjectHandle.h>

using AL::maya::test::compareNodes;
using AL::maya::test::randomAnimatedNode;
using AL::maya::test::randomNode;
using AL::usdmaya::fileio::AnimationTranslator;
using AL::usdmaya::fileio::ExporterParams;
using AL::usdmaya::fileio::ImporterParams;
using AL::usdmaya::fileio::translators::DagNodeTranslator;
using AL::usdmaya::fileio::translators::TransformTranslator;

using AL::maya::test::buildTempPath;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the alUsdNodeHelper.
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_TranformTranslator, io)
{
    DagNodeTranslator::registerType();
    TransformTranslator::registerType();
    for (int i = 0; i < 100; ++i) {
        MFnDagNode fn;

        MObject node = fn.create("transform");

        const char* const attributeNames[] = {
            "rotate",     "rotateAxis",          "rotatePivot", "rotatePivotTranslate", "scale",
            "scalePivot", "scalePivotTranslate", "shear",       "inheritsTransform",    "translate",
            "rotateOrder"
        };
        const uint32_t numAttributes = sizeof(attributeNames) / sizeof(const char* const);

        randomNode(node, attributeNames, numAttributes);

        // generate a prim for testing
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   xform = UsdGeomXform::Define(stage, SdfPath("/hello"));
        UsdPrim        prim = xform.GetPrim();

        ExporterParams      eparams;
        ImporterParams      iparams;
        TransformTranslator xlator;

        EXPECT_EQ(
            MStatus(MS::kSuccess),
            TransformTranslator::copyAttributes(
                node, prim, eparams, fn.dagPath(), eparams.m_exportInWorldSpace));

        MObject nodeB = xlator.createNode(prim, MObject::kNullObj, "transform", iparams);

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
    for (int i = 0; i < 100; ++i) {
        MFnDagNode fn;

        MFileIO::newFile(true);

        MObject node = fn.create("transform");

        const char* const attributeNames[] = { "rotate",
                                               "rotateAxis",
                                               "rotatePivot",
                                               "rotatePivotTranslate",
                                               "scale",
                                               "scalePivot",
                                               "scalePivotTranslate",
                                               "shear",
                                               "inheritsTransform",
                                               "translate",
                                               "rotateOrder",
                                               "visibility" };

        const char* const keyableAttributeNames[]
            = { "rotateX", "rotateY",    "rotateZ",    "scaleX",     "scaleY",
                "scaleZ",  "translateX", "translateY", "translateZ", "visibility" };
        const uint32_t numAttributes = sizeof(attributeNames) / sizeof(const char* const);
        const uint32_t numKeyableAttributes
            = sizeof(keyableAttributeNames) / sizeof(const char* const);

        randomAnimatedNode(node, attributeNames, numAttributes, startFrame, endFrame);

        // generate a prim for testing
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   xform = UsdGeomXform::Define(stage, SdfPath("/hello"));
        UsdPrim        prim = xform.GetPrim();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Export animation
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        ExporterParams eparams;
        eparams.m_minFrame = startFrame;
        eparams.m_maxFrame = endFrame;
        eparams.m_animation = true;
        eparams.m_animTranslator = new AnimationTranslator;

        EXPECT_EQ(
            MStatus(MS::kSuccess),
            TransformTranslator::copyAttributes(
                node, prim, eparams, fn.dagPath(), eparams.m_exportInWorldSpace));
        eparams.m_animTranslator->exportAnimation(eparams);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Import animation
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        ImporterParams      iparams;
        TransformTranslator xlator;
        MObject nodeB = xlator.createNode(prim, MObject::kNullObj, "transform", iparams);

        EXPECT_TRUE(nodeB != MObject::kNullObj);

        // now make sure the imported node matches the one we started with
        for (double t = eparams.m_minFrame, e = eparams.m_maxFrame + 1e-3f; t < e; t += 1.0) {
            MGlobal::viewFrame(t);
            compareNodes(node, nodeB, attributeNames, numAttributes, true);
        }
        unsigned int initAnimCurveCount = iparams.m_newAnimCurves.length();
        EXPECT_TRUE(initAnimCurveCount);
        MDagModifier mod;

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // animCurve nodes management
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        MFnDependencyNode nodeFn(nodeB);

        // Import for multiple times and we should still be reusing the original animCurves:
        constexpr const uint32_t times { 10 };
        for (uint32_t l = 0; l < times; l++) {
            EXPECT_EQ(MStatus(MS::kSuccess), xlator.copyAttributes(prim, nodeB, iparams));
            EXPECT_EQ(iparams.m_newAnimCurves.length(), initAnimCurveCount);
            for (uint32_t i = 0; i < numKeyableAttributes; ++i) {
                MPlug plug = nodeFn.findPlug(keyableAttributeNames[i], true);
                MPlug sourcePlug = plug.source();
                EXPECT_FALSE(sourcePlug.isNull());
                MObject node = sourcePlug.node();
                EXPECT_TRUE(MObjectHandle(node).isValid());
                bool isOldNode = false;
                for (unsigned int i = 0; i < iparams.m_newAnimCurves.length(); ++i) {
                    if (node == iparams.m_newAnimCurves[i]) {
                        isOldNode = true;
                        break;
                    }
                }
                EXPECT_TRUE(isOldNode);
            }
        }

        EXPECT_EQ(MStatus(MS::kSuccess), mod.deleteNode(node));
        EXPECT_EQ(MStatus(MS::kSuccess), mod.deleteNode(nodeB));
        EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());
    }
}

TEST(translators_TranformTranslator, default_rotateOrder_true)
{
    // If the Rotate order is not default, confrim xformOpOrder value is set correctly
    DagNodeTranslator::registerType();
    TransformTranslator::registerType();

    MDagModifier fn;
    MObject      node = fn.createNode("transform");
    fn.doIt();

    MFnDependencyNode nodeFn(node);
    MPlug             plug = nodeFn.findPlug("rotateOrder");
    plug.setInt(5); // rotateZYX

    MDagPath nodeDagPath;
    MDagPath::getAPathTo(node, nodeDagPath);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    ExporterParams eparams;
    eparams.m_animation = false;

    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/rotateOrder_true"));
    UsdPrim      prim = xform.GetPrim();
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        TransformTranslator::copyAttributes(
            node, prim, eparams, nodeDagPath, eparams.m_exportInWorldSpace));

    bool reset;
    auto xformOps = xform.GetOrderedXformOps(&reset);
    ASSERT_TRUE(1 == xformOps.size());

    GfVec3f resultValue;
    xformOps[0].Get(&resultValue);
    auto resultName = xformOps[0].GetName().GetString();

    ASSERT_EQ(resultName, "xformOp:rotateZYX");
    ASSERT_EQ(resultValue, GfVec3f(0.f, 0.f, 0.f));
}

TEST(translators_TranformTranslator, default_rotateOrder_false)
{
    // If the Rotate order is default, confrim xformOpOrder value is not set
    DagNodeTranslator::registerType();
    TransformTranslator::registerType();

    MDagModifier fn;
    MObject      node = fn.createNode("transform");
    fn.doIt();

    MDagPath nodeDagPath;
    MDagPath::getAPathTo(node, nodeDagPath);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    ExporterParams eparams;
    eparams.m_animation = false;

    UsdGeomXform xform = UsdGeomXform::Define(stage, SdfPath("/rotateOrder_false"));
    UsdPrim      prim = xform.GetPrim();
    EXPECT_EQ(
        MStatus(MS::kSuccess),
        TransformTranslator::copyAttributes(
            node, prim, eparams, nodeDagPath, eparams.m_exportInWorldSpace));

    auto    attribute = xform.GetXformOpOrderAttr();
    VtValue currentValue;
    attribute.Get(&currentValue, UsdTimeCode::Default());

    ASSERT_TRUE(currentValue.GetArraySize() == 0);
}

TEST(translators_TranformTranslator, worldSpaceExport)
{
    MFileIO::newFile(true);

    // create cube, parent to a group, and move the parent
    MString buildCommand = "polyCube; group; move 1 2 3; select -r \"pCube1\";";
    ASSERT_TRUE(MGlobal::executeCommand(buildCommand));

    auto    path = buildTempPath("AL_USDMayaTests_exportInWorldSpace.usda");
    MString exportCommand = "file -force -options "
                            "\"Dynamic_Attributes=0;Meshes=1;Mesh_Face_Connects=1;Mesh_Points=1;"
                            "Mesh_Normals=0;Mesh_Vertex_Creases=0;"
                            "Mesh_Edge_Creases=0;Mesh_UVs=0;Mesh_UV_Only=0;Mesh_Points_as_PRef=0;"
                            "Mesh_Colours=0;Mesh_Holes=0;Compaction_Level=0;"
                            "Nurbs_Curves=0;Duplicate_Instances=0;Merge_Transforms=1;Animation=1;"
                            "Use_Timeline_Range=0;Frame_Min=1;"
                            "Frame_Max=2;Sub_Samples=1;Filter_Sample=0;Export_At_Which_Time=2;"
                            "Export_In_World_Space=1;\" -typ \"AL usdmaya export\" -pr -es ";
    exportCommand += "\"";
    exportCommand += path;
    exportCommand += "\"";

    // export cube in world space
    ASSERT_TRUE(MGlobal::executeCommand(exportCommand));

    auto stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);

    auto prim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    ASSERT_TRUE(prim);

    UsdGeomXform xform(prim);
    bool         resetsXformStack = 0;
    GfMatrix4d   transform;
    xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());

    // make sure the local space tm values match the world coords.
    EXPECT_NEAR(1.0, transform[3][0], 1e-6);
    EXPECT_NEAR(2.0, transform[3][1], 1e-6);
    EXPECT_NEAR(3.0, transform[3][2], 1e-6);
}

TEST(translators_TranformTranslator, worldSpaceGroupsExport)
{
    MFileIO::newFile(true);

    // clang-format off
  // Command below creates a hierarchy like this:
  //    Maya               Selection      Expected in USD
  //      A                     *             A
  //        X1                  *               X1
  //          Y1                *                 Y1          # Only Y1 exists
  //          Y2                                              # Siblings should be excluded
  //          Y3
  //        X2                                                # X2 is excluded because X1 is selected
  //          Y1
  //          Y2
  //          Y3
  //        X3                                  X3            # root node A and child Y3 is selected, X3 is preserved
  //          Y1                                              # Y1 is excluded because of Y3
  //          Y2                                              # Y2 is excluded because of Y3
  //          Y3                *                 Y3
  //      B                                                   # B is excluded
  //        X1                                                # root node B is not selected, X1 is excluded
  //          Y1                *             Y1              # none of Y1's parents are selected, Y1 becomes a root prim in USD
  //          Y2                *             Y2              # none of Y2's parents are selected, Y2 becomes a root prim in USD
  //          Y3
  //        X2
  //          Y1
  //            cube1
  //              cube1Shape    *             cube1           # none of Y2's parents are selected, the cube becomes a root prim in USD
  //          Y2
  //          Y3
  //        X3
  //      C                     *             C               # C is exported as it is
  //        X1                                  X1            # X1 is exported as it is
  //        X2                                  X2            # X2 is exported as it is
    // clang-format on

    MString buildCommand =
        R"(
      polyCube -n "cube1";
      group -n "Y1" "cube1";        move 1 1 1; duplicate -n "Y2" "Y1"; duplicate -n "Y3" "Y1";
      group -n "X1" "Y1" "Y2" "Y3"; move 1 1 1; duplicate -n "X2" "X1"; duplicate -n "X3" "X1";
      group -n "A" "X1" "X2" "X3";  move 1 1 1; duplicate -n "B" "A";   duplicate -n "C" "A";
      delete "|C|X3" "|C|X1|Y2" "|C|X1|Y3";
      select -r "|A" "|A|X1" "|A|X1|Y1" "|A|X3|Y3" "|B|X1|Y1" "|B|X1|Y2" "|B|X2|Y1|cube1|cube1Shape" "|C";
    )";
    ASSERT_TRUE(MGlobal::executeCommand(buildCommand));

    auto    path = buildTempPath("AL_USDMayaTests_exportInWorldSpaceMultipleGroups.usda");
    MString exportCommand = "file -force -options "
                            "\"Dynamic_Attributes=0;Meshes=1;Mesh_Face_Connects=1;Mesh_Points=1;"
                            "Mesh_Normals=0;Mesh_Vertex_Creases=0;"
                            "Mesh_Edge_Creases=0;Mesh_UVs=0;Mesh_UV_Only=0;Mesh_Points_as_PRef=0;"
                            "Mesh_Colours=0;Mesh_Holes=0;Compaction_Level=0;"
                            "Nurbs_Curves=0;Duplicate_Instances=0;Merge_Transforms=1;Animation=1;"
                            "Use_Timeline_Range=0;Frame_Min=1;"
                            "Frame_Max=2;Sub_Samples=1;Filter_Sample=0;Export_At_Which_Time=2;"
                            "Export_In_World_Space=1;\" -typ \"AL usdmaya export\" -pr -es ";
    exportCommand += "\"";
    exportCommand += path;
    exportCommand += "\"";

    // Export cube in world space
    ASSERT_TRUE(MGlobal::executeCommand(exportCommand));

    auto stage = UsdStage::Open(path);
    ASSERT_TRUE(stage);

    bool       resetsXformStack = false;
    GfMatrix4d transform;

    // === Test the A group
    auto primA = stage->GetPrimAtPath(SdfPath("/A"));
    ASSERT_TRUE(primA);
    {
        UsdGeomXform xform(primA);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // Make sure the local space tm values match the world coords.
        EXPECT_NEAR(1.0, transform[3][0], 1e-6);
        EXPECT_NEAR(1.0, transform[3][1], 1e-6);
        EXPECT_NEAR(1.0, transform[3][2], 1e-6);
    }

    // /A/X1: the child group should have no change
    auto primA_X1 = stage->GetPrimAtPath(SdfPath("/A/X1"));
    ASSERT_TRUE(primA_X1);
    {
        UsdGeomXform xform(primA_X1);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // The local space tm values should be untouched
        EXPECT_NEAR(1.0, transform[3][0], 1e-6);
        EXPECT_NEAR(1.0, transform[3][1], 1e-6);
        EXPECT_NEAR(1.0, transform[3][2], 1e-6);
    }
    auto primA_X1_Y1 = stage->GetPrimAtPath(SdfPath("/A/X1/Y1"));
    ASSERT_TRUE(primA_X1_Y1);
    {
        UsdGeomXform xform(primA_X1_Y1);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // The local space tm values should be untouched
        EXPECT_NEAR(1.0, transform[3][0], 1e-6);
        EXPECT_NEAR(1.0, transform[3][1], 1e-6);
        EXPECT_NEAR(1.0, transform[3][2], 1e-6);
    }
    auto primA_X1_Y1_cube = stage->GetPrimAtPath(SdfPath("/A/X1/Y1/cube1"));
    ASSERT_TRUE(primA_X1_Y1_cube);
    {
        UsdGeomXform xform(primA_X1_Y1_cube);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // The local space tm values should be untouched
        EXPECT_NEAR(0.0, transform[3][0], 1e-6);
        EXPECT_NEAR(0.0, transform[3][1], 1e-6);
        EXPECT_NEAR(0.0, transform[3][2], 1e-6);
    }

    // Test the rest of the prims in hierarchy
    // /A/X2 should not be there
    ASSERT_FALSE(stage->GetPrimAtPath(SdfPath("/A/X2")));
    // /A/X3/Y3/cube1 should be preserved
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/A/X3/Y3/cube1")));

    // === Test the B group
    // /B should not be there
    ASSERT_FALSE(stage->GetPrimAtPath(SdfPath("/B")));

    // The child Y1 in B should now become a root level prim with world space xform baked
    auto primB_Y1 = stage->GetPrimAtPath(SdfPath("/Y1"));
    ASSERT_TRUE(primB_Y1);
    {
        UsdGeomXform xform(primB_Y1);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // The local space tm values should be untouched
        EXPECT_NEAR(3.0, transform[3][0], 1e-6);
        EXPECT_NEAR(3.0, transform[3][1], 1e-6);
        EXPECT_NEAR(3.0, transform[3][2], 1e-6);
    }

    // The nested cube under Y1 should be untouched
    auto primB_Y1_cube = stage->GetPrimAtPath(SdfPath("/Y1/cube1"));
    ASSERT_TRUE(primB_Y1_cube);
    {
        UsdGeomXform xform(primB_Y1_cube);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // The local space tm values should be untouched
        EXPECT_NEAR(0.0, transform[3][0], 1e-6);
        EXPECT_NEAR(0.0, transform[3][1], 1e-6);
        EXPECT_NEAR(0.0, transform[3][2], 1e-6);
    }

    // Same for Y2 in B
    auto primB_Y2 = stage->GetPrimAtPath(SdfPath("/Y2"));
    ASSERT_TRUE(primB_Y2);
    {
        UsdGeomXform xform(primB_Y2);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // The local space tm values should be untouched
        EXPECT_NEAR(3.0, transform[3][0], 1e-6);
        EXPECT_NEAR(3.0, transform[3][1], 1e-6);
        EXPECT_NEAR(3.0, transform[3][2], 1e-6);
    }
    auto primB_Y2_cube = stage->GetPrimAtPath(SdfPath("/Y2/cube1"));
    ASSERT_TRUE(primB_Y2_cube);

    // leaf cube in B also becomes a root level prim
    auto primB_cube = stage->GetPrimAtPath(SdfPath("/cube1"));
    ASSERT_TRUE(primB_cube);
    {
        UsdGeomXform xform(primB_cube);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // The local space tm values should be untouched
        EXPECT_NEAR(3.0, transform[3][0], 1e-6);
        EXPECT_NEAR(3.0, transform[3][1], 1e-6);
        EXPECT_NEAR(3.0, transform[3][2], 1e-6);
    }

    // === Test the C group hierarchy
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X1")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X1/Y1")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X1/Y1/cube1")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X2")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X2/Y1")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X2/Y1/cube1")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X2/Y2")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X2/Y2/cube1")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X2/Y3")));
    ASSERT_TRUE(stage->GetPrimAtPath(SdfPath("/C/X2/Y3/cube1")));
}

TEST(translators_TranformTranslator, withOffsetParentMatrix)
{
    MFileIO::newFile(true);

    MString buildCommand = R"(
      polyCube;
      // Local translate
      move 1 2 3 pCube1;
      // Offset translate
      setAttr "pCube1.offsetParentMatrix" -type "matrix" 1 0 0 0 0 1 0 0 0 0 1 0 1 2 3 1 ;
      group "pCube1";
      move 1 2 3 group1;
      setAttr "group1.rp" -type "double3" 0 0 0;
      setAttr "group1.sp" -type "double3" 0 0 0;
      // Translate XYZ = (1, 2, 3)
      // Rotate XYZ == (30.0, 45.0, 60.0)
      setAttr "group1.offsetParentMatrix" -type "matrix" 0.353553 0.612373 -0.707107 0 -0.573223 0.739199 0.353554 0 0.739199 0.28033 0.612372 0 1 2 3 1 ;
      group "group1";
      move 1 2 3 group2;
      setAttr "group2.rp" -type "double3" 0 0 0;
      setAttr "group2.sp" -type "double3" 0 0 0;
      // Scale XYZ = (2, 2, 2)
      //setAttr "group2.offsetParentMatrix" -type "matrix" 2 0 0 0 0 2 0 0 0 0 2 0 0 0 0 1 ;
      group "group2";
      move 1 2 3 group3;
      setAttr "group3.rp" -type "double3" 0 0 0;
      setAttr "group3.sp" -type "double3" 0 0 0;
      // Drive group2.offsetParentMatrix by other node
      createNode "composeMatrix";
      connectAttr -f composeMatrix1.outputMatrix group2.offsetParentMatrix;
      currentTime 1;
      setAttr "composeMatrix1.inputTranslate" 1 1 1; setKeyframe "composeMatrix1.inputTranslate";
      setAttr "composeMatrix1.inputRotate" 10 10 10 ;   setKeyframe "composeMatrix1.inputRotate";
      setAttr "composeMatrix1.inputScale" 1 1 1;     setKeyframe "composeMatrix1.inputScale";
      currentTime 2;
      setAttr "composeMatrix1.inputTranslate" 2 2 2; setKeyframe "composeMatrix1.inputTranslate";
      setAttr "composeMatrix1.inputRotate" 20 20 20 ;   setKeyframe "composeMatrix1.inputRotate";
      setAttr "composeMatrix1.inputScale"  2 2 2;     setKeyframe "composeMatrix1.inputScale";
      currentTime 3;
      setAttr "composeMatrix1.inputTranslate" 3 3 3; setKeyframe "composeMatrix1.inputTranslate";
      setAttr "composeMatrix1.inputRotate" 30 30 30 ;   setKeyframe "composeMatrix1.inputRotate";
      setAttr "composeMatrix1.inputScale"  3 3 3;     setKeyframe "composeMatrix1.inputScale";
      // Animate the local matrix
      currentTime 1;
      setAttr "pCube1.tx" 1; setKeyframe "pCube1.tx";
      setAttr "pCube1.ty" 2; setKeyframe "pCube1.ty";
      setAttr "pCube1.tz" 3; setKeyframe "pCube1.tz";
      currentTime 2;
      setAttr "pCube1.tx" 4; setKeyframe "pCube1.tx";
      setAttr "pCube1.ty" 5; setKeyframe "pCube1.ty";
      setAttr "pCube1.tz" 6; setKeyframe "pCube1.tz";
      currentTime 3;
      setAttr "pCube1.tx" 7; setKeyframe  "pCube1.tx";
      setAttr "pCube1.ty" 8; setKeyframe  "pCube1.ty";
      setAttr "pCube1.tz" 9; setKeyframe  "pCube1.tz";
      currentTime 1;
      )";

    ASSERT_TRUE(MGlobal::executeCommand(buildCommand));

    auto    path = buildTempPath("AL_USDMayaTests_withOffsetParentMatrix.usda");
    MString exportCommand = "file -force -options "
                            "\""
                            "Animation=1;"
                            "Use_Timeline_Range=0;Frame_Min=1;Frame_Max=3;"
                            "Merge_Offset_Parent_Matrix=1;"
                            "Export_In_World_Space=0;"
                            "Merge_Transforms=1;"
                            "Dynamic_Attributes=0;Mesh_Normals=0;"
                            "Mesh_Vertex_Creases=0;Mesh_Edge_Creases=0;"
                            "Mesh_UVs=0;Mesh_UV_Only=0;Mesh_Points_as_PRef=0;"
                            "Mesh_Colours=0;Mesh_Holes=0;Compaction_Level=0;"
                            "Nurbs_Curves=0;Duplicate_Instances=0;"
                            "Sub_Samples=1;Filter_Sample=0;Export_At_Which_Time=2;"
                            "Meshes=1;Mesh_Face_Connects=1;Mesh_Points=1;"
                            "\" -typ \"AL usdmaya export\" -pr -ea ";
    exportCommand += "\"";
    exportCommand += path;
    exportCommand += "\"";

    // export cube in world space
    ASSERT_TRUE(MGlobal::executeCommand(exportCommand));

    auto stage = UsdStage::Open(path);

    ASSERT_TRUE(stage);
    bool resetsXformStack = false;

    // group3 has static xform matrix, translate == (1, 2, 3)
    {
        auto prim = stage->GetPrimAtPath(SdfPath("/group3"));
        ASSERT_TRUE(prim);
        UsdGeomXform xform(prim);
        GfMatrix4d   transform(1.0);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        // make sure the local space tm values match the world coords.
        EXPECT_NEAR(1.0, transform[0][0], 1e-6);
        EXPECT_NEAR(0.0, transform[0][1], 1e-6);
        EXPECT_NEAR(0.0, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(0.0, transform[1][0], 1e-6);
        EXPECT_NEAR(1.0, transform[1][1], 1e-6);
        EXPECT_NEAR(0.0, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(0.0, transform[2][0], 1e-6);
        EXPECT_NEAR(0.0, transform[2][1], 1e-6);
        EXPECT_NEAR(1.0, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(1.0, transform[3][0], 1e-6);
        EXPECT_NEAR(2.0, transform[3][1], 1e-6);
        EXPECT_NEAR(3.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
    }

    // group2 offset parent matrix is driven by connection
    {
        auto prim = stage->GetPrimAtPath(SdfPath("/group3/group2"));
        ASSERT_TRUE(prim);

        UsdGeomXform xform(prim);
        GfMatrix4d   transform(1.0);

        // group2 at frame 1:
        // translate: 2, 3, 4
        // rotate xyz: 10, 10, 10
        // scale: 1, 1, 1
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode(1));
        EXPECT_NEAR(0.9698463103929541, transform[0][0], 1e-6);
        EXPECT_NEAR(0.17101007166283433, transform[0][1], 1e-6);
        EXPECT_NEAR(-0.17364817766693033, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(-0.14131448435589197, transform[1][0], 1e-6);
        EXPECT_NEAR(0.9750824436431519, transform[1][1], 1e-6);
        EXPECT_NEAR(0.17101007166283433, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(0.19856573402377836, transform[2][0], 1e-6);
        EXPECT_NEAR(-0.141314484355892, transform[2][1], 1e-6);
        EXPECT_NEAR(0.9698463103929541, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(2.0, transform[3][0], 1e-6);
        EXPECT_NEAR(3.0, transform[3][1], 1e-6);
        EXPECT_NEAR(4.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
        // group2 at frame 2:
        // translate: 3, 4, 5
        // rotate xyz: 20, 20, 20
        // scale: 2, 2, 2
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode(2));
        EXPECT_NEAR(1.7660444431189781, transform[0][0], 1e-6);
        EXPECT_NEAR(0.6427876096865395, transform[0][1], 1e-6);
        EXPECT_NEAR(-0.6840402866513375, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(-0.42294129929358526, transform[1][0], 1e-6);
        EXPECT_NEAR(1.8460619562152618, transform[1][1], 1e-6);
        EXPECT_NEAR(0.6427876096865395, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(0.8379783304360758, transform[2][0], 1e-6);
        EXPECT_NEAR(-0.4229412992935852, transform[2][1], 1e-6);
        EXPECT_NEAR(1.7660444431189781, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(3.0, transform[3][0], 1e-6);
        EXPECT_NEAR(4.0, transform[3][1], 1e-6);
        EXPECT_NEAR(5.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
        // group2 at frame 3:
        // translate: 4, 5, 6
        // rotate xyz: 30, 30, 30
        // scale: 3, 3, 3
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode(3));
        EXPECT_NEAR(2.2500000000000004, transform[0][0], 1e-6);
        EXPECT_NEAR(1.299038105676658, transform[0][1], 1e-6);
        EXPECT_NEAR(-1.4999999999999998, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(-0.649519052838329, transform[1][0], 1e-6);
        EXPECT_NEAR(2.6250000000000004, transform[1][1], 1e-6);
        EXPECT_NEAR(1.299038105676658, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(1.875, transform[2][0], 1e-6);
        EXPECT_NEAR(-0.649519052838329, transform[2][1], 1e-6);
        EXPECT_NEAR(2.2500000000000004, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(4.0, transform[3][0], 1e-6);
        EXPECT_NEAR(5.0, transform[3][1], 1e-6);
        EXPECT_NEAR(6.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
    }
    // group1 local matrix and offset parent matrix are both static at
    //  local translate: 1, 2, 3
    // offset translate: 1, 2, 3
    // offset    rotate: 30, 45, 60
    {
        auto prim = stage->GetPrimAtPath(SdfPath("/group3/group2/group1"));
        ASSERT_TRUE(prim);

        UsdGeomXform xform(prim);
        GfMatrix4d   transform(1.0);
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode::EarliestTime());
        EXPECT_NEAR(0.3535533905932738, transform[0][0], 1e-6);
        EXPECT_NEAR(0.6123724356957945, transform[0][1], 1e-6);
        EXPECT_NEAR(-0.7071067811865476, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(-0.5732233047033631, transform[1][0], 1e-6);
        EXPECT_NEAR(0.7391989197401168, transform[1][1], 1e-6);
        EXPECT_NEAR(0.3535533905932737, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(0.7391989197401165, transform[2][0], 1e-6);
        EXPECT_NEAR(0.2803300858899107, transform[2][1], 1e-6);
        EXPECT_NEAR(0.6123724356957945, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(2.0, transform[3][0], 1e-6);
        EXPECT_NEAR(4.0, transform[3][1], 1e-6);
        EXPECT_NEAR(6.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
    }
    // cube1 local translate is animated, offset parent matrix is static
    //  local translate: 1, 2, 3 -> 4, 5, 6 -> 7, 8, 9
    // offset translate: 1, 2, 3
    {
        auto prim = stage->GetPrimAtPath(SdfPath("/group3/group2/group1/pCube1"));
        ASSERT_TRUE(prim);

        UsdGeomXform xform(prim);
        GfMatrix4d   transform(1.0);
        // frame 1: local translate: 1, 2, 3
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode(1));
        EXPECT_NEAR(1.0, transform[0][0], 1e-6);
        EXPECT_NEAR(0.0, transform[0][1], 1e-6);
        EXPECT_NEAR(0.0, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(0.0, transform[1][0], 1e-6);
        EXPECT_NEAR(1.0, transform[1][1], 1e-6);
        EXPECT_NEAR(0.0, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(0.0, transform[2][0], 1e-6);
        EXPECT_NEAR(0.0, transform[2][1], 1e-6);
        EXPECT_NEAR(1.0, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(2.0, transform[3][0], 1e-6);
        EXPECT_NEAR(4.0, transform[3][1], 1e-6);
        EXPECT_NEAR(6.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
        // frame 2: local translate: 4, 5, 6
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode(2));
        EXPECT_NEAR(1.0, transform[0][0], 1e-6);
        EXPECT_NEAR(0.0, transform[0][1], 1e-6);
        EXPECT_NEAR(0.0, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(0.0, transform[1][0], 1e-6);
        EXPECT_NEAR(1.0, transform[1][1], 1e-6);
        EXPECT_NEAR(0.0, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(0.0, transform[2][0], 1e-6);
        EXPECT_NEAR(0.0, transform[2][1], 1e-6);
        EXPECT_NEAR(1.0, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(5.0, transform[3][0], 1e-6);
        EXPECT_NEAR(7.0, transform[3][1], 1e-6);
        EXPECT_NEAR(9.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
        // frame 3: local translate: 7, 8, 9
        xform.GetLocalTransformation(&transform, &resetsXformStack, UsdTimeCode(3));
        EXPECT_NEAR(1.0, transform[0][0], 1e-6);
        EXPECT_NEAR(0.0, transform[0][1], 1e-6);
        EXPECT_NEAR(0.0, transform[0][2], 1e-6);
        EXPECT_NEAR(0.0, transform[0][3], 1e-6);
        EXPECT_NEAR(0.0, transform[1][0], 1e-6);
        EXPECT_NEAR(1.0, transform[1][1], 1e-6);
        EXPECT_NEAR(0.0, transform[1][2], 1e-6);
        EXPECT_NEAR(0.0, transform[1][3], 1e-6);
        EXPECT_NEAR(0.0, transform[2][0], 1e-6);
        EXPECT_NEAR(0.0, transform[2][1], 1e-6);
        EXPECT_NEAR(1.0, transform[2][2], 1e-6);
        EXPECT_NEAR(0.0, transform[2][3], 1e-6);
        EXPECT_NEAR(8.0, transform[3][0], 1e-6);
        EXPECT_NEAR(10.0, transform[3][1], 1e-6);
        EXPECT_NEAR(12.0, transform[3][2], 1e-6);
        EXPECT_NEAR(1.0, transform[3][3], 1e-6);
    }
}