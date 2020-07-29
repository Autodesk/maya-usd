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
