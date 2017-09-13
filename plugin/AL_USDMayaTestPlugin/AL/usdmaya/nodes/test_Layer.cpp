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
#include "maya/MFnTransform.h"
#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MDagModifier.h"
#include "maya/MFileIO.h"
#include "maya/MFnDependencyNode.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/sdf/layer.h"

//#define TEST(X, Y) void X##Y()

//  Layer();
//  void init(ProxyShape* shape, SdfLayerHandle handle);
TEST(Layer, init)
{
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_Layer_init.usda";

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MString shapeName;
  {
    MFnDagNode fn;
    MObject xform = fn.create("transform");
    MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();

    auto layer = stage->GetRootLayer();
    AL::usdmaya::nodes::Layer* root = proxy->findLayer(layer);

    // AL_DECL_ATTRIBUTE(comment);
    root->commentPlug().setString("hello dave");
    EXPECT_EQ(std::string("hello dave"), layer->GetComment());
    layer->SetComment("bye dave");
    EXPECT_EQ(MString("bye dave"), root->commentPlug().asString());
    layer->SetComment("");

    // AL_DECL_ATTRIBUTE(defaultPrim);
    // AL_DECL_ATTRIBUTE(documentation);
    root->documentationPlug().setString("hello dave");
    EXPECT_EQ(std::string("hello dave"), layer->GetDocumentation());
    layer->SetDocumentation("bye dave");
    EXPECT_EQ(MString("bye dave"), root->documentationPlug().asString());
    layer->SetDocumentation("");

    // AL_DECL_ATTRIBUTE(startTime);
    root->startTimePlug().setDouble(3.0);
    EXPECT_EQ(3.0, layer->GetStartTimeCode());
    layer->SetStartTimeCode(4.0);
    EXPECT_EQ(4.0, root->startTimePlug().asDouble());
    layer->SetStartTimeCode(0);

    // AL_DECL_ATTRIBUTE(endTime);
    root->endTimePlug().setDouble(3.0);
    EXPECT_EQ(3.0, layer->GetEndTimeCode());
    layer->SetEndTimeCode(4.0);
    EXPECT_EQ(4.0, root->endTimePlug().asDouble());
    layer->SetEndTimeCode(0);

    // AL_DECL_ATTRIBUTE(timeCodesPerSecond);
    root->timeCodesPerSecondPlug().setDouble(3.0);
    EXPECT_EQ(3.0, layer->GetTimeCodesPerSecond());
    layer->SetTimeCodesPerSecond(4.0);
    EXPECT_EQ(4.0, root->timeCodesPerSecondPlug().asDouble());
    layer->SetTimeCodesPerSecond(0);

    // AL_DECL_ATTRIBUTE(framePrecision);
    root->framePrecisionPlug().setValue(3);
    EXPECT_EQ(3, layer->GetFramePrecision());
    layer->SetFramePrecision(4);
    EXPECT_EQ(4, root->framePrecisionPlug().asInt());
    layer->SetFramePrecision(0);

    // AL_DECL_ATTRIBUTE(owner);
    root->ownerPlug().setString("hello dave");
    EXPECT_EQ(std::string("hello dave"), layer->GetOwner());
    layer->SetOwner("bye dave");
    EXPECT_EQ(MString("bye dave"), root->ownerPlug().asString());
    layer->SetOwner("");

    // AL_DECL_ATTRIBUTE(sessionOwner);
    root->sessionOwnerPlug().setString("hello dave");
    EXPECT_EQ(std::string("hello dave"), layer->GetSessionOwner());
    layer->SetSessionOwner("bye dave");
    EXPECT_EQ(MString("bye dave"), root->sessionOwnerPlug().asString());
    layer->SetSessionOwner("");

    // AL_DECL_ATTRIBUTE(permissionToEdit);
    root->permissionToEditPlug().setBool(false);
    EXPECT_FALSE(layer->PermissionToEdit());
    layer->SetPermissionToEdit(true);
    EXPECT_TRUE(root->permissionToEditPlug().asBool());

    // AL_DECL_ATTRIBUTE(permissionToSave);
    root->permissionToSavePlug().setBool(false);
    EXPECT_FALSE(layer->PermissionToSave());
    layer->SetPermissionToSave(true);
    EXPECT_TRUE(root->permissionToSavePlug().asBool());

    // read only identification
    // AL_DECL_ATTRIBUTE(displayName);
    EXPECT_EQ(MString(layer->GetDisplayName().c_str()), root->displayNamePlug().asString());

    // AL_DECL_ATTRIBUTE(realPath);
    EXPECT_EQ(MString(layer->GetRealPath().c_str()), root->realPathPlug().asString());

    // AL_DECL_ATTRIBUTE(fileExtension);
    EXPECT_EQ(MString(layer->GetFileExtension().c_str()), root->fileExtensionPlug().asString());

    // AL_DECL_ATTRIBUTE(version);
    EXPECT_EQ(MString(layer->GetVersion().c_str()), root->versionPlug().asString());

    // AL_DECL_ATTRIBUTE(repositoryPath);
    EXPECT_EQ(MString(layer->GetRepositoryPath().c_str()), root->repositoryPathPlug().asString());

    // AL_DECL_ATTRIBUTE(assetName);
    EXPECT_EQ(MString(layer->GetAssetName().c_str()), root->assetNamePlug().asString());

    // AL_DECL_ATTRIBUTE(proxyShape);
    // AL_DECL_ATTRIBUTE(subLayers);
    // AL_DECL_ATTRIBUTE(childLayers);
    // AL_DECL_ATTRIBUTE(parentLayer);
  }

  MFileIO::newFile(true);
}

//  SdfLayerHandle getHandle();
//  std::vector<Layer*> getSubLayers();
//  std::vector<Layer*> getChildLayers();
//  Layer* getParentLayer();
//  MPlug parentLayerPlug();
//  Layer* findLayer(SdfLayerHandle handle);
TEST(Layer, getHandleSimple)
{
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_Layer_init.usda";

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MString shapeName;
  {
    MFnDagNode fn;
    MObject xform = fn.create("transform");
    MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();

    AL::usdmaya::nodes::Layer* root = proxy->findLayer(stage->GetRootLayer());
    AL::usdmaya::nodes::Layer* sesh = proxy->findLayer(stage->GetSessionLayer());
    EXPECT_NE(nullptr, root);
    EXPECT_NE(nullptr, sesh);
    EXPECT_TRUE(root->getParentLayer() == sesh);

    EXPECT_TRUE(root->getHandle() == stage->GetRootLayer());
    EXPECT_TRUE(sesh->getHandle() == stage->GetSessionLayer());
    EXPECT_EQ(root, sesh->findLayer(stage->GetRootLayer()));

    {
      auto layers = sesh->getChildLayers();
      EXPECT_EQ(1, layers.size());
      EXPECT_TRUE(root == layers[0]);
    }

    {
      auto layers = sesh->getSubLayers();
      EXPECT_EQ(0, layers.size());
    }
  }
}

// bool removeChildLayer(Layer* childLayer);
// void addChildLayer(Layer* childLayer, MDGModifier* modifier = 0);
TEST(Layer, addRemoveChildLayers)
{
  /*
   * Tht addChildLayer methods only add child Layers in the Maya world, this is
   * to solve a use case where we have a reference that was unable to be resolved,
   * in this case we need to create a new layer on the fly and, target it with edits,
   * then save it out.
   */
  MFileIO::newFile(true);

  const std::string temp_path = "/tmp/AL_USDMayaTests_addRemoveChildLayers.usda";
  UsdStageRefPtr stage = UsdStage::CreateInMemory();
  stage->DefinePrim(SdfPath("/root"));
  stage->Export(temp_path, false);

  MFnDagNode fn;
  MObject xform = fn.create("transform");
  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
  AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
  proxy->filePathPlug().setString(temp_path.c_str());

  auto mayaStage = proxy->getUsdStage();
  AL::usdmaya::nodes::Layer* mayaRootLayer = proxy->findLayer(mayaStage->GetRootLayer());
  EXPECT_TRUE(mayaRootLayer);

  MFnDependencyNode fnl;
  MObject mayaChildNode = fnl.create(AL::usdmaya::nodes::Layer::kTypeId);
  AL::usdmaya::nodes::Layer* newLayer = (AL::usdmaya::nodes::Layer*)fnl.userNode();

  // Check that there is a child layer
  EXPECT_EQ(0, mayaRootLayer->getChildLayers().size());
  mayaRootLayer->addChildLayer(newLayer);
  EXPECT_EQ(1, mayaRootLayer->getChildLayers().size());
  mayaRootLayer->removeChildLayer(newLayer);
  EXPECT_EQ(0, mayaRootLayer->getChildLayers().size());
}

//  void buildSubLayers(MDGModifier* pmodifier = 0);
//  void addSubLayer(Layer* subLayer, MDGModifier* pmodifier = 0);
//  bool removeSubLayer(Layer* subLayer);
TEST(Layer, addRemoveSublayersLayers)
{
  MFileIO::newFile(true);
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_Layer_init.usda";

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MString shapeName;
  {
    MFnDagNode fn;
    MObject xform = fn.create("transform");
    MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();

    AL::usdmaya::nodes::Layer* root = proxy->findLayer(stage->GetRootLayer());

    // construct the new layer node
    MFnDependencyNode fnl;
    MObject layerNode = fnl.create(AL::usdmaya::nodes::Layer::kTypeId);
    AL::usdmaya::nodes::Layer* m_newLayer = (AL::usdmaya::nodes::Layer*)fnl.userNode();


    auto newLayer = SdfLayer::CreateNew("/tmp/AL_USDMayaTests_new_layer.usda");

    auto layer = root->getHandle();
    layer->GetSubLayerPaths().push_back(newLayer->GetIdentifier());
    layer->Save();
    m_newLayer->init(proxy, newLayer);
    root->addSubLayer(m_newLayer);

    auto sublayers = root->getSubLayers();
    EXPECT_EQ(1, sublayers.size());
    EXPECT_TRUE(sublayers[0] == m_newLayer);
    EXPECT_TRUE(root == m_newLayer->getParentLayer());

    EXPECT_TRUE(root->removeSubLayer(m_newLayer));

    sublayers = root->getSubLayers();
    EXPECT_EQ(0, sublayers.size());
    EXPECT_TRUE(0 == m_newLayer->getParentLayer());
  }
}

//  static MString toMayaNodeName(std::string name);
TEST(Layer, toMayaNodeName)
{
  MString ex = "bah_blah_usdc";
  MString temp1 = AL::usdmaya::nodes::Layer::toMayaNodeName("bah_blah.usdc");
  MString temp2 = AL::usdmaya::nodes::Layer::toMayaNodeName("bah blah.usdc");
  EXPECT_EQ(ex, temp1);
  EXPECT_EQ(ex, temp2);
}


//  bool hasBeenTheEditTarget() const;
//  void setHasBeenTheEditTarget(bool value);
TEST(Layer, editTarget)
{
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_Layer_editTarget.usda";

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MString shapeName;
  {
    MFnDagNode fn;
    MObject xform = fn.create("transform");
    MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();
    AL::usdmaya::nodes::Layer* sesh = proxy->findLayer(stage->GetSessionLayer());
    AL::usdmaya::nodes::Layer* root = proxy->findLayer(stage->GetRootLayer());
    EXPECT_TRUE(sesh != 0);
    EXPECT_TRUE(root != 0);

    // initially, the root layer should be the edit target. The session layer will not yet have been set as a target
    EXPECT_FALSE(sesh->hasBeenTheEditTarget());
    EXPECT_TRUE(root->hasBeenTheEditTarget());

    // If I modify the edit target directly on the stage, does the maya plugin track that change?
    stage->SetEditTarget(stage->GetSessionLayer());

    EXPECT_TRUE(sesh->hasBeenTheEditTarget());
    EXPECT_TRUE(root->hasBeenTheEditTarget());

    // can I forcibly override the flag?
    sesh->setHasBeenTheEditTarget(false);
    root->setHasBeenTheEditTarget(false);
    EXPECT_FALSE(sesh->hasBeenTheEditTarget());
    EXPECT_FALSE(root->hasBeenTheEditTarget());
    sesh->setHasBeenTheEditTarget(true);
    root->setHasBeenTheEditTarget(true);
    EXPECT_TRUE(sesh->hasBeenTheEditTarget());
    EXPECT_TRUE(root->hasBeenTheEditTarget());
  }
}


//  void setLayerAndClearAttribute(SdfLayerHandle handle);
//  void populateSerialisationAttributes();
TEST(Layer, populateSerialisationAttributes)
{
  auto constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform root = UsdGeomXform::Define(stage, SdfPath("/root"));
    UsdGeomXform leg1 = UsdGeomXform::Define(stage, SdfPath("/root/hip1"));
    return stage;
  };

  const std::string temp_path = "/tmp/AL_USDMayaTests_Layer_editTarget.usda";

  // generate some data for the proxy shape
  {
    auto stage = constructTransformChain();
    stage->Export(temp_path, false);
  }

  MString shapeName;
  {
    MFnDagNode fn;
    MObject xform = fn.create("transform");
    MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();

    // force the stage to load
    proxy->filePathPlug().setString(temp_path.c_str());

    auto stage = proxy->getUsdStage();
    AL::usdmaya::nodes::Layer* sesh = proxy->findLayer(stage->GetSessionLayer());
    AL::usdmaya::nodes::Layer* root = proxy->findLayer(stage->GetRootLayer());
    EXPECT_TRUE(sesh != 0);
    EXPECT_TRUE(root != 0);

    sesh->populateSerialisationAttributes();
    root->populateSerialisationAttributes();

    // I initially expect the session layer to do nothing here (it has not been the edit target as yet)
    {
      MString name = sesh->nameOnLoadPlug().asString();
      MString contents = sesh->serializedPlug().asString();

      EXPECT_EQ(0, name.length());
      EXPECT_EQ(0, contents.length());
    }

    std::string sesh_contents;
    std::string root_contents;
    {
      MString name = root->nameOnLoadPlug().asString();
      MString contents = root->serializedPlug().asString();

      MString ex_name = root->realPathPlug().asString();

      auto layer = root->getHandle();
      layer->ExportToString(&root_contents);

      EXPECT_NE(0, name.length());
      EXPECT_NE(0, contents.length());

      EXPECT_EQ(ex_name, name);
      EXPECT_EQ(MString(root_contents.c_str()), contents);
    }

    stage->SetEditTarget(stage->GetSessionLayer());
    sesh->populateSerialisationAttributes();

    {
      MString name = sesh->nameOnLoadPlug().asString();
      MString contents = sesh->serializedPlug().asString();

      MString ex_name = sesh->realPathPlug().asString();

      auto layer = sesh->getHandle();
      layer->ExportToString(&sesh_contents);

      EXPECT_EQ(0U, name.length());
      EXPECT_NE(0U, contents.length());

      EXPECT_EQ(ex_name, name);
      EXPECT_EQ(MString(sesh_contents.c_str()), contents);
    }

    // nuke the layer handles (simulate a file re-open)
    sesh->testing_clearHandle();
    root->testing_clearHandle();

    // this should reset the handles, re-import the session layer, and leave an empty serialisation attrib
    sesh->setLayerAndClearAttribute(stage->GetSessionLayer());
    root->setLayerAndClearAttribute(stage->GetRootLayer());

    EXPECT_EQ(MString(), sesh->serializedPlug().asString());
    EXPECT_EQ(MString(), root->serializedPlug().asString());

    EXPECT_TRUE(root->getHandle() == stage->GetRootLayer());
    EXPECT_TRUE(sesh->getHandle() == stage->GetSessionLayer());


    MFileIO::newFile(true);
  }
}

/*
 * Tests that AL_USDMaya creates layers in maya.
 */
// Disabled: TODO: Set up this test in such a way that the referenced-identifier that is referenced resolves to a different identifier by the resolver
//TEST(Layer, hasCorrectlyTranslatedLayers)
//{
//  const std::string rootLayerPath = std::string(AL_USDMAYA_TEST_DATA) + std::string("/root.usda");
//  const std::string subLayerPath = std::string(AL_USDMAYA_TEST_DATA) + std::string("/subLayer.usda");
//  const std::string referencedLayerPath = std::string(AL_USDMAYA_TEST_DATA) + std::string("/referencedLayer.usda");
//
//  MFnDagNode fn;
//  MObject xform = fn.create("transform");
//  MObject shape = fn.create("AL_usdmaya_ProxyShape", xform);
//  AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
//  auto mayaStage = proxy->getUsdStage();
//
//  // force the stage to load
//  proxy->filePathPlug().setString(MString(rootLayerPath.c_str()));
//
//  MString result = MGlobal::executeCommandStringResult("ls -type \"AL_usdmaya_Layer\"", true);
//
//  SdfLayerHandle rootLayer = SdfLayer::Find(rootLayerPath);
//  SdfLayerHandle subLayer = SdfLayer::Find(subLayerPath);
//  SdfLayerHandle referencedLayer = SdfLayer::Find(referencedLayerPath);
//
//  AL::usdmaya::nodes::Layer* mayaRoot = proxy->findLayer(rootLayer);
//  EXPECT_TRUE(mayaRoot != 0);
//
//  AL::usdmaya::nodes::Layer* mayaSubLayer = proxy->findLayer(subLayer);
//  EXPECT_TRUE(mayaSubLayer != 0);
//
//
//  AL::usdmaya::nodes::Layer* mayaReferencedLayer = proxy->findLayer(referencedLayer);
//  EXPECT_TRUE(mayaReferencedLayer != 0);
//}




