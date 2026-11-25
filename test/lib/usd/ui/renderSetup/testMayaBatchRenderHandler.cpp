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

#include <mayaUsdUI/ui/mayaBatchRenderAllHandler.h>
#include <mayaUsdUI/ui/mayaBatchRenderHandler.h>
#include <mayaUsdUI/ui/mayaBatchRenderQueue.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>
#include <ufe/pathString.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace RenderSetupTest;
using AdskUsdRenderSetup::RenderDescription;
using AdskUsdRenderSetup::RenderLayerManager;
using MayaUsdRenderSetup::MayaBatchRenderAllHandler;
using MayaUsdRenderSetup::MayaBatchRenderHandler;
using MayaUsdRenderSetup::MayaBatchRenderQueue;

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
        for (const auto& layerFile : layerFiles) {
            std::error_code ignored;
            std::filesystem::remove(layerFile, ignored);
        }
    }

    static RenderLayerManager& manager() { return RenderLayerManager::instance(); }

    //! Adds a render layer saved to a temporary file, removed on tear down.
    SdfLayerRefPtr addFileRenderLayer(const std::string& name, bool bindRenderPass = true)
    {
        layerFiles.push_back(
            std::filesystem::temp_directory_path()
            / ("testMayaBatchRenderHandler" + name + ".usda"));
        SdfLayerRefPtr layer = SdfLayer::CreateNew(layerFiles.back().string());
        if (layer) {
            EXPECT_EQ(manager().addRenderLayer(stage, layer, name, bindRenderPass), name);
        }
        return layer;
    }

    //! The -activeRenderDescription option escaped for a MEL string, in the format
    //! MayaRenderSetupHost::setActiveRenderDescription writes.
    std::string activeRenderDescriptionOption(const std::string& passPath) const
    {
        return "-activeRenderDescription \\\""
            + Ufe::PathString::string(MayaUsd::ufe::stagePath(stage)) + "," + passPath + "\\\"";
    }

    static std::string forwardSlashes(std::string path)
    {
        std::replace(path.begin(), path.end(), '\\', '/');
        return path;
    }

    UsdStageRefPtr                     stage;
    std::vector<std::filesystem::path> layerFiles;
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
    SdfLayerRefPtr layer = addFileRenderLayer("Beauty");
    ASSERT_TRUE(layer);
    manager().setActiveRenderLayer(stage, "Beauty");
    ASSERT_TRUE(manager().getActiveRenderLayer(stage));

    const std::string expectedPath = forwardSlashes(layer->GetRealPath());
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

TEST_F(MayaBatchRenderHandlerTest, DisplayNameNamesTheActiveRenderLayer)
{
    const std::string name = manager().createRenderLayer(stage, "Beauty");
    manager().setActiveRenderLayer(stage, name);

    EXPECT_EQ(
        MayaBatchRenderHandler::displayNameFor(
            RenderDescription { stage, SdfPath("/Render/Settings") }),
        "Batch render Beauty");
}

TEST_F(MayaBatchRenderHandlerTest, DisplayNameNamesTheRenderSettingWithoutActiveRenderLayer)
{
    manager().createRenderLayer(stage, "Beauty");

    EXPECT_EQ(
        MayaBatchRenderHandler::displayNameFor(
            RenderDescription { stage, SdfPath("/Render/Settings") }),
        "Batch render Settings");
}

TEST_F(MayaBatchRenderHandlerTest, DisplayNameWithoutTargetIsPlain)
{
    EXPECT_EQ(MayaBatchRenderHandler::displayNameFor(RenderDescription {}), "Batch render");
}

TEST_F(MayaBatchRenderHandlerTest, RenderAllCommandsCoverRenderableLayersWithAFile)
{
    SdfLayerRefPtr first = addFileRenderLayer("First");
    SdfLayerRefPtr skipped = addFileRenderLayer("Skipped");
    SdfLayerRefPtr second = addFileRenderLayer("Second");
    SdfLayerRefPtr noPass = addFileRenderLayer("Third", false);
    ASSERT_TRUE(first && skipped && second && noPass);
    manager().setIncludedInRender(stage, "Skipped", false);
    manager().createRenderLayer(stage, "Anonymous");
    const SdfPath firstPass("/Render/FirstPass");
    manager().bindRenderPass(stage, "First", firstPass);

    const std::vector<MString> commands = MayaBatchRenderAllHandler::batchRenderCommands(stage);

    // Third has no pass, so it would render whatever description the scene saved.
    ASSERT_EQ(commands.size(), 2u);
    const std::string firstCommand = commands[0].asChar();
    const std::string secondCommand = commands[1].asChar();
    EXPECT_EQ(firstCommand.rfind("batchRender ", 0), 0u);
    EXPECT_EQ(secondCommand.rfind("batchRender ", 0), 0u);
    const std::string firstSession
        = "-sessionLayer \\\"" + forwardSlashes(first->GetRealPath()) + "\\\"";
    const std::string secondSession
        = "-sessionLayer \\\"" + forwardSlashes(second->GetRealPath()) + "\\\"";
    EXPECT_NE(firstCommand.find(firstSession), std::string::npos);
    EXPECT_NE(secondCommand.find(secondSession), std::string::npos);

    // Each layer renders its own pass, and the flag precedes the other Render flags.
    const std::string firstActive = activeRenderDescriptionOption(firstPass.GetString());
    const std::string secondActive
        = activeRenderDescriptionOption(AdskUsdRenderSetup::kDefaultRenderPassPath);
    ASSERT_NE(firstActive, secondActive);
    ASSERT_NE(firstCommand.find(firstActive), std::string::npos);
    ASSERT_NE(secondCommand.find(secondActive), std::string::npos);
    EXPECT_LT(firstCommand.find(firstActive), firstCommand.find(firstSession));
    EXPECT_LT(secondCommand.find(secondActive), secondCommand.find(secondSession));
    EXPECT_EQ(firstCommand.find(secondActive), std::string::npos);
    EXPECT_EQ(secondCommand.find(firstActive), std::string::npos);
}

TEST_F(MayaBatchRenderHandlerTest, RenderAllWithoutTheMayaHydraFlagStartsNothing)
{
    int loaded = 0;
    int exists = 0;
    MGlobal::executeCommand("pluginInfo -q -loaded mayaHydra", loaded);
    MGlobal::executeCommand("exists mayaHydraSetActiveRenderDescription", exists);
    if (loaded && exists) {
        GTEST_SKIP() << "MayaHydra supports -activeRenderDescription here.";
    }
    ASSERT_TRUE(addFileRenderLayer("Beauty"));

    EXPECT_FALSE(MayaBatchRenderAllHandler().render());
    EXPECT_EQ(MayaBatchRenderQueue::instance().pendingCount(), 0u);
}

TEST_F(MayaBatchRenderHandlerTest, RenderAllWithNothingRenderableGivesNoCommands)
{
    manager().createRenderLayer(stage, "Anonymous");
    SdfLayerRefPtr excluded = addFileRenderLayer("Excluded");
    ASSERT_TRUE(excluded);
    manager().setIncludedInRender(stage, "Excluded", false);

    EXPECT_TRUE(MayaBatchRenderAllHandler::batchRenderCommands(stage).empty());
    EXPECT_TRUE(MayaBatchRenderAllHandler::batchRenderCommands(nullptr).empty());
}

TEST(MayaBatchRenderQueueTest, StatusProcessIdReadsProgressReports)
{
    EXPECT_EQ(
        MayaBatchRenderQueue::statusProcessId("batchRender -status \"-1111 4242 C:/out.exr\""),
        4242);
    EXPECT_EQ(MayaBatchRenderQueue::statusProcessId("batchRender -st \"100 17 \""), 17);
    // As Maya hands it to command callbacks, with quoted flags.
    EXPECT_EQ(
        MayaBatchRenderQueue::statusProcessId("batchRender \"-status\" \"-1111 5704 \\\"\";\n"),
        5704);
    EXPECT_FALSE(MayaBatchRenderQueue::statusProcessId("batchRender"));
    EXPECT_FALSE(MayaBatchRenderQueue::statusProcessId("batchRender -status \"Rendering\""));
    EXPECT_FALSE(
        MayaBatchRenderQueue::statusProcessId(
            "batchRender -renderCommandOptions \"-interactive 1\""));
    EXPECT_FALSE(
        MayaBatchRenderQueue::statusProcessId("print \"batchRender -status \\\"1 2\\\"\""));
}

TEST(MayaBatchRenderQueueTest, OnlyCancellingOrStartingARenderStopsTheQueue)
{
    // The cancel procedure MayaHydra registers.
    EXPECT_TRUE(MayaBatchRenderQueue::stopsQueue("batchRender;\n"));
    EXPECT_TRUE(
        MayaBatchRenderQueue::stopsQueue(
            "batchRender \"-renderCommandOptions\" \"-interactive 1 -r HdStorm\";\n"));
    EXPECT_TRUE(MayaBatchRenderQueue::stopsQueue("batchRender -f \"C:/scene.mb\""));
    // The show procedure MayaHydra registers.
    EXPECT_FALSE(MayaBatchRenderQueue::stopsQueue("batchRender \"-showImage\" true;\n"));
    EXPECT_FALSE(
        MayaBatchRenderQueue::stopsQueue("batchRender \"-status\" \"-1111 5704 \\\"\";\n"));
    EXPECT_FALSE(MayaBatchRenderQueue::stopsQueue("print \"batchRender\""));
}

TEST(MayaBatchRenderQueueTest, NewSceneDropsPendingRenders)
{
    MayaBatchRenderQueue& queue = MayaBatchRenderQueue::instance();
    queue.start({ MString("print \"\""), MString("print \"\"") });
    EXPECT_EQ(queue.pendingCount(), 1u);

    MGlobal::executeCommand("file -force -new");
    EXPECT_EQ(queue.pendingCount(), 0u);
}
