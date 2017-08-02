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
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/Metadata.h"
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MDagModifier.h"
#include "maya/MFileIO.h"
#include "maya/MStringArray.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

/*
 * Test that prims the are tagged as selectable are deemed "selectable" once the restrictionSelection
 * flag is turned on in the proxy shape. Once restrictionSelection is offoff, everything that wasn't
 * tagged as selectable should still be selectable
 */
TEST(ProxyShapeSelectableDB, restrictedSelection)
{
  MFileIO::newFile(true);

  MString shapeName;

  //Test that the seletable prims
  {
    MFnDagNode fn;
    MObject xform = fn.create("transform");
    MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->DefinePrim(SdfPath("/A/B/C"));

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    const std::string temp_path = "/tmp/AL_USDMayaTests_ProxyShape_restrictedSelection.usda";
    proxy->filePathPlug().setString(temp_path.c_str());

    SdfPath pathA("/A");
    SdfPath pathB("/A/B");
    SdfPath pathC("/A/B/C");

    //Turn off the restriction, so everything should become selectable.
    proxy->unrestrictSelection();
    EXPECT_FALSE(proxy->isSelectionRestricted());
    EXPECT_TRUE(proxy->isPathSelectable(pathA));
    EXPECT_TRUE(proxy->isPathSelectable(pathB));

    // After restricting the selection, all the prims should become unselectable
    proxy->restrictSelection();
    EXPECT_TRUE(proxy->isSelectionRestricted());
    EXPECT_FALSE(proxy->isPathSelectable(pathA));
    EXPECT_FALSE(proxy->isPathSelectable(pathB));
    EXPECT_FALSE(proxy->isPathSelectable(pathC));

    // Add the path as selectable, now it should be selectable
    proxy->selectableDB().addPathAsSelectable(pathA);
    EXPECT_TRUE(proxy->isPathSelectable(pathA));
    EXPECT_TRUE(proxy->isPathSelectable(pathB));
  }
}

/*
 * Test that prims that are marked as selectable are picked when opening a new stage
 * and that prims whos selectability is modified is picked up
 */
TEST(ProxyShapeSelectableDB, selectablesOnOpen)
{
  std::function<UsdStageRefPtr()>  constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->DefinePrim(SdfPath("/A/B/C"));
    UsdPrim b = stage->GetPrimAtPath(SdfPath("/A/B"));
    b.SetMetadata<TfToken>(AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::selectable);
    return stage;
  };

  MFileIO::newFile(true);
  const std::string temp_path = "/tmp/AL_USDMayaTests_ProxyShape_selectablesOnOpen.usda";
  AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(constructTransformChain, temp_path);
  proxyShape->restrictSelection();
  proxyShape->filePathPlug().setString(temp_path.c_str());

  //Check that the path is selectable directly using the selectableDB object
  SdfPath expectedSelectable("/A/B");
  EXPECT_TRUE(proxyShape->selectableDB().isPathSelectable(expectedSelectable));
}

// Test changin
TEST(ProxyShapeSelectableDB, selectablesOnModification)
{
  std::function<UsdStageRefPtr()>  constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->DefinePrim(SdfPath("/A/B/C"));
    return stage;
  };

  MFileIO::newFile(true);

  // unsure undo is enabled for this test
  const std::string temp_path = "/tmp/AL_USDMayaTests_ProxyShape_selectablesOnModification.usda";
  AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(constructTransformChain, temp_path);
  proxyShape->filePathPlug().setString(temp_path.c_str());

  SdfPath expectedSelectable("/A/B");
  EXPECT_FALSE(proxyShape->selectableDB().isPathSelectable(expectedSelectable));

  UsdPrim b = proxyShape->getUsdStage()->GetPrimAtPath(expectedSelectable);
  b.SetMetadata(AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::selectable);

  //Check that the path is selectable directly using the selectableDB object
  EXPECT_TRUE(proxyShape->selectableDB().isPathSelectable(expectedSelectable));
}

TEST(ProxyShapeSelectableDB, selectableIsRemoval)
{
  std::function<UsdStageRefPtr()>  constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->DefinePrim(SdfPath("/A/B/C"));
    UsdPrim b = stage->GetPrimAtPath(SdfPath("/A/B"));

    b.SetMetadata(AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::selectable);
    return stage;
  };

  MFileIO::newFile(true);
  const std::string temp_path = "/tmp/AL_USDMayaTests_ProxyShape_selectableIsRemoval.usda";
  AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(constructTransformChain, temp_path);
  proxyShape->restrictSelection();
  proxyShape->filePathPlug().setString(temp_path.c_str());
  SdfPath expectedSelectable("/A/B");
  EXPECT_TRUE(proxyShape->selectableDB().isPathSelectable(expectedSelectable));

  UsdPrim b = proxyShape->getUsdStage()->GetPrimAtPath(expectedSelectable);
  b.SetMetadata(AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::unselectable);

  //Check that the path has been removed from the selectable list
  EXPECT_FALSE(proxyShape->selectableDB().isPathSelectable(expectedSelectable));
}
