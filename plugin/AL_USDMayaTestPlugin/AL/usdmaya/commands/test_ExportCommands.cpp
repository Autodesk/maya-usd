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
