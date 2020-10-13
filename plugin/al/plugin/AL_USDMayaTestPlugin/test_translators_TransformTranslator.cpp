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
