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

void setup(std::string testName, UsdStageRefPtr* outStage = nullptr)
{
    auto constructTransformChain = []() -> UsdStageRefPtr {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        UsdPrim        p = stage->DefinePrim(SdfPath("/root"));
        return stage;
    };

    MFileIO::newFile(true);
    std::string                     testFilename = testName + ".usda";
    const std::string               tempPath = buildTempPath(testFilename.c_str());
    MObject                         shapeParent;
    AL::usdmaya::nodes::ProxyShape* proxyShape
        = CreateMayaProxyShape(constructTransformChain, tempPath, &shapeParent);

    // force the stage to load
    proxyShape->filePathPlug().setString(tempPath.c_str());
    UsdStageRefPtr stage = proxyShape->getUsdStage();
    if (outStage && stage)
        *outStage = stage;

    SdfLayerHandle layer = stage->GetRootLayer();

    MDagPath       dp;
    MSelectionList sl;
    MDagPath::getAPathTo(shapeParent, dp);
    sl.add(dp);
    MObjectHandle test = shapeParent;

    MGlobal::setActiveSelectionList(sl);
    MSelectionList gl;
    MGlobal::getActiveSelectionList(gl);
}

TEST(LayerManagerCommands, noDirtyLayers)
{
    setup("LayerManagerCommands_noDirtyLayers");
    // Ensure no dirty layers
    {
        MStatus      result;
        MStringArray dirtyLayerPairs;
        // There should be no dirty layers since we have no edited anything
        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dal", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 0u);
        dirtyLayerPairs.clear();

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dso", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 0u);
        dirtyLayerPairs.clear();

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dlo", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 0u);
        dirtyLayerPairs.clear();
    }
}

TEST(LayerManagerCommands, dirtySublayer)
{
    UsdStageRefPtr stage;
    setup("LayerManagerCommands_dirtySublayer", &stage);
    MStatus result;

    // Change a sublayer
    {
        MStringArray   dirtyLayerPairs;
        const char*    sublayerId = "sublayertest";
        SdfLayerRefPtr sublayer = SdfLayer::CreateAnonymous(sublayerId);

        stage->SetEditTarget(stage->GetRootLayer());
        std::vector<std::string> sublayers;
        sublayers.push_back(sublayer->GetIdentifier());
        stage->GetRootLayer()->SetSubLayerPaths(sublayers);
        EXPECT_FALSE(sublayer->IsDirty());

        // Now set the EditTarget to the session layer and dirty it
        stage->SetEditTarget(sublayer);
        stage->DefinePrim(SdfPath("/DirtySublayer"));

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dal", dirtyLayerPairs, true);

        // 4 (layerIdentifier,layerContents), since both the rootlayer and sublayer have been
        // modified.
        EXPECT_EQ(dirtyLayerPairs.length(), 4u);
        dirtyLayerPairs.clear();

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dso", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 0u);
        dirtyLayerPairs.clear();

        // 4 (layerIdentifier,layerContents), since both the rootlayer and sublayer have been
        // modified.
        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dlo", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 4u);
        dirtyLayerPairs.clear();
    }
}

TEST(LayerManagerCommands, dirtySessionLayer)
{
    UsdStageRefPtr stage;
    setup("LayerManagerCommands_dirtySessionLayer", &stage);

    MStatus result;
    // Change a sublayer
    {
        MStringArray dirtyLayerPairs;
        // Now set the EditTarget to the session layer and dirty it
        stage->SetEditTarget(stage->GetSessionLayer());
        stage->DefinePrim(SdfPath("/DirtySessionLayer"));

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dal", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 2u);
        dirtyLayerPairs.clear();

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dso", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 2u);
        dirtyLayerPairs.clear();

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dlo", dirtyLayerPairs, true);

        EXPECT_EQ(dirtyLayerPairs.length(), 0u);
        dirtyLayerPairs.clear();
    }
}

TEST(LayerManagerCommands, dirtySublayerSessionLayer)
{
    UsdStageRefPtr stage;
    setup("LayerManagerCommands_dirtySublayerSessionLayer", &stage);

    MStatus result;

    // Change a Session and a Sublayer
    {
        MStringArray             dirtyLayerPairs;
        const char*              sublayerId = "sublayertest";
        SdfLayerRefPtr           sublayer = SdfLayer::CreateAnonymous(sublayerId);
        std::vector<std::string> sublayers;
        sublayers.push_back(sublayer->GetIdentifier());

        // this modified the RootLayer
        stage->SetEditTarget(stage->GetRootLayer());
        stage->GetRootLayer()->SetSubLayerPaths(sublayers);
        EXPECT_TRUE(stage->GetRootLayer()->IsDirty());
        EXPECT_FALSE(sublayer->IsDirty());

        // Now set the EditTarget to the session layer and dirty it
        stage->SetEditTarget(stage->GetSessionLayer());
        stage->DefinePrim(SdfPath("/DirtySublayer"));
        EXPECT_TRUE(stage->GetSessionLayer()->IsDirty());

        stage->SetEditTarget(sublayer);
        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dal", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 4u);
        dirtyLayerPairs.clear();

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dso", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 2u);
        dirtyLayerPairs.clear();

        result = MGlobal::executeCommand("AL_usdmaya_LayerManager -dlo", dirtyLayerPairs, true);
        EXPECT_EQ(dirtyLayerPairs.length(), 2u);
        dirtyLayerPairs.clear();
    }
}
