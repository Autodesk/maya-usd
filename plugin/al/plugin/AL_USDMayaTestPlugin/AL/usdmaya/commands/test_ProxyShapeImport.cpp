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
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>
#include <maya/MUuid.h>

#include <functional>

using AL::maya::test::buildTempPath;

TEST(ProxyShapeImport, populationMaskInclude)
{
    auto constructTestUSDFile = []() {
        const std::string temp_bootstrap_path
            = buildTempPath("AL_USDMayaTests_populationMaskInclude.usda");

        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));

        UsdPrim leg1 = stage->DefinePrim(SdfPath("/root/hip1"), TfToken("xform"));
        UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee"));

        UsdGeomXform::Define(stage, SdfPath("/root/hip2"));
        UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee"));

        UsdGeomXform::Define(stage, SdfPath("/root/hip3"));
        UsdGeomXform::Define(stage, SdfPath("/root/hip3/knee"));

        SdfPath materialPath = SdfPath("/root/material");
        UsdPrim material = stage->DefinePrim(materialPath, TfToken("xform"));
        auto    relation = leg1.CreateRelationship(TfToken("material"), true);
        relation.AddTarget(materialPath);

        stage->Export(temp_bootstrap_path, false);
        return MString(temp_bootstrap_path.c_str());
    };

    auto constructMaskTestCommand = [](const MString& bootstrap_path, const MString& mask) {
        MString cmd = "AL_usdmaya_ProxyShapeImport -file \"";
        cmd += bootstrap_path;
        cmd += "\" -populationMaskInclude \"";
        cmd += mask;
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

    auto assertSdfPathIsInvalid = [](UsdStageRefPtr usdStage, const std::string& path) {
        EXPECT_FALSE(usdStage->GetPrimAtPath(SdfPath(path)).IsValid());
    };

    MString bootstrap_path = constructTestUSDFile();

    // Test no mask:
    MFileIO::newFile(true);
    MGlobal::executeCommand(constructMaskTestCommand(bootstrap_path, ""), false, true);
    auto stage = getStageFromCache();
    ASSERT_TRUE(stage);
    assertSdfPathIsValid(stage, "/root");
    assertSdfPathIsValid(stage, "/root/hip1/knee");
    assertSdfPathIsValid(stage, "/root/hip2/knee");
    assertSdfPathIsValid(stage, "/root/hip3/knee");
    assertSdfPathIsValid(stage, "/root/material");

    // Test single mask:
    MFileIO::newFile(true);
    MGlobal::executeCommand(constructMaskTestCommand(bootstrap_path, "/root/hip2"), false, true);
    stage = getStageFromCache();
    ASSERT_TRUE(stage);
    assertSdfPathIsValid(stage, "/root");
    assertSdfPathIsInvalid(stage, "/root/hip1/knee");
    assertSdfPathIsValid(stage, "/root/hip2/knee");
    assertSdfPathIsInvalid(stage, "/root/hip3/knee");
    assertSdfPathIsInvalid(stage, "/root/material");

    // Test multiple  masks:
    MFileIO::newFile(true);
    MGlobal::executeCommand(
        constructMaskTestCommand(bootstrap_path, "/root/hip2/knee,/root/hip3"), false, true);
    stage = getStageFromCache();
    ASSERT_TRUE(stage);
    assertSdfPathIsValid(stage, "/root");
    assertSdfPathIsInvalid(stage, "/root/hip1/knee");
    assertSdfPathIsValid(stage, "/root/hip2/knee");
    assertSdfPathIsValid(stage, "/root/hip3/knee");
    assertSdfPathIsInvalid(stage, "/root/material");

    // Test relation expansion:
    MFileIO::newFile(true);
    MGlobal::executeCommand(constructMaskTestCommand(bootstrap_path, "/root/hip1"), false, true);
    stage = getStageFromCache();
    ASSERT_TRUE(stage);
    assertSdfPathIsValid(stage, "/root");
    assertSdfPathIsValid(stage, "/root/hip1/knee");
    assertSdfPathIsInvalid(stage, "/root/hip2/knee");
    assertSdfPathIsValid(stage, "/root/material");
}

TEST(ProxyShapeImport, lockMetaData)
{
    MFileIO::newFile(true);
    const std::string temp_bootstrap_path = buildTempPath("AL_USDMayaTests_lockMetaData.usda");
    auto              constructTestUSDFile = [temp_bootstrap_path]() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));

        UsdPrim       geo = stage->DefinePrim(SdfPath("/root/geo"), TfToken("xform"));
        const TfToken lockMetadata("al_usdmaya_lock");
        geo.SetMetadata(lockMetadata, TfToken("transform"));

        UsdPrim cam = stage->DefinePrim(SdfPath("/root/geo/cam"), TfToken("Camera"));

        stage->Export(temp_bootstrap_path, false);
        return MString(temp_bootstrap_path.c_str());
    };

    constructTestUSDFile();

    MFileIO::newFile(true);
    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_bootstrap_path.c_str());

    auto assertSdfPathIsValid = [](UsdStageRefPtr usdStage, const std::string& path) {
        EXPECT_TRUE(usdStage->GetPrimAtPath(SdfPath(path)).IsValid());
    };

    auto stage = proxy->getUsdStage();
    ASSERT_TRUE(stage);
    assertSdfPathIsValid(stage, "/root");
    assertSdfPathIsValid(stage, "/root/geo");
    assertSdfPathIsValid(stage, "/root/geo/cam");
    UsdPrim geoPrim = stage->GetPrimAtPath(SdfPath("/root/geo"));

    MStatus        status;
    MSelectionList sl;
    MObject        camObj;
    EXPECT_TRUE(sl.add("cam") == MS::kSuccess);
    EXPECT_TRUE(sl.getDependNode(0, camObj) == MS::kSuccess);
    ASSERT_FALSE(camObj.isNull());

    MFnDependencyNode fndd(camObj);
    MUuid             iid = fndd.uuid();

    MPlug pushToPlugPrim(camObj, AL::usdmaya::nodes::Transform::pushToPrim());
    EXPECT_FALSE(pushToPlugPrim.asBool());

    MFnDependencyNode camDG(camObj, &status);
    EXPECT_TRUE(status == MS::kSuccess);

    MPlug tPlug = camDG.findPlug("t", &status);
    EXPECT_TRUE(status == MS::kSuccess);

    MPlug rPlug = camDG.findPlug("r", &status);
    EXPECT_TRUE(status == MS::kSuccess);

    MPlug sPlug = camDG.findPlug("s", &status);
    EXPECT_TRUE(status == MS::kSuccess);
    EXPECT_TRUE(tPlug.isLocked());
    EXPECT_TRUE(rPlug.isLocked());
    EXPECT_TRUE(sPlug.isLocked());

    status = MGlobal::executeCommand("setAttr cam.t 5 5 5");
    EXPECT_TRUE(status != MStatus::kSuccess);
    status = MGlobal::executeCommand("setAttr cam.r 5 5 5");
    EXPECT_TRUE(status != MStatus::kSuccess);
    status = MGlobal::executeCommand("setAttr cam.s 5 5 5");
    EXPECT_TRUE(status != MStatus::kSuccess);
}

TEST(ProxyShapeImport, sessionLayer)
{
    constexpr double EPSILON = 1e-5;
    constexpr auto   SESSION_LAYER_CONTENTS = R"ESC(#sdf 1.4.32
over "root" {
  float3 xformOp:translate = (1.2, 2.3, 3.4)
  uniform token[] xformOpOrder = ["xformOp:translate"]
})ESC";

    MFileIO::newFile(true);
    auto constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        return stage;
    };

    const std::string temp_path = buildTempPath("AL_USDMayaTests_ImportCommands_sessionLayer.usda");

    // generate our usda
    {
        auto stage = constructTransformChain();
        stage->Export(temp_path, false);
    }

    // Bring it in with no session layer, assert that no translate x
    MFileIO::newFile(true);
    {
        MString importCmd;
        importCmd.format(
            MString("AL_usdmaya_ProxyShapeImport -file \"^1s\""),
            AL::maya::utils::convert(temp_path));
        MGlobal::executeCommand(importCmd);
        MGlobal::executeCommand("AL_usdmaya_ProxyShapeImportAllTransforms AL_usdmaya_Proxy;");

        MSelectionList sel;
        sel.add("root");
        MObject rootObj;
        sel.getDependNode(0, rootObj);
        ASSERT_FALSE(rootObj.isNull());
        MFnTransform rootFn(rootObj);

        MVector translation = rootFn.getTranslation(MSpace::kObject);
        EXPECT_EQ(0.0, translation.x);
        EXPECT_EQ(0.0, translation.y);
        EXPECT_EQ(0.0, translation.z);
    }

    // Now repeat, but bring in with a session layer that sets a tx
    MFileIO::newFile(true);
    {
        MString importCmd;

        // First do a janky mel-encode of our session layer string...
        std::string encodedString1 = TfStringReplace(SESSION_LAYER_CONTENTS, "\"", "\\\"");
        std::string encodedString2 = TfStringReplace(encodedString1, "\n", "\\n");
        importCmd.format(
            MString("AL_usdmaya_ProxyShapeImport -file \"^1s\" -s \"^2s\""),
            AL::maya::utils::convert(temp_path),
            AL::maya::utils::convert(encodedString2));
        MGlobal::executeCommand(importCmd);
        MGlobal::executeCommand("AL_usdmaya_ProxyShapeImportAllTransforms AL_usdmaya_Proxy;");

        MSelectionList sel;
        sel.add("root");
        MObject rootObj;
        sel.getDependNode(0, rootObj);
        ASSERT_FALSE(rootObj.isNull());
        MFnTransform rootFn(rootObj);

        MVector translation = rootFn.getTranslation(MSpace::kObject);
        EXPECT_NEAR(1.2, translation.x, EPSILON);
        EXPECT_NEAR(2.3, translation.y, EPSILON);
        EXPECT_NEAR(3.4, translation.z, EPSILON);
    }
}

TEST(ProxyShapeImport, stageLoadWithCacheId)
{
    auto createTestUSDStage = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        stage->DefinePrim(SdfPath("/root/parent"), TfToken("xform"));
        return stage;
    };

    const MString proxyName = "testProxy";
    auto          constructProxyImportCommand = [proxyName](int stageId) {
        MString cmd = "AL_usdmaya_ProxyShapeImport ";
        cmd += " -name \"" + proxyName + "\"";
        cmd += MString(" -stageId ") + stageId;
        return cmd;
    };

    MFileIO::newFile(true);

    auto stage = createTestUSDStage();
    auto stageCacheId = AL::usdmaya::StageCache::Get().Insert(stage);

    EXPECT_TRUE(stageCacheId.IsValid());
    ASSERT_TRUE(
        MGlobal::executeCommand(constructProxyImportCommand(stageCacheId.ToLongInt()), false, false)
        == MS::kSuccess);

    MSelectionList sel;
    EXPECT_TRUE(sel.add(proxyName + "Shape") == MS::kSuccess);
    MObject proxyShapeObj;
    EXPECT_TRUE(sel.getDependNode(0, proxyShapeObj) == MS::kSuccess);

    MStatus           stat;
    MFnDependencyNode fn(proxyShapeObj, &stat);
    EXPECT_TRUE(stat);
    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode(&stat);
    ASSERT_TRUE(stat);

    auto proxyStage = proxy->getUsdStage();
    auto proxyStageCacheId = AL::usdmaya::StageCache::Get().GetId(proxyStage);

    ASSERT_EQ(stageCacheId.ToLongInt(), proxyStageCacheId.ToLongInt());
}

TEST(ProxyShapeImport, stageLoadAndChangeFilePath)
{
    MFileIO::newFile(true);
    auto createNewStageInMemory = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdGeomXform   root = UsdGeomXform::Define(stage, SdfPath("/root"));
        return stage;
    };

    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_ImportCommands_changeFilePath.usda");

    // Export stage to disk to load later.
    {
        auto stage = createNewStageInMemory();
        stage->Export(temp_path, false);
    }

    MFileIO::newFile(true);
    auto    stage = createNewStageInMemory();
    auto    stageCacheId = AL::usdmaya::StageCache::Get().Insert(stage);
    MString importCmd;

    // Import stage using cache Id.
    importCmd.format(
        MString("AL_usdmaya_ProxyShapeImport -stageId ") + stageCacheId.ToString().c_str());
    MGlobal::executeCommand(importCmd);

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    auto preFilePathUpdateStage = proxy->getUsdStage();
    auto preFilePathUpdatePath = proxy->filePathPlug().asString();

    // Force the proxy to load another stage from disk.
    proxy->filePathPlug().setString(temp_path.c_str());

    auto postFilePathUpdateStage = proxy->getUsdStage();
    auto postFilePathUpdatePath = proxy->filePathPlug().asString();

    EXPECT_NE(preFilePathUpdateStage, postFilePathUpdateStage);
    EXPECT_NE(preFilePathUpdatePath, postFilePathUpdatePath);
    ASSERT_EQ(AL::maya::utils::convert(temp_path), postFilePathUpdatePath);
}

TEST(ProxyShapeImport, layerManagerTracksCurrentEditTargetWhenLoadingFromStageCacheId)
{
    MFileIO::newFile(true);
    UsdStageRefPtr    stage = UsdStage::CreateInMemory();
    UsdStageCache::Id stageCacheId = AL::usdmaya::StageCache::Get().Insert(stage);

    // Current edit target that will be tracked.
    auto targetLayer = stage->GetEditTarget().GetLayer();

    // Import stage using cache Id.
    MString importCmd = MString("AL_usdmaya_ProxyShapeImport -stageId ")
        + AL::maya::utils::convert(stageCacheId.ToString());
    MStatus status = MGlobal::executeCommand(importCmd);
    ASSERT_TRUE(status);

    // After creating proxy, we should have a layerManager.
    AL::usdmaya::nodes::LayerManager* layerManager
        = AL::usdmaya::nodes::LayerManager::findManager();
    ASSERT_TRUE(layerManager);

    // Make an edit to dirty the current edit target, for layerManager to acknowledge the tracked
    // layer.
    stage->DefinePrim(SdfPath("/test"));

    auto trackedLayer = layerManager->findLayer(targetLayer->GetIdentifier());
    ASSERT_TRUE(trackedLayer);
}

TEST(ProxyShapeImport, layerManagerTracksAllDirtyLayersWhenLoadingFromStageCacheId)
{
    MFileIO::newFile(true);
    UsdStageRefPtr    stage = UsdStage::CreateInMemory();
    UsdStageCache::Id stageCacheId = AL::usdmaya::StageCache::Get().Insert(stage);

    auto rootLayer = stage->GetRootLayer();
    auto sessionLayer = stage->GetSessionLayer();
    // Create a sub layer to make sure non-dirty layers are not being tracked.
    auto subLayer = SdfLayer::CreateAnonymous("test_sub_layer");

    rootLayer->InsertSubLayerPath(subLayer->GetIdentifier());

    // Set target and make an edit on root layer.
    stage->SetEditTarget(UsdEditTarget(rootLayer));
    auto testPrim = UsdGeomXform::Define(stage, SdfPath("/test")).GetPrim();

    // Override testPrim in session layer.
    stage->SetEditTarget(UsdEditTarget(sessionLayer));
    testPrim.GetAttribute(UsdGeomTokens->visibility).Set(UsdGeomTokens->invisible);

    // Import stage using cache Id.
    MString importCmd = MString("AL_usdmaya_ProxyShapeImport -stageId ")
        + AL::maya::utils::convert(stageCacheId.ToString());
    MStatus status = MGlobal::executeCommand(importCmd);
    ASSERT_TRUE(status);

    // After creating proxy, we should have a layerManager.
    AL::usdmaya::nodes::LayerManager* layerManager
        = AL::usdmaya::nodes::LayerManager::findManager();
    ASSERT_TRUE(layerManager);

    MStringArray trackedLayerIds;
    layerManager->getLayerIdentifiers(trackedLayerIds);
    ASSERT_EQ(trackedLayerIds.length(), 2u);

    auto trackedLayer = layerManager->findLayer(rootLayer->GetIdentifier());
    ASSERT_TRUE(trackedLayer);

    trackedLayer = layerManager->findLayer(sessionLayer->GetIdentifier());
    ASSERT_TRUE(trackedLayer);

    trackedLayer = layerManager->findLayer(subLayer->GetIdentifier());
    ASSERT_FALSE(trackedLayer);
}
