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

#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/Utils.h"
#include "pxr/base/tf/stringUtils.h"
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MFileIO.h"

#include <functional>

TEST(ProxyShapeImport, populationMaskInclude)
{
  auto  constructTestUSDFile = [] ()
  {
    const std::string temp_bootstrap_path = "/tmp/AL_USDMayaTests_populationMaskInclude.usda";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));

    UsdPrim leg1 = stage->DefinePrim(SdfPath("/root/hip1"), TfToken("xform"));
    UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee"));

    UsdGeomXform::Define(stage, SdfPath("/root/hip2"));
    UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee"));

    UsdGeomXform::Define(stage, SdfPath("/root/hip3"));
    UsdGeomXform::Define(stage, SdfPath("/root/hip3/knee"));

    SdfPath materialPath = SdfPath("/root/material");
    UsdPrim material = stage->DefinePrim(materialPath, TfToken("xform"));
    auto relation = leg1.CreateRelationship(TfToken("material"), true);
    relation.AddTarget(materialPath);

    stage->Export(temp_bootstrap_path, false);
    return MString(temp_bootstrap_path.c_str());
  };

  auto constructMaskTestCommand = [] (const MString &bootstrap_path, const MString &mask)
  {
    MString cmd = "AL_usdmaya_ProxyShapeImport -file \"";
    cmd += bootstrap_path;
    cmd += "\" -populationMaskInclude \"";
    cmd += mask;
    cmd += "\"";
    return cmd;
  };

  auto getStageFromCache = [] ()
  {
    auto usdStageCache = AL::usdmaya::StageCache::Get();
    if(usdStageCache.IsEmpty())
    {
      return UsdStageRefPtr();
    }
    return usdStageCache.GetAllStages()[0];
  };

  auto assertSdfPathIsValid = [] (UsdStageRefPtr usdStage, const std::string &path)
  {
    EXPECT_TRUE(usdStage->GetPrimAtPath(SdfPath(path)).IsValid());
  };

  auto assertSdfPathIsInvalid = [] (UsdStageRefPtr usdStage, const std::string &path)
  {
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
  MGlobal::executeCommand(constructMaskTestCommand(bootstrap_path, "/root/hip2/knee,/root/hip3"), false, true);
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
  auto  constructTestUSDFile = [] ()
  {
    const std::string temp_bootstrap_path = "/tmp/AL_USDMayaTests_lockMetaData.usda";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));

    UsdPrim geo = stage->DefinePrim(SdfPath("/root/geo"), TfToken("xform"));
    const TfToken lockMetadata("al_usdmaya_lock");
    geo.SetMetadata(lockMetadata, TfToken("transform"));

    UsdPrim cam = stage->DefinePrim(SdfPath("/root/geo/cam"), TfToken("Camera"));

    stage->Export(temp_bootstrap_path, false);
    return MString(temp_bootstrap_path.c_str());
  };

  auto constructLockMetaDataTestCommand = [] (const MString& bootstrap_path, const MString& mask)
  {
    MString cmd = "AL_usdmaya_ProxyShapeImport -file \"";
    cmd += bootstrap_path;
    cmd += "\"";
    return cmd;
  };

  auto getStageFromCache = [] ()
  {
    auto usdStageCache = AL::usdmaya::StageCache::Get();
    if(usdStageCache.IsEmpty())
    {
      return UsdStageRefPtr();
    }
    return usdStageCache.GetAllStages()[0];
  };

  auto assertSdfPathIsValid = [] (UsdStageRefPtr usdStage, const std::string &path)
  {
    EXPECT_TRUE(usdStage->GetPrimAtPath(SdfPath(path)).IsValid());
  };

  MString bootstrap_path = constructTestUSDFile();
  MFileIO::newFile(true);
  MGlobal::executeCommand(constructLockMetaDataTestCommand(bootstrap_path, ""), false, true);
  auto stage = getStageFromCache();
  ASSERT_TRUE(stage);
  assertSdfPathIsValid(stage, "/root");
  assertSdfPathIsValid(stage, "/root/geo");
  assertSdfPathIsValid(stage, "/root/geo/cam");
  UsdPrim geoPrim = stage->GetPrimAtPath(SdfPath("/root/geo"));

  MSelectionList sl;
  MObject camObj;
  sl.add("cam");
  sl.getDependNode(0, camObj);
  ASSERT_FALSE(camObj.isNull());
  ASSERT_FALSE(MPlug(camObj, AL::usdmaya::nodes::Transform::pushToPrim()).asBool());
  MFnDependencyNode camDG(camObj);
  MPlug tPlug = camDG.findPlug("t");
  MPlug rPlug = camDG.findPlug("r");
  MPlug sPlug = camDG.findPlug("s");
  ASSERT_TRUE(tPlug.isLocked());
  ASSERT_TRUE(rPlug.isLocked());
  ASSERT_TRUE(sPlug.isLocked());

  MStatus status = MGlobal::executeCommand("setAttr cam.t 5 5 5");
  ASSERT_TRUE(status == MStatus::kFailure);
  status = MGlobal::executeCommand("setAttr cam.r 5 5 5");
  ASSERT_TRUE(status == MStatus::kFailure);
  status = MGlobal::executeCommand("setAttr cam.s 5 5 5");
  ASSERT_TRUE(status == MStatus::kFailure);
}

TEST(ProxyShapeImport, sessionLayer)
{
  constexpr double EPSILON = 1e-5;
  constexpr auto SESSION_LAYER_CONTENTS = R"ESC(#sdf 1.4.32
over "root" {
  float3 xformOp:translate = (1.2, 2.3, 3.4)
  uniform token[] xformOpOrder = ["xformOp:translate"]
})ESC";

  MFileIO::newFile(true);
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_ImportCommands_sessionLayer.usda";

  // generate our usda
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  // Bring it in with no session layer, assert that no translate x
  MFileIO::newFile(true);
  {
    MString importCmd;
    importCmd.format(MString("AL_usdmaya_ProxyShapeImport -file \"^1s\""), AL::usdmaya::convert(temp_path));
    MGlobal::executeCommand(importCmd);
    MGlobal::executeCommand("AL_usdmaya_ProxyShapeImportAllTransforms AL_usdmaya_ProxyShape1;");

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
     importCmd.format(MString("AL_usdmaya_ProxyShapeImport -file \"^1s\" -s \"^2s\""),
         AL::usdmaya::convert(temp_path), AL::usdmaya::convert(encodedString2));
     MGlobal::executeCommand(importCmd);
     MGlobal::executeCommand("AL_usdmaya_ProxyShapeImportAllTransforms AL_usdmaya_ProxyShape1;");

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
