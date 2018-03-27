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

#include "AL/maya/utils/Utils.h"
#include "test_usdmaya.h"
#include "maya/MGlobal.h"
#include "maya/MFileIO.h"
#include "maya/MFnDagNode.h"

TEST(ExportCommands, exportUV)
{
  MFileIO::newFile(true);

  const std::string temp_path = "/tmp/AL_USDMayaTests_exportUV.usda";
  MGlobal::executeCommand(MString("createNode transform -n geo;polyCube -n cube -cuv 2;parent cube geo;select geo"), false, true);
  MString exportCmd;
  exportCmd.format(MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -muv 1 -luv 1"), AL::maya::utils::convert(temp_path));
  MGlobal::executeCommand(exportCmd, true);

  UsdStageRefPtr stage = UsdStage::Open(temp_path);
  UsdPrim geoPrim = stage->GetPrimAtPath(SdfPath("/geo"));
  ASSERT_TRUE(geoPrim.IsValid());
  ASSERT_EQ(geoPrim.GetSpecifier(), SdfSpecifierOver);

  UsdPrim cubePrim = stage->GetPrimAtPath(SdfPath("/geo/cube"));
  ASSERT_TRUE(cubePrim.IsValid());
  ASSERT_EQ(cubePrim.GetSpecifier(), SdfSpecifierOver);

  const uint32_t uvSZ = 24;
  const GfVec2f faceUVs[4] = {GfVec2f(.0f, 1.0f), GfVec2f(1.0f, 1.0f), GfVec2f(1.0f, .0f), GfVec2f(.0f, .0f)};

  UsdAttribute stAttr = cubePrim.GetAttribute(TfToken("primvars:st"));
  ASSERT_TRUE(stAttr.IsValid());
  VtArray<GfVec2f> uvs;
  stAttr.Get(&uvs);
  ASSERT_EQ(uvs.size(), uvSZ);

  UsdGeomPrimvar primVar(stAttr);
  ASSERT_TRUE(primVar.IsIndexed());
  VtIntArray indices;
  primVar.GetIndices(&indices);
  ASSERT_EQ(indices.size(), uvSZ);

  for (uint32_t i = 0; i < uvSZ; ++i)
  {
    const GfVec2f& uv =  uvs[indices[i]];
    const GfVec2f& faceUV = faceUVs[i % 4];
    ASSERT_FLOAT_EQ(uv[0], faceUV[0]);
    ASSERT_FLOAT_EQ(uv[1], faceUV[1]);
  }
}

TEST(ExportCommands, extensiveAnimationCheck)
{
  MFileIO::newFile(true);
  MStatus status;

  MFnDagNode transformFN;
  MObject parent = transformFN.create("transform", MObject::kNullObj, &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MObject child = transformFN.create("transform", parent, &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  MObject master = transformFN.create("transform", MObject::kNullObj, &status);
  EXPECT_EQ(MStatus(MS::kSuccess), status);

  transformFN.setObject(master);
  MPlug sourceTx = transformFN.findPlug("translateX");

  MDGModifier mod;
  transformFN.setObject(parent);
  MPlug targetTx = transformFN.findPlug("translateX");

  EXPECT_EQ(MStatus(MS::kSuccess), mod.connect(sourceTx,targetTx));
  mod.doIt();

  transformFN.setObject(child);
  MString name = transformFN.name(&status);

  MSelectionList sel;
  sel.add(transformFN.dagPath(&status));
  MGlobal::setActiveSelectionList(sel);

  const std::string temp_path = "/tmp/AL_USDMayaTests_extensiveAnimationCheck.usda";
  MString exportCmd;

  auto expectAnimation = [temp_path, name] (bool expectAnimation)
  {
    UsdStageRefPtr stage = UsdStage::Open(temp_path);
    EXPECT_TRUE(stage);

    UsdPrim prim = stage->GetPrimAtPath(SdfPath(name.asChar()));
    EXPECT_TRUE(prim.IsValid());

    UsdGeomXform transform(prim);

    bool resetsXformStack;
    std::vector<UsdGeomXformOp> ops = transform.GetOrderedXformOps(&resetsXformStack);
    if(expectAnimation)
    {
      EXPECT_FALSE(ops.empty());
      for(auto op : ops)
      {
        auto attr = op.GetAttr();
        EXPECT_EQ(10, attr.GetNumTimeSamples());
      }
    }
    else
    {
      EXPECT_TRUE(ops.empty());
    }
  };

  exportCmd.format(MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -animation 1 -frameRange 1 10"), AL::maya::utils::convert(temp_path));
  MGlobal::executeCommand(exportCmd, true);

  expectAnimation(true);

  exportCmd.format(MString("AL_usdmaya_ExportCommand -f \"^1s\" -sl 1 -animation 1 -extensiveAnimationCheck 0 -frameRange 1 10"), AL::maya::utils::convert(temp_path));
  MGlobal::executeCommand(exportCmd, true);
  expectAnimation(false);


  mod.deleteNode(master);
  mod.deleteNode(child);
  mod.deleteNode(parent);
  mod.doIt();
}
