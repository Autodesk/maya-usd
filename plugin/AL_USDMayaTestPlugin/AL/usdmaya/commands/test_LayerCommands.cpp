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
#include "AL/usdmaya/Utils.h"
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
#include <functional>

//  Layer();
//  void init(ProxyShape* shape, SdfLayerHandle handle);
TEST(LayerCommands, layerCreateLayerTests)
{
  std::function<UsdStageRefPtr()>  constructTransformChain = [] ()
  {
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim p = stage->DefinePrim(SdfPath("/layerCreateLayerTests"));
    return stage;
  };

  MFileIO::newFile(true);
  MString shapeName;
  {
    const std::string temp_path = "/tmp/AL_USDMayaTests_layerCreateLayerTests.usda";
    const std::string testLayer = std::string(AL_USDMAYA_TEST_DATA) + "/root.usda";
    AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(constructTransformChain, temp_path);
    // force the stage to load
    proxyShape->filePathPlug().setString(temp_path.c_str());

    auto stage = proxyShape->getUsdStage();

    SdfLayerHandle layer = stage->GetRootLayer();
    AL::usdmaya::nodes::Layer* root = proxyShape->findLayer(layer);

    MStatus result = MGlobal::executeCommand("ls -type \"AL_usdmaya_Layer\"", true);
    EXPECT_EQ(result, MStatus::kSuccess);

    SdfLayerRefPtr handle = SdfLayer::FindOrOpen(testLayer); // hold a strong reference to it.

    std::stringstream ss;
    ss << "AL_usdmaya_LayerCreateLayer -o \"" << testLayer  <<  "\" -p \"" << AL::usdmaya::convert(proxyShape->name()) << "\"" << std::endl;
    result = MGlobal::executeCommand(ss.str().c_str(), true);
    EXPECT_EQ(result, MStatus::kSuccess);

    result = MGlobal::executeCommand("ls -type \"AL_usdmaya_Layer\"", true);
    EXPECT_EQ(result, MStatus::kSuccess);

    // Assert the new layer has been created in the Usd's layer cache
    SdfLayerHandle expectedLayer = SdfLayer::Find(testLayer);
    EXPECT_TRUE(expectedLayer);

    // Check that it is a child of the right layer
    AL::usdmaya::nodes::Layer* refoundExpectedLayer = root->findChildLayer(expectedLayer);
    EXPECT_TRUE(refoundExpectedLayer);
  }
}

