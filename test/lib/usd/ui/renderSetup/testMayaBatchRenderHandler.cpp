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

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MSceneMessage.h>
#include <ufe/pathString.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
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

TEST_F(MayaBatchRenderHandlerTest, RenderAllCoversASavedRenderLayerThatIsNotLoaded)
{
    SdfLayerRefPtr layer = addFileRenderLayer("Unloaded");
    ASSERT_TRUE(layer);
    ASSERT_TRUE(layer->Save());
    const std::string identifier = layer->GetIdentifier();
    const std::string expectedSession
        = "-sessionLayer \\\"" + forwardSlashes(layer->GetRealPath()) + "\\\"";
    // An inactive render layer is not part of the stage, so nothing keeps it loaded.
    layer.Reset();
    ASSERT_FALSE(SdfLayer::Find(identifier));

    const std::vector<MString> commands = MayaBatchRenderAllHandler::batchRenderCommands(stage);

    ASSERT_EQ(commands.size(), 1u);
    EXPECT_NE(std::string(commands[0].asChar()).find(expectedSession), std::string::npos);
}

TEST_F(MayaBatchRenderHandlerTest, RenderAllCommandsRenderTheSceneCopy)
{
    SdfLayerRefPtr first = addFileRenderLayer("First");
    SdfLayerRefPtr second = addFileRenderLayer("Second");
    ASSERT_TRUE(first && second);

    const std::vector<MString> commands
        = MayaBatchRenderAllHandler::batchRenderCommands(stage, "C:/scenes/copy__1.ma");

    ASSERT_EQ(commands.size(), 2u);
    for (const MString& command : commands) {
        EXPECT_EQ(
            std::string(command.asChar())
                .rfind("batchRender -filename \"C:/scenes/copy__1.ma\" ", 0),
            0u);
    }
}

TEST_F(MayaBatchRenderHandlerTest, SceneCopyIsWrittenNextToTheScene)
{
    const std::string copyPath = MayaBatchRenderAllHandler::sceneCopyPath();
    ASSERT_FALSE(copyPath.empty());
    const std::filesystem::path copy(copyPath);
    const std::filesystem::path scene(MFileIO::currentFile().asChar());
    EXPECT_EQ(copy.parent_path(), scene.parent_path());
    int processId = 0;
    MGlobal::executeCommand("getpid", processId);
    EXPECT_EQ(copy.stem().string(), scene.stem().string() + "__" + std::to_string(processId));
    EXPECT_FALSE(copy.extension().empty());

    layerFiles.push_back(copy);
    ASSERT_TRUE(MayaBatchRenderAllHandler::exportSceneCopy(copyPath));
    EXPECT_TRUE(std::filesystem::exists(copy));
}

// The flag as Maya's export callbacks that keep Render Setup nodes and AOVs read it: they are
// Python, reading os.environ.
static std::string batchRenderExportFlag()
{
    return MGlobal::executePythonCommandStringResult(
               "__import__('os').environ.get('MAYA_BATCH_RENDER_EXPORT', '')")
        .asChar();
}

TEST_F(MayaBatchRenderHandlerTest, SceneCopyIsExportedForBatchRender)
{
    std::string       flagDuringExport;
    MStatus           status;
    const MCallbackId callbackId = MSceneMessage::addCallback(
        MSceneMessage::kBeforeExport,
        [](void* clientData) { *static_cast<std::string*>(clientData) = batchRenderExportFlag(); },
        &flagDuringExport,
        &status);
    ASSERT_TRUE(status);

    const std::string copyPath = MayaBatchRenderAllHandler::sceneCopyPath();
    ASSERT_FALSE(copyPath.empty());
    layerFiles.push_back(copyPath);
    const bool exported = MayaBatchRenderAllHandler::exportSceneCopy(copyPath);
    MMessage::removeCallback(callbackId);

    ASSERT_TRUE(exported);
    EXPECT_EQ(flagDuringExport, "1");
    EXPECT_EQ(batchRenderExportFlag(), "");
}

TEST_F(MayaBatchRenderHandlerTest, SceneCopyOverwritesAnExistingCopy)
{
    const std::string copyPath = MayaBatchRenderAllHandler::sceneCopyPath();
    ASSERT_FALSE(copyPath.empty());
    layerFiles.push_back(copyPath);
    ASSERT_TRUE(MayaBatchRenderAllHandler::exportSceneCopy(copyPath));

    EXPECT_TRUE(MayaBatchRenderAllHandler::exportSceneCopy(copyPath));
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

namespace {

// MEL command starting a 30 second child process of this process, standing in for the
// batch process batchRender spawns.
MString spawnChildCommand()
{
#ifdef _WIN32
    const char* args = "['ping', '-n', '30', '127.0.0.1']";
#else
    const char* args = "['sleep', '30']";
#endif
    return MString(
               "python(\"import subprocess, __main__; "
               "vars(__main__).setdefault('batchRenderQueueTestChildren', []).append("
               "subprocess.Popen(")
        + args + ", stdout=subprocess.DEVNULL))\")";
}

int childCount()
{
    int count = 0;
    MGlobal::executePythonCommand(
        "len(vars(__import__('__main__')).get('batchRenderQueueTestChildren', []))", count);
    return count;
}

// Whether every child spawned exits within a few seconds.
bool childrenExit()
{
    return MGlobal::executePythonCommandStringResult(
               "str(all(child.wait(10) is not None for child in "
               "vars(__import__('__main__')).get('batchRenderQueueTestChildren', [])))")
        == "True";
}

void killChildren()
{
    MGlobal::executePythonCommand(
        "import __main__\n"
        "for child in vars(__main__).pop('batchRenderQueueTestChildren', []):\n"
        "    child.kill()\n"
        "    child.wait()\n");
}

} // namespace

class MayaBatchRenderQueueTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        MayaBatchRenderQueue::instance().clear();
        killChildren();
        for (const auto& sceneCopy : sceneCopies) {
            std::error_code ignored;
            std::filesystem::remove(sceneCopy, ignored);
        }
    }

    static MayaBatchRenderQueue& queue() { return MayaBatchRenderQueue::instance(); }

    //! Creates an empty scene copy in the temporary directory, removed on tear down.
    std::filesystem::path createSceneCopy(const std::string& name)
    {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
        std::ofstream { path };
        sceneCopies.push_back(path);
        EXPECT_TRUE(std::filesystem::exists(path));
        return path;
    }

    std::vector<std::filesystem::path> sceneCopies;
};

TEST_F(MayaBatchRenderQueueTest, OnlyCancellingOrStartingARenderStopsTheQueue)
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

TEST_F(MayaBatchRenderQueueTest, LaunchWithoutBatchProcessDropsPendingRenders)
{
    queue().start({ MString("print \"\""), MString("print \"\"") });
    EXPECT_EQ(queue().pendingCount(), 0u);
}

TEST_F(MayaBatchRenderQueueTest, SpawnedProcessHoldsPendingRenders)
{
    queue().start({ spawnChildCommand(), MString("print \"\"") });
    ASSERT_EQ(queue().pendingCount(), 1u);
}

TEST_F(MayaBatchRenderQueueTest, NewSceneDropsPendingRenders)
{
    queue().start({ spawnChildCommand(), MString("print \"\"") });
    ASSERT_EQ(queue().pendingCount(), 1u);

    MGlobal::executeCommand("file -force -new");
    EXPECT_EQ(queue().pendingCount(), 0u);
}

TEST_F(MayaBatchRenderQueueTest, SceneCopyLastsUntilTheQueueIsDone)
{
    const std::filesystem::path copy = createSceneCopy("testMayaBatchRenderQueueCopy.ma");

    queue().start({ spawnChildCommand() }, copy.string());
    // The last render still reads it.
    EXPECT_TRUE(std::filesystem::exists(copy));

    queue().clear();
    EXPECT_FALSE(std::filesystem::exists(copy));
}

TEST_F(MayaBatchRenderQueueTest, NewSceneKeepsTheSceneCopyForTheRunningRender)
{
    const std::filesystem::path copy = createSceneCopy("testMayaBatchRenderQueueNewSceneCopy.ma");

    queue().start({ spawnChildCommand(), MString("print \"\"") }, copy.string());
    ASSERT_EQ(queue().pendingCount(), 1u);

    MGlobal::executeCommand("file -force -new");
    EXPECT_EQ(queue().pendingCount(), 0u);
    // The running render still reads it.
    EXPECT_TRUE(std::filesystem::exists(copy));

    queue().clear();
    EXPECT_FALSE(std::filesystem::exists(copy));
}

TEST_F(MayaBatchRenderQueueTest, CancelKillsTheRunningRenderAndDropsPendingRenders)
{
    const std::filesystem::path copy = createSceneCopy("testMayaBatchRenderQueueCancelCopy.ma");
    queue().start({ spawnChildCommand(), MString("print \"\"") }, copy.string());
    ASSERT_EQ(queue().pendingCount(), 1u);
    ASSERT_EQ(childCount(), 1);

    queue().cancel();
    EXPECT_EQ(queue().pendingCount(), 0u);
    EXPECT_TRUE(childrenExit());
    // Until the timer notices the exit, the killed render counts as running and keeps the copy.
    EXPECT_TRUE(queue().isRunning());
    EXPECT_TRUE(std::filesystem::exists(copy));

    queue().clear();
    EXPECT_FALSE(queue().isRunning());
    EXPECT_FALSE(std::filesystem::exists(copy));
}

TEST_F(MayaBatchRenderQueueTest, StartAfterCancelWaitsForTheCanceledRender)
{
    queue().start({ spawnChildCommand() });
    ASSERT_EQ(childCount(), 1);

    queue().cancel();
    queue().start({ spawnChildCommand() });
    // The timer starts it once Maya noticed the killed render exit.
    EXPECT_EQ(queue().pendingCount(), 1u);
    EXPECT_EQ(childCount(), 1);
    EXPECT_TRUE(queue().isRunning());
}

TEST_F(MayaBatchRenderQueueTest, RestartWithTheSameSceneCopyKeepsIt)
{
    const std::filesystem::path copy = createSceneCopy("testMayaBatchRenderQueueRestartCopy.ma");

    queue().start({ spawnChildCommand() }, copy.string());
    queue().start({ spawnChildCommand() }, copy.string());
    EXPECT_TRUE(std::filesystem::exists(copy));

    queue().clear();
    EXPECT_FALSE(std::filesystem::exists(copy));
}
