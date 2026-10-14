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
