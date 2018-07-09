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
#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/modelAPI.h"
#include "maya/MTime.h"

#include "AL/maya/utils/Utils.h"
#include "test_usdmaya.h"
#include "maya/MGlobal.h"
#include "maya/MFileIO.h"
#include "maya/MFnDagNode.h"

TEST(UsdMayaTranslators, exportProxyShapes)
{
  MFileIO::newFile(true);

  const std::string temp_path = buildTempPath("AL_USDMayaTests_exportProxyShape.usda");
  const std::string sphere2_path = std::string(AL_USDMAYA_TEST_DATA) + "/sphere2.usda";

  MGlobal::executeCommand(MString("createNode transform -n world;createNode transform -n geo -p world;select geo"), false, true);

  MSelectionList sl;
  MObject geoDepNode;
  sl.add("geo");
  sl.getDependNode(0, geoDepNode);
  MFnDagNode geoNode(geoDepNode);

  // create one proxyShape with a time offset
  MObject proxyParent;
  AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(nullptr, sphere2_path, &proxyParent);
  MObject proxyParentNode1 = MFnDagNode(proxyParent).parent(0);
  geoNode.addChild(proxyParentNode1);
  // force the stage to load
  UsdStageRefPtr stage = proxyShape->getUsdStage();
  ASSERT_TRUE(stage);

  MTime offset;
  offset.setUnit(MTime::uiUnit());
  offset.setValue(40);
  proxyShape->timeOffsetPlug().setValue(offset);
  proxyShape->timeScalarPlug().setValue(2);

  // create another proxyShape with a few session layer edits
  AL::usdmaya::nodes::ProxyShape* proxyShape2 = CreateMayaProxyShape(nullptr, sphere2_path, &proxyParent);
  MObject proxyParentNode2 = MFnDagNode(proxyParent).parent(0);
  geoNode.addChild(proxyParentNode2);
  // force the stage to load
  UsdStageRefPtr stage2 = proxyShape2->getUsdStage();
  ASSERT_TRUE(stage2);

  SdfLayerHandle session = stage2->GetSessionLayer();
  stage2->SetEditTarget(session);
  std::string kExtraPrimPath = "/pExtraPrimPath";
  std::string kSecondSpherePath = "/pSphereShape2";
  // add a new prim
  stage2->DefinePrim(SdfPath("/pSphere1" + kExtraPrimPath));
  // deactivate an existing prim
  SdfPath existingSpherePath("/pSphere1" + kSecondSpherePath);
  ASSERT_TRUE(stage2->GetPrimAtPath(existingSpherePath));
  stage2->DefinePrim(existingSpherePath).SetActive(false);

  MGlobal::executeCommand(MString("select world;"), false, true);
  MString exportCmd;
  exportCmd.format(MString("usdExport -f \"^1s\""), AL::maya::utils::convert(temp_path));
  MGlobal::executeCommand(exportCmd, true);

  UsdStageRefPtr resultStage = UsdStage::Open(temp_path);
  ASSERT_TRUE(resultStage);
  SdfLayerHandle rootLayer = resultStage->GetRootLayer();

  const std::string refPrimPath = "/world/geo/" + std::string(MFnDagNode(proxyParentNode1).name().asChar());
  const std::string refPrimPath2 = "/world/geo/" + std::string(MFnDagNode(proxyParentNode2).name().asChar());
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Ref Prim Path 1: " + refPrimPath + "\n");
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Ref Prim Path 2: " + refPrimPath2 + "\n");
  std::string text;
  rootLayer->ExportToString(&text);
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Resulting stage contents: \n" + text);

  // Check proxyShape1
  // make sure references were created and that they have correct offset + scale
  SdfPrimSpecHandle refSpec = rootLayer->GetPrimAtPath(SdfPath(refPrimPath));
  ASSERT_TRUE(refSpec);
  ASSERT_TRUE(refSpec->HasReferences());
  std::vector<SdfReference> refs = refSpec->GetReferenceList().GetAddedOrExplicitItems();
  ASSERT_EQ(refs[0].GetLayerOffset(), SdfLayerOffset(40, 2));

  // Check proxyShape2
  // make sure the session layer was properly grafted on
  UsdPrim refPrim2 = resultStage->GetPrimAtPath(SdfPath(refPrimPath2));
  ASSERT_TRUE(refPrim2.IsValid());
  ASSERT_EQ(refPrim2.GetTypeName(), "Xform");
  ASSERT_EQ(refPrim2.GetSpecifier(), SdfSpecifierDef);

  SdfPrimSpecHandle refSpec2 = rootLayer->GetPrimAtPath(SdfPath(refPrimPath2));
  ASSERT_TRUE(refSpec2);
  ASSERT_TRUE(refSpec2->HasReferences());
  // ref root should be a defined xform on the main export layer also
  ASSERT_EQ(refSpec2->GetTypeName(), "Xform");
  ASSERT_EQ(refSpec2->GetSpecifier(), SdfSpecifierDef);

  const std::string spherePrimPath = refPrimPath2 + std::string(kSecondSpherePath);
  UsdPrim spherePrim = resultStage->GetPrimAtPath(SdfPath(spherePrimPath));
  ASSERT_TRUE(spherePrim.IsValid());
  ASSERT_FALSE(spherePrim.IsActive());
  // check that the proper specs are being created
  SdfPrimSpecHandle specOnExportLayer = rootLayer->GetPrimAtPath(SdfPath(spherePrimPath));
  ASSERT_EQ(specOnExportLayer->GetSpecifier(), SdfSpecifierOver);

}