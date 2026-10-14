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

#include <mayaUsdUI/ui/mayaEditCommitter.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace RenderSetupTest;
using AdskUsdRenderSetup::RenderLayerInfo;
using AdskUsdRenderSetup::RenderLayerManager;

namespace {

const TfToken kCubeType("Cube");

} // namespace

// Drives the render layer manager through the Maya edit committer, so every
// operation goes onto the Maya undo stack like it does from the Render Setup UI.
class MayaRenderLayersTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        resetScene();
        stage = createProxyShapeStage();
        ASSERT_TRUE(stage);
        committer.setStages({ { stage, "stage", "stageId" } });
        manager().attach(stage);
        flushUndo();
    }

    void TearDown() override
    {
        manager().detach(stage);
        flushUndo();
    }

    static RenderLayerManager& manager() { return RenderLayerManager::instance(); }

    std::string create(const std::string& name = {})
    {
        return manager().createRenderLayer(stage, name, committer);
    }

    std::optional<RenderLayerInfo> find(const std::string& name) const
    {
        const std::vector<RenderLayerInfo> layers = manager().getRenderLayers(stage);
        const auto                         it
            = std::find_if(layers.begin(), layers.end(), [&](const RenderLayerInfo& info) {
                  return info.name == name;
              });
        return it == layers.end() ? std::nullopt : std::optional<RenderLayerInfo>(*it);
    }

    bool exists(const std::string& name) const { return find(name).has_value(); }

    std::string activeName() const
    {
        const std::optional<RenderLayerInfo> active = manager().getActiveRenderLayer(stage);
        return active ? active->name : std::string();
    }

    std::string editTargetIdentifier() const
    {
        return stage->GetEditTarget().GetLayer()->GetIdentifier();
    }

    UsdStageRefPtr                        stage;
    MayaUsdRenderSetup::MayaEditCommitter committer;
};

TEST_F(MayaRenderLayersTest, CreateAllocatesANameAndListsTheLayer)
{
    EXPECT_TRUE(manager().getRenderLayers(stage).empty());

    const std::string name = create();
    ASSERT_FALSE(name.empty());
    EXPECT_TRUE(exists(name));
    EXPECT_EQ(manager().getRenderLayers(stage).size(), 1u);
}

TEST_F(MayaRenderLayersTest, CreateHonorsTheRequestedName)
{
    EXPECT_EQ(create("Beauty"), "Beauty");
    EXPECT_TRUE(exists("Beauty"));
}

TEST_F(MayaRenderLayersTest, CreateDoesNotActivateOrTargetTheLayer)
{
    const std::string rootIdentifier = editTargetIdentifier();
    create("Beauty");

    EXPECT_TRUE(activeName().empty());
    EXPECT_EQ(editTargetIdentifier(), rootIdentifier);
}

TEST_F(MayaRenderLayersTest, CreateIsOneUndoStep)
{
    const std::string name = create("Beauty");
    ASSERT_EQ(name, "Beauty");
    EXPECT_FALSE(undoName().empty());

    undo();
    EXPECT_FALSE(exists(name));

    redo();
    EXPECT_TRUE(exists(name));
}

TEST_F(MayaRenderLayersTest, CreatedLayerSurvivesRepeatedUndoRedo)
{
    const std::string name = create("Beauty");
    ASSERT_TRUE(exists(name));

    // The window can go away while the undo entry stays on Maya's undo stack.
    for (int i = 0; i < 3; ++i) {
        undo();
        EXPECT_FALSE(exists(name)) << i;
        redo();
        ASSERT_TRUE(exists(name)) << i;
    }

    // The restored layer is usable: it can be targeted.
    manager().setTargetRenderLayer(stage, name, committer);
    EXPECT_EQ(editTargetIdentifier(), find(name)->identifier);
}

TEST_F(MayaRenderLayersTest, DeleteRemovesTheLayerAndIsUndoable)
{
    const std::string name = create("Beauty");
    flushUndo();

    manager().deleteRenderLayer(stage, name, committer);
    EXPECT_FALSE(exists(name));
    EXPECT_FALSE(undoName().empty());

    undo();
    EXPECT_TRUE(exists(name));

    redo();
    EXPECT_FALSE(exists(name));
}

TEST_F(MayaRenderLayersTest, DeleteOnlyRemovesTheNamedLayer)
{
    create("Beauty");
    create("Shadow");

    manager().deleteRenderLayer(stage, "Beauty", committer);
    EXPECT_FALSE(exists("Beauty"));
    EXPECT_TRUE(exists("Shadow"));
}

TEST_F(MayaRenderLayersTest, DeleteOfTheActiveLayerClearsTheActiveLayer)
{
    const std::string name = create("Beauty");
    manager().setActiveRenderLayer(stage, name, committer);
    ASSERT_EQ(activeName(), name);

    manager().deleteRenderLayer(stage, name, committer);
    EXPECT_TRUE(activeName().empty());
}

TEST_F(MayaRenderLayersTest, DeleteOfAnUnknownLayerChangesNothing)
{
    create("Beauty");
    flushUndo();

    manager().deleteRenderLayer(stage, "NoSuchLayer", committer);
    EXPECT_TRUE(exists("Beauty"));
    EXPECT_EQ(manager().getRenderLayers(stage).size(), 1u);
}

TEST_F(MayaRenderLayersTest, TargetActivatesTheLayerAndSetsTheEditTarget)
{
    const std::string name = create("Beauty");
    manager().setTargetRenderLayer(stage, name, committer);

    EXPECT_EQ(activeName(), name);
    EXPECT_EQ(editTargetIdentifier(), find(name)->identifier);
}

TEST_F(MayaRenderLayersTest, EditsWhileTargetedLandInTheRenderLayer)
{
    const std::string rootIdentifier = editTargetIdentifier();
    const std::string name = create("Beauty");
    manager().setTargetRenderLayer(stage, name, committer);

    const SdfPath cubePath("/Cube");
    stage->DefinePrim(cubePath, kCubeType);

    const SdfLayerHandle renderLayer = stage->GetEditTarget().GetLayer();
    EXPECT_EQ(renderLayer->GetIdentifier(), find(name)->identifier);
    EXPECT_TRUE(renderLayer->GetPrimAtPath(cubePath));
    EXPECT_FALSE(stage->GetRootLayer()->GetPrimAtPath(cubePath));
}

TEST_F(MayaRenderLayersTest, TargetingAnotherLayerMovesTheEditTarget)
{
    create("Beauty");
    create("Shadow");

    manager().setTargetRenderLayer(stage, "Beauty", committer);
    EXPECT_EQ(editTargetIdentifier(), find("Beauty")->identifier);

    manager().setTargetRenderLayer(stage, "Shadow", committer);
    EXPECT_EQ(activeName(), "Shadow");
    EXPECT_EQ(editTargetIdentifier(), find("Shadow")->identifier);
}

TEST_F(MayaRenderLayersTest, UntargetingReturnsTheEditTargetToTheRootLayer)
{
    const std::string rootIdentifier = editTargetIdentifier();
    const std::string name = create("Beauty");
    manager().setTargetRenderLayer(stage, name, committer);
    ASSERT_NE(editTargetIdentifier(), rootIdentifier);

    manager().setTargetRenderLayer(stage, "", committer);
    EXPECT_TRUE(activeName().empty());
    EXPECT_EQ(editTargetIdentifier(), rootIdentifier);
}

TEST_F(MayaRenderLayersTest, TargetIsUndoable)
{
    const std::string rootIdentifier = editTargetIdentifier();
    const std::string name = create("Beauty");
    flushUndo();

    manager().setTargetRenderLayer(stage, name, committer);
    ASSERT_EQ(editTargetIdentifier(), find(name)->identifier);
    EXPECT_FALSE(undoName().empty());

    undo();
    EXPECT_TRUE(activeName().empty());
    EXPECT_EQ(editTargetIdentifier(), rootIdentifier);

    redo();
    EXPECT_EQ(activeName(), name);
    EXPECT_EQ(editTargetIdentifier(), find(name)->identifier);
}

TEST_F(MayaRenderLayersTest, DeleteOfTheTargetedLayerReturnsTheEditTargetToTheRootLayer)
{
    const std::string rootIdentifier = editTargetIdentifier();
    const std::string name = create("Beauty");
    manager().setTargetRenderLayer(stage, name, committer);
    ASSERT_NE(editTargetIdentifier(), rootIdentifier);

    manager().deleteRenderLayer(stage, name, committer);
    EXPECT_EQ(editTargetIdentifier(), rootIdentifier);
}
