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
            TransformTranslator::copyAttributes(node, prim, eparams, fn.dagPath()));

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
            TransformTranslator::copyAttributes(node, prim, eparams, fn.dagPath()));
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
