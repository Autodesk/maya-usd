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
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "test_usdmaya.h"

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>

using AL::maya::test::buildTempPath;
using AL::maya::test::compareNodes;
using AL::maya::test::randomAnimatedNode;
using AL::maya::test::randomNode;
using AL::usdmaya::fileio::AnimationTranslator;
using AL::usdmaya::fileio::ExporterParams;
using AL::usdmaya::fileio::ImporterParams;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the CameraTranslator.
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_CameraTranslator, io)
{
    for (int i = 0; i < 100; ++i) {
        MDagModifier mod, mod2;
        MObject      xform = mod.createNode("transform");
        MObject      node = mod.createNode("camera", xform);
        MObject      xformB = mod.createNode("transform");
        EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

        const char* const attributeNames[] = {
            "orthographic",         "horizontalFilmAperture",
            "verticalFilmAperture", "horizontalFilmOffset",
            "verticalFilmOffset",   "focalLength",
            "focusDistance",        "nearClipPlane",
            "farClipPlane",         "fStop",
            //"lensSqueezeRatio"
        };
        const uint32_t numAttributes = sizeof(attributeNames) / sizeof(const char* const);

        randomNode(node, attributeNames, numAttributes);

        // generate a prim for testing
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        ExporterParams eparams;
        ImporterParams iparams;
        SdfPath        cameraPath("/hello");
        MDagPath       nodeDagPath;
        MDagPath::getAPathTo(node, nodeDagPath);

        AL::usdmaya::fileio::translators::TranslatorManufacture manufacture(nullptr);
        AL::usdmaya::fileio::translators::TranslatorRefPtr xtrans = manufacture.getTranslatorFromId(
            AL::usdmaya::fileio::translators::TranslatorManufacture::TranslatorPrefixSchemaType
                .GetString()
            + TfToken("Camera").GetString());
        UsdPrim cameraPrim = xtrans->exportObject(stage, nodeDagPath, cameraPath, eparams);
        EXPECT_TRUE(cameraPrim.IsValid());
        MObject nodeB;
        EXPECT_EQ(MStatus(MS::kSuccess), xtrans->import(cameraPrim, xformB, nodeB));

        // now make sure the imported node matches the one we started with
        compareNodes(node, nodeB, attributeNames, numAttributes, true);

        mod2.deleteNode(nodeB);
        mod2.deleteNode(xformB);
        mod2.deleteNode(node);
        mod2.deleteNode(xform);
        mod2.doIt();
    }
}

TEST(translators_CameraTranslator, animated_io)
{
    const double startFrame = 1.0;
    const double endFrame = 20.0;

    for (int i = 0; i < 100; ++i) {
        MDagModifier mod;
        MObject      xform = mod.createNode("transform");
        MObject      node = mod.createNode("camera", xform);
        MObject      xformB = mod.createNode("transform");
        EXPECT_EQ(MStatus(MS::kSuccess), mod.doIt());

        // Note: separate keyable and non-keyable attributes to have a clear
        //       expectation; force setting keyframe for non-keyable attributes
        //       e.g. "nearClipPlane" and "farClipPlane" is necessary otherwise
        //       exporting animation would not going to work properly
        const char* const keyableAttributeNames[] = {
            "orthographic", "horizontalFilmAperture", "verticalFilmAperture",
            "focalLength",  "focusDistance",          "fStop",
            //"lensSqueezeRatio"
        };
        const uint32_t numKeyableAttributes
            = sizeof(keyableAttributeNames) / sizeof(const char* const);

        randomAnimatedNode(node, keyableAttributeNames, numKeyableAttributes, startFrame, endFrame);

        const char* const nonKeyableAttributeNames[] = {
            "horizontalFilmOffset",
            "verticalFilmOffset",
            "nearClipPlane",
            "farClipPlane",
        };
        const uint32_t numNonKeyableAttributes
            = sizeof(nonKeyableAttributeNames) / sizeof(const char* const);

        randomAnimatedNode(
            node, nonKeyableAttributeNames, numNonKeyableAttributes, startFrame, endFrame, true);

        // generate a prim for testing
        UsdStageRefPtr stage = UsdStage::CreateInMemory();

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Export animation
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        ExporterParams eparams;
        eparams.m_minFrame = startFrame;
        eparams.m_maxFrame = endFrame;
        eparams.m_animation = true;
        eparams.m_animTranslator = new AnimationTranslator;

        AL::usdmaya::fileio::translators::TranslatorManufacture manufacture(nullptr);
        AL::usdmaya::fileio::translators::TranslatorRefPtr xtrans = manufacture.getTranslatorFromId(
            AL::usdmaya::fileio::translators::TranslatorManufacture::TranslatorPrefixSchemaType
                .GetString()
            + TfToken("Camera").GetString());
        SdfPath  cameraPath("/hello");
        MDagPath nodeDagPath;
        MDagPath::getAPathTo(node, nodeDagPath);

        UsdPrim cameraPrim = xtrans->exportObject(stage, nodeDagPath, cameraPath, eparams);
        EXPECT_TRUE(cameraPrim.IsValid());
        eparams.m_animTranslator->exportAnimation(eparams);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Import animation
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        ImporterParams iparams;
        MObject        nodeB;
        EXPECT_EQ(MStatus(MS::kSuccess), xtrans->import(cameraPrim, xformB, nodeB));

        // now make sure the imported node matches the one we started with
        for (double t = eparams.m_minFrame, e = eparams.m_maxFrame + 1e-3f; t < e; t += 1.0) {
            MGlobal::viewFrame(t);
            compareNodes(node, nodeB, keyableAttributeNames, numKeyableAttributes, true);
            compareNodes(node, nodeB, nonKeyableAttributeNames, numNonKeyableAttributes, true);
        }

        EXPECT_EQ(MStatus(MS::kSuccess), mod.undoIt());
    }
}

TEST(translators_CameraTranslator, readAnimatedValues)
{
    constexpr auto LAYER_CONTENTS = R"ESC(#usda 1.0
(
    defaultPrim = "GEO"
    endTimeCode = 1000
    startTimeCode = 999
)
def Xform "GEO"
{
    float3 xformOp:translate = (-1.0, 2.5, 6.5)
    uniform token[] xformOpOrder = ["xformOp:translate"]
    def Xform "cameraGroup"
    {
        double3 xformOp:translate = (-0.0000015314200130234168, 1.587618925213975e-14, -1.4210854715201993e-14)
        uniform token[] xformOpOrder = ["xformOp:translate"]
        float3 xformOp:rotateZXY:rotate = (0.014807368, 358.9961, -0.03618303)
        float3 xformOp:rotateZXY:rotate.timeSamples = {
            999: (0.014807368, -1.003891, -0.03618303),
            1000: (0.11926156, -0.9200447, 0.12728521)
        }
        def Camera "renderCam"
        {
        }
    }
}
)ESC";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->GetRootLayer()->ImportFromString(LAYER_CONTENTS);

    SdfPath           currentPrimPath("/GEO/cameraGroup/renderCam");
    UsdPrim           currentPrim = stage->GetPrimAtPath(currentPrimPath);
    UsdStageCache::Id stageCacheId = AL::usdmaya::StageCache::Get().Insert(stage);
    SdfPath           cameraPrim("/GEO/cameraGroup/renderCam");

    // Load the stage into Maya
    EXPECT_TRUE(stageCacheId.IsValid());
    MString importCommand
        = MString("AL_usdmaya_ProxyShapeImport  ") + "-stageId " + stageCacheId.ToLongInt();

    MGlobal::executeCommand(importCommand);

    // Fetch relevant maya nodes
    MSelectionList mSel;
    mSel.add("|AL_usdmaya_Proxy|GEO|cameraGroup|renderCam");
    mSel.add("|AL_usdmaya_Proxy|GEO|cameraGroup");
    mSel.add("|AL_usdmaya_Proxy|GEO"); // Has transforms

    MDagPath dagPath1;
    mSel.getDagPath(0, dagPath1);
    MFnDagNode fnDagNode1(dagPath1.node());

    MDagPath dagPath2;
    mSel.getDagPath(1, dagPath2);
    MFnDagNode fnDagNode2(dagPath2.node());

    MDagPath dagPath3;
    mSel.getDagPath(2, dagPath3);
    MFnDagNode fnDagNode3(dagPath3.node());

    // Confrim read animated values is set True
    ASSERT_TRUE(fnDagNode1.findPlug("readAnimatedValues").asBool());
    ASSERT_TRUE(fnDagNode2.findPlug("readAnimatedValues").asBool());
    ASSERT_TRUE(fnDagNode3.findPlug("readAnimatedValues").asBool());

    // Validate the transforms on the |AL_usdmaya_Proxy|GEO dag path
    // matches our usd stage transforms.
    ASSERT_TRUE(fnDagNode3.findPlug("translateX").asFloat() == -1.0);
    ASSERT_TRUE(fnDagNode3.findPlug("translateY").asFloat() == 2.5);
    ASSERT_TRUE(fnDagNode3.findPlug("translateZ").asFloat() == 6.5);
}

TEST(translators_CameraTranslator, cameraShapeName)
{
    auto constructTestUSDFile = []() {
        const std::string temp_bootstrap_path = buildTempPath("AL_USDMayaTests_camShapeName.usda");

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        UsdPrim        geo = stage->DefinePrim(SdfPath("/root/geo"), TfToken("xform"));
        UsdPrim        cam = stage->DefinePrim(SdfPath("/root/geo/cam"), TfToken("Camera"));

        stage->Export(temp_bootstrap_path, false);
        return MString(temp_bootstrap_path.c_str());
    };

    auto constructCamTestCommand = [](const MString& bootstrap_path) {
        MString cmd = "AL_usdmaya_ProxyShapeImport -file \"";
        cmd += bootstrap_path;
        cmd += "\"";
        return cmd;
    };

    auto getStageFromCache = []() {
        auto usdStageCache = AL::usdmaya::StageCache::Get();
        if (usdStageCache.IsEmpty()) {
            return UsdStageRefPtr();
        }
        return usdStageCache.GetAllStages()[0];
    };

    auto assertSdfPathIsValid = [](UsdStageRefPtr usdStage, const std::string& path) {
        EXPECT_TRUE(usdStage->GetPrimAtPath(SdfPath(path)).IsValid());
    };

    MString bootstrap_path = constructTestUSDFile();
    MFileIO::newFile(true);
    MGlobal::executeCommand(constructCamTestCommand(bootstrap_path), false, true);
    auto stage = getStageFromCache();
    ASSERT_TRUE(stage);
    assertSdfPathIsValid(stage, "/root");
    assertSdfPathIsValid(stage, "/root/geo");
    assertSdfPathIsValid(stage, "/root/geo/cam");
    UsdPrim camPrim = stage->GetPrimAtPath(SdfPath("/root/geo/cam"));
    ASSERT_TRUE(camPrim.IsValid());
    ASSERT_EQ("Camera", camPrim.GetTypeName());

    MSelectionList sl;
    MObject        camObj;
    sl.add("cam");
    sl.getDependNode(0, camObj);
    ASSERT_FALSE(camObj.isNull());
    MFnDagNode camDag(camObj);
    ASSERT_EQ(MString("AL_usdmaya_Transform"), camDag.typeName());
    ASSERT_EQ(MString("cam"), camDag.name());
    ASSERT_EQ(1u, camDag.childCount());
    MFnDagNode shapeDag(camDag.child(0));
    ASSERT_EQ(MString("camera"), shapeDag.typeName());
    ASSERT_EQ(MString("camShape"), shapeDag.name());
}
