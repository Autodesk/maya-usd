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
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MFileIO.h"

#include <functional>

TEST(ProxyShapeImport, populationMaskInclude)
{
  MFileIO::newFile(true);

  auto  constructTestUSDFile = [] ()
  {
    const std::string temp_bootstrap_path = "/tmp/AL_USDMayaTests_proxyShapeImportTests.usda";

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));

    UsdPrim leg1 = stage->DefinePrim(SdfPath("/root/hip1"), TfToken("xform"));
    UsdGeomXform::Define(stage, SdfPath("/root/hip1/knee1"));

    UsdGeomXform::Define(stage, SdfPath("/root/hip2"));
    UsdGeomXform::Define(stage, SdfPath("/root/hip2/knee1"));

    UsdGeomXform::Define(stage, SdfPath("/root/hip3"));
    UsdGeomXform::Define(stage, SdfPath("/root/hip3/knee1"));

    SdfPath materialPath = SdfPath("/root/material");
    UsdPrim material = stage->DefinePrim(materialPath, TfToken("xform"));
    auto relation = leg1.CreateRelationship(TfToken("material"), true);
    relation.AppendTarget(materialPath);

    stage->Export(temp_bootstrap_path, false);
    return MString(temp_bootstrap_path.c_str());
  };

  auto constructTestCommand = [] (const MString &bootstrap_path, const MString &testFlags)
  {
    MString cmd = "AL_usdmaya_ProxyShapeImport -file \"";
    cmd += bootstrap_path;
    cmd += "\" ";
    cmd += testFlags;
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
  MGlobal::executeCommand(constructTestCommand(bootstrap_path, ""), false, true);
  auto stage = getStageFromCache();
  ASSERT_TRUE(stage);
  assertSdfPathIsValid(stage, "/root");
  assertSdfPathIsValid(stage, "/root/hip1/knee1");
  assertSdfPathIsValid(stage, "/root/hip2/knee2");
  assertSdfPathIsValid(stage, "/root/hip3/knee2");
  assertSdfPathIsValid(stage, "/root/material");

  // Test single mask:
  MString mask = "-populationMaskInclude \"/root/hip2\"";
  MGlobal::executeCommand(constructTestCommand(bootstrap_path, mask), false, true);
  stage = getStageFromCache();
  ASSERT_TRUE(stage);
  assertSdfPathIsValid(stage, "/root");
  assertSdfPathIsInvalid(stage, "/root/hip1/knee1");
  assertSdfPathIsValid(stage, "/root/hip2/knee2");
  assertSdfPathIsInvalid(stage, "/root/hip3/knee2");
  assertSdfPathIsInvalid(stage, "/root/material");

  mask = "-populationMaskInclude \"/root/hip2/knee1,/root/hip3\"";
  MGlobal::executeCommand(constructTestCommand(bootstrap_path, mask), false, true);
  stage = getStageFromCache();
  ASSERT_TRUE(stage);
  assertSdfPathIsValid(stage, "/root");
  assertSdfPathIsInvalid(stage, "/root/hip1/knee1");
  assertSdfPathIsValid(stage, "/root/hip2/knee2");
  assertSdfPathIsValid(stage, "/root/hip3/knee2");
  assertSdfPathIsInvalid(stage, "/root/material");

  // Test relation expansion:
  mask = "-populationMaskInclude \"/root/hip1\"";
  stage = getStageFromCache();
  ASSERT_TRUE(stage);
  assertSdfPathIsValid(stage, "/root");
  assertSdfPathIsValid(stage, "/root/hip1/knee1");
  assertSdfPathIsInvalid(stage, "/root/hip2/knee1");
  assertSdfPathIsValid(stage, "/root/material");
}
