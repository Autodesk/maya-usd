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
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace RenderSetupTest;

namespace {

const TfToken kCubeType("Cube");
const SdfPath kCubePath("/Cube");

std::vector<AdskUsdRenderSetup::HostStage> hostStages(const UsdStageRefPtr& stage)
{
    return { { stage, "stage", "stageId" } };
}

} // namespace

class MayaEditCommitterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        resetScene();
        stage = createProxyShapeStage();
        ASSERT_TRUE(stage);
        flushUndo();
    }

    void defineCube() { stage->DefinePrim(kCubePath, kCubeType); }

    UsdStageRefPtr                                         stage;
    std::unique_ptr<MayaUsdRenderSetup::MayaEditCommitter> committer
        = std::make_unique<MayaUsdRenderSetup::MayaEditCommitter>();
};

TEST_F(MayaEditCommitterTest, CommitWithoutStagesDoesNotRunTheEdit)
{
    bool ran = false;
    committer->commit("Edit", [&]() { ran = true; });
    EXPECT_FALSE(ran);
    EXPECT_TRUE(undoName().empty());
}

TEST_F(MayaEditCommitterTest, CommitWithoutEditDoesNothing)
{
    committer->setStages(hostStages(stage));
    committer->commit("Edit", nullptr);
    committer->commit("Edit", nullptr, { SdfLayer::CreateAnonymous() });
    EXPECT_TRUE(undoName().empty());
}

TEST_F(MayaEditCommitterTest, SetStagesIgnoresNullStages)
{
    committer->setStages({ { nullptr, "null", "nullId" } });

    bool ran = false;
    committer->commit("Edit", [&]() { ran = true; });
    EXPECT_FALSE(ran);
}

TEST_F(MayaEditCommitterTest, CommitIsOneUndoStepNamedAfterTheLabel)
{
    committer->setStages(hostStages(stage));

    committer->commit("Make_Cube", [&]() { defineCube(); });
    EXPECT_TRUE(stage->GetPrimAtPath(kCubePath));
    EXPECT_EQ(undoName(), "Make_Cube");

    undo();
    EXPECT_FALSE(stage->GetPrimAtPath(kCubePath));

    redo();
    EXPECT_TRUE(stage->GetPrimAtPath(kCubePath));
}

TEST_F(MayaEditCommitterTest, CommitWithLayersIsOneUndoStepNamedAfterTheLabel)
{
    committer->setStages(hostStages(stage));

    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    committer->commit("Make_Cube", [&]() { defineCube(); }, { layer });
    EXPECT_TRUE(stage->GetPrimAtPath(kCubePath));
    EXPECT_EQ(undoName(), "Make_Cube");

    undo();
    EXPECT_FALSE(stage->GetPrimAtPath(kCubePath));

    redo();
    EXPECT_TRUE(stage->GetPrimAtPath(kCubePath));
}

TEST_F(MayaEditCommitterTest, PinnedLayersStayAliveWhileTheUndoEntryExists)
{
    committer->setStages(hostStages(stage));

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfLayerHandle handle = layer;
    committer->commit("Make_Cube", [&]() { defineCube(); }, { handle });

    layer.Reset();
    EXPECT_TRUE(handle);

    // The undo stack outlives the committer.
    committer.reset();
    EXPECT_TRUE(handle);

    undo();
    EXPECT_TRUE(handle);

    redo();
    EXPECT_TRUE(handle);

    undo();
    EXPECT_TRUE(handle);
}

TEST_F(MayaEditCommitterTest, PinnedLayersAreReleasedWhenTheUndoStackIsFlushed)
{
    committer->setStages(hostStages(stage));

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfLayerHandle handle = layer;
    committer->commit("Make_Cube", [&]() { defineCube(); }, { handle });
    layer.Reset();
    ASSERT_TRUE(handle);

    flushUndo();
    EXPECT_FALSE(handle);
}

TEST_F(MayaEditCommitterTest, LayersAreNotPinnedByTheCommitWithoutLayers)
{
    committer->setStages(hostStages(stage));

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfLayerHandle handle = layer;
    committer->commit("Make_Cube", [&]() { defineCube(); });

    layer.Reset();
    EXPECT_FALSE(handle);
}

TEST_F(MayaEditCommitterTest, PinnedLayersAreNotKeptWhenTheEditIsSkipped)
{
    // No stages: the edit is skipped.
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfLayerHandle handle = layer;
    committer->commit("Make_Cube", [&]() { defineCube(); }, { handle });

    layer.Reset();
    EXPECT_FALSE(handle);
}

TEST_F(MayaEditCommitterTest, NullAndExpiredPinnedLayersAreIgnored)
{
    committer->setStages(hostStages(stage));

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfLayerHandle expired = layer;
    layer.Reset();
    ASSERT_FALSE(expired);

    committer->commit("Make_Cube", [&]() { defineCube(); }, { SdfLayerHandle(), expired });
    EXPECT_TRUE(stage->GetPrimAtPath(kCubePath));
    EXPECT_EQ(undoName(), "Make_Cube");
}

TEST_F(MayaEditCommitterTest, EditTargetChangeIsUndoneAndRedoneRepeatedly)
{
    committer->setStages(hostStages(stage));
    const SdfLayerRefPtr sub = SdfLayer::CreateAnonymous();
    stage->GetRootLayer()->InsertSubLayerPath(sub->GetIdentifier());
    const UsdEditTarget root = stage->GetEditTarget();
    flushUndo();

    committer->commit("Target", [&]() { stage->SetEditTarget(UsdEditTarget(sub)); });
    ASSERT_EQ(stage->GetEditTarget().GetLayer(), sub);

    for (int i = 0; i < 3; ++i) {
        undo();
        EXPECT_EQ(stage->GetEditTarget(), root) << i;
        redo();
        EXPECT_EQ(stage->GetEditTarget().GetLayer(), sub) << i;
    }
}

TEST_F(MayaEditCommitterTest, EditTargetOnALayerAddedByTheEditIsRestored)
{
    committer->setStages(hostStages(stage));
    const UsdEditTarget  root = stage->GetEditTarget();
    const SdfLayerRefPtr sub = SdfLayer::CreateAnonymous();
    flushUndo();

    // The layer joins the stage during the edit; undo removes it and redo adds it back,
    // inside a change block, so the edit target can only be set after recomposition.
    committer->commit(
        "Target",
        [&]() {
            stage->GetRootLayer()->InsertSubLayerPath(sub->GetIdentifier());
            stage->SetEditTarget(UsdEditTarget(sub));
        },
        { sub });
    ASSERT_EQ(stage->GetEditTarget().GetLayer(), sub);

    for (int i = 0; i < 2; ++i) {
        undo();
        EXPECT_EQ(stage->GetEditTarget(), root) << i;
        redo();
        EXPECT_EQ(stage->GetEditTarget().GetLayer(), sub) << i;
    }
}

TEST_F(MayaEditCommitterTest, UnchangedEditTargetIsNotTouchedByUndo)
{
    committer->setStages(hostStages(stage));
    const SdfLayerRefPtr sub = SdfLayer::CreateAnonymous();
    stage->GetRootLayer()->InsertSubLayerPath(sub->GetIdentifier());
    flushUndo();

    committer->commit("Make_Cube", [&]() { defineCube(); });

    // Changed by the user outside of any commit.
    stage->SetEditTarget(UsdEditTarget(sub));
    undo();
    EXPECT_EQ(stage->GetEditTarget().GetLayer(), sub);
    redo();
    EXPECT_EQ(stage->GetEditTarget().GetLayer(), sub);
}

TEST_F(MayaEditCommitterTest, EditTargetsOfMultipleStagesAreRestored)
{
    const UsdStageRefPtr stage2 = createProxyShapeStage("shape2");
    ASSERT_TRUE(stage2);
    ASSERT_NE(stage, stage2);

    const SdfLayerRefPtr sub1 = SdfLayer::CreateAnonymous();
    const SdfLayerRefPtr sub2 = SdfLayer::CreateAnonymous();
    stage->GetRootLayer()->InsertSubLayerPath(sub1->GetIdentifier());
    stage2->GetRootLayer()->InsertSubLayerPath(sub2->GetIdentifier());
    const UsdEditTarget root1 = stage->GetEditTarget();
    const UsdEditTarget root2 = stage2->GetEditTarget();
    flushUndo();

    committer->setStages({ { stage, "stage", "stageId" }, { stage2, "stage2", "stageId2" } });
    committer->commit("Target", [&]() {
        stage->SetEditTarget(UsdEditTarget(sub1));
        stage2->SetEditTarget(UsdEditTarget(sub2));
    });

    undo();
    EXPECT_EQ(stage->GetEditTarget(), root1);
    EXPECT_EQ(stage2->GetEditTarget(), root2);

    redo();
    EXPECT_EQ(stage->GetEditTarget().GetLayer(), sub1);
    EXPECT_EQ(stage2->GetEditTarget().GetLayer(), sub2);
}

TEST_F(MayaEditCommitterTest, SessionLayerEditsAreUndoneAndRedone)
{
    committer->setStages(hostStages(stage));
    flushUndo();

    committer->commit("Session_Cube", [&]() {
        const UsdEditContext context(stage, stage->GetSessionLayer());
        defineCube();
    });
    ASSERT_TRUE(stage->GetSessionLayer()->GetPrimAtPath(kCubePath));

    undo();
    EXPECT_FALSE(stage->GetSessionLayer()->GetPrimAtPath(kCubePath));
    redo();
    EXPECT_TRUE(stage->GetSessionLayer()->GetPrimAtPath(kCubePath));
}
