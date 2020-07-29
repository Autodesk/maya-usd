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
#include "AL/usdmaya/utils/Utils.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>

#include <functional>

using AL::maya::test::buildTempPath;

//  Layer();
//  void init(ProxyShape* shape, SdfLayerHandle handle);
TEST(LayerCommands, layerCreateLayerTests)
{
    std::function<UsdStageRefPtr()> constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim        p = stage->DefinePrim(SdfPath("/layerCreateLayerTests"));
        return stage;
    };

    MFileIO::newFile(true);
    MString shapeName;
    {
        const std::string temp_path = buildTempPath("AL_USDMayaTests_layerCreateLayerTests.usda");
        const std::string testLayer = std::string(AL_USDMAYA_TEST_DATA) + "/root.usda";
        AL::usdmaya::nodes::ProxyShape* proxyShape
            = CreateMayaProxyShape(constructTransformChain, temp_path);
        // force the stage to load
        proxyShape->filePathPlug().setString(temp_path.c_str());

        auto stage = proxyShape->getUsdStage();

        SdfLayerHandle layer = stage->GetRootLayer();

        MStatus result = MGlobal::executeCommand("ls -type \"AL_usdmaya_Layer\"", true);
        EXPECT_EQ(result, MStatus::kSuccess);

        SdfLayerRefPtr handle = SdfLayer::FindOrOpen(testLayer); // hold a strong reference to it.

        std::stringstream ss;
        ss << "AL_usdmaya_LayerCreateLayer -o \"" << testLayer << "\" -p \""
           << AL::maya::utils::convert(proxyShape->name()) << "\"" << std::endl;
        result = MGlobal::executeCommand(ss.str().c_str(), true);
        EXPECT_EQ(result, MStatus::kSuccess);

        result = MGlobal::executeCommand("ls -type \"AL_usdmaya_Layer\"", true);
        EXPECT_EQ(result, MStatus::kSuccess);

        // Assert the new layer has been created in the Usd's layer cache
        SdfLayerHandle expectedLayer = SdfLayer::Find(testLayer);
        EXPECT_TRUE(expectedLayer);

        // Check that since the layer hasn't been modified, it's not in the layerManager
        AL::usdmaya::nodes::LayerManager* layerManager
            = AL::usdmaya::nodes::LayerManager::findManager();
        EXPECT_TRUE(layerManager);
        SdfLayerHandle refoundExpectedLayer
            = layerManager->findLayer(expectedLayer->GetIdentifier());
        EXPECT_FALSE(refoundExpectedLayer);

        // Then dirty it, and check that we can find it in the layerManager
        expectedLayer->SetComment("SetLayerAsDirty");
        refoundExpectedLayer = layerManager->findLayer(expectedLayer->GetIdentifier());
        EXPECT_TRUE(refoundExpectedLayer);
        EXPECT_EQ(refoundExpectedLayer, expectedLayer);
    }
}

// Test that I can successfully create and add a sublayer to the RootLayer
TEST(LayerCommands, addSubLayer)
{
    MFileIO::newFile(true);
    MString           shapeName;
    const std::string temp_path = buildTempPath("AL_USDMayaTests_addSubLayer.usda");

    std::function<UsdStageRefPtr()> constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        return stage;
    };

    AL::usdmaya::nodes::ProxyShape* proxyShape
        = CreateMayaProxyShape(constructTransformChain, temp_path);

    EXPECT_EQ(proxyShape->getUsdStage()->GetLayerStack().size(), 2u); // Session layer and RootLayer

    // Add anonymous layer to the sublayers
    MGlobal::executeCommand("AL_usdmaya_LayerCreateLayer -s -o \"\" -p \"AL_usdmaya_ProxyShape1\"");
    EXPECT_EQ(proxyShape->getUsdStage()->GetLayerStack().size(), 3u); // With added anonymous layer

    const MString testLayer = MString(AL_USDMAYA_TEST_DATA) + MString("/root.usda");
    MString       c;
    c.format(
        MString("AL_usdmaya_LayerCreateLayer -s -o \"^1s\" -p \"AL_usdmaya_ProxyShape1\""),
        testLayer);

    MGlobal::executeCommand(c);
    EXPECT_EQ(proxyShape->getUsdStage()->GetLayerStack().size(), 5u); // With added named layer
}
