//
// Copyright 2026 Autodesk
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

#include "renderSetupTestUtils.h"

#include <mayaUsdUI/ui/mayaBatchRenderHandler.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/pathString.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace RenderSetupTest;
using AdskUsdRenderSetup::RenderLayerManager;
using MayaUsdRenderSetup::MayaBatchRenderHandler;

class MayaBatchRenderHandlerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        resetScene();
        stage = createProxyShapeStage();
        ASSERT_TRUE(stage);
        manager().attach(stage);
    }

    void TearDown() override
    {
        manager().detach(stage);
        if (!layerFile.empty()) {
            std::error_code ignored;
            std::filesystem::remove(layerFile, ignored);
        }
    }

    static RenderLayerManager& manager() { return RenderLayerManager::instance(); }

    UsdStageRefPtr        stage;
    std::filesystem::path layerFile;
};

TEST_F(MayaBatchRenderHandlerTest, NoActiveRenderLayerGivesNoOptions)
{
    manager().createRenderLayer(stage, "Beauty");

    EXPECT_TRUE(MayaBatchRenderHandler::sessionLayerRenderOptions(stage).empty());
}

TEST_F(MayaBatchRenderHandlerTest, AnonymousActiveRenderLayerGivesNoOptions)
{
    const std::string name = manager().createRenderLayer(stage, "Beauty");
    manager().setActiveRenderLayer(stage, name);
    ASSERT_TRUE(manager().getActiveRenderLayer(stage));

    EXPECT_TRUE(MayaBatchRenderHandler::sessionLayerRenderOptions(stage).empty());
}

TEST_F(MayaBatchRenderHandlerTest, FileActiveRenderLayerGivesSessionLayerOptions)
{
    layerFile = std::filesystem::temp_directory_path() / "testMayaBatchRenderHandler.usda";
    SdfLayerRefPtr layer = SdfLayer::CreateNew(layerFile.string());
    ASSERT_TRUE(layer);

    const std::string name = manager().addRenderLayer(stage, layer, "Beauty");
    ASSERT_EQ(name, "Beauty");
    manager().setActiveRenderLayer(stage, name);
    ASSERT_TRUE(manager().getActiveRenderLayer(stage));

    std::string expectedPath = layer->GetRealPath();
    std::replace(expectedPath.begin(), expectedPath.end(), '\\', '/');
    const std::string expectedStage = Ufe::PathString::string(MayaUsd::ufe::stagePath(stage));
    ASSERT_FALSE(expectedStage.empty());

    EXPECT_EQ(
        MayaBatchRenderHandler::sessionLayerRenderOptions(stage),
        "-sessionLayer \\\"" + expectedPath + "\\\" -sessionLayerStage \\\"" + expectedStage
            + "\\\"");
}

TEST_F(MayaBatchRenderHandlerTest, NullStageGivesNoOptions)
{
    EXPECT_TRUE(MayaBatchRenderHandler::sessionLayerRenderOptions(nullptr).empty());
}
