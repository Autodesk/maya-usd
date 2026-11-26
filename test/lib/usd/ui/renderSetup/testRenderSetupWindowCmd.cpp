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

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsdUI/ui/renderSetupWindowCmd.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/selection.h>

#include <gtest/gtest.h>

#include <initializer_list>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using RenderSetupTest::createProxyShapeStage;

// Selection of the scene items at `paths`, in order. A path without a scene
// item is reported as a failure and left out of the selection.
Ufe::Selection selectionOf(std::initializer_list<Ufe::Path> paths)
{
    Ufe::Selection selection;
    for (const Ufe::Path& path : paths) {
        const Ufe::SceneItem::Ptr item = Ufe::Hierarchy::createItem(path);
        EXPECT_TRUE(item) << path.string();
        if (item) {
            selection.append(item);
        }
    }
    return selection;
}

Ufe::Path primPath(const UsdStageRefPtr& stage, const char* primName)
{
    return Ufe::PathString::path(
        Ufe::PathString::string(MayaUsd::ufe::stagePath(stage)) + "," + primName);
}

Ufe::Path perspPath() { return Ufe::PathString::path("|persp"); }

} // namespace

class RenderSetupWindowCmdTest : public ::testing::Test
{
protected:
    void SetUp() override { RenderSetupTest::resetScene(); }
};

TEST_F(RenderSetupWindowCmdTest, EmptySelectionHasNoStage)
{
    EXPECT_FALSE(MayaUsd::RenderSetupWindowCmd::selectedStage(Ufe::Selection()));
}

TEST_F(RenderSetupWindowCmdTest, MayaNodeHasNoStage)
{
    // With a stage in the scene, a null result means persp itself was not matched to it.
    ASSERT_TRUE(createProxyShapeStage());

    const Ufe::Selection selection = selectionOf({ perspPath() });
    ASSERT_FALSE(selection.empty());
    EXPECT_FALSE(MayaUsd::RenderSetupWindowCmd::selectedStage(selection));
}

TEST_F(RenderSetupWindowCmdTest, MayaNodeBeforePrimSelectsThePrimStage)
{
    const UsdStageRefPtr stage = createProxyShapeStage();
    ASSERT_TRUE(stage);
    stage->DefinePrim(SdfPath("/A"));

    const Ufe::Selection selection = selectionOf({ perspPath(), primPath(stage, "/A") });
    ASSERT_EQ(selection.size(), 2u);
    EXPECT_EQ(MayaUsd::RenderSetupWindowCmd::selectedStage(selection), stage);
}

TEST_F(RenderSetupWindowCmdTest, PrimSelectsItsStage)
{
    const UsdStageRefPtr stageA = createProxyShapeStage();
    const UsdStageRefPtr stageB = createProxyShapeStage();
    ASSERT_TRUE(stageA);
    ASSERT_TRUE(stageB);
    stageB->DefinePrim(SdfPath("/A"));

    const Ufe::Selection selection = selectionOf({ primPath(stageB, "/A") });
    ASSERT_FALSE(selection.empty());
    EXPECT_EQ(MayaUsd::RenderSetupWindowCmd::selectedStage(selection), stageB);
}

TEST_F(RenderSetupWindowCmdTest, ProxyShapeSelectsItsStage)
{
    const UsdStageRefPtr stageA = createProxyShapeStage();
    const UsdStageRefPtr stageB = createProxyShapeStage();
    ASSERT_TRUE(stageA);
    ASSERT_TRUE(stageB);

    const Ufe::Selection selection = selectionOf({ MayaUsd::ufe::stagePath(stageB) });
    ASSERT_FALSE(selection.empty());
    EXPECT_EQ(MayaUsd::RenderSetupWindowCmd::selectedStage(selection), stageB);
}

TEST_F(RenderSetupWindowCmdTest, ProxyTransformSelectsItsStage)
{
    const UsdStageRefPtr stageA = createProxyShapeStage();
    const UsdStageRefPtr stageB = createProxyShapeStage();
    ASSERT_TRUE(stageA);
    ASSERT_TRUE(stageB);

    const Ufe::Selection selection = selectionOf({ MayaUsd::ufe::stagePath(stageB).pop() });
    ASSERT_FALSE(selection.empty());
    EXPECT_EQ(MayaUsd::RenderSetupWindowCmd::selectedStage(selection), stageB);
}

TEST_F(RenderSetupWindowCmdTest, FirstStageInSelectionWins)
{
    const UsdStageRefPtr stageA = createProxyShapeStage();
    const UsdStageRefPtr stageB = createProxyShapeStage();
    ASSERT_TRUE(stageA);
    ASSERT_TRUE(stageB);
    ASSERT_NE(stageA, stageB);
    stageA->DefinePrim(SdfPath("/A"));
    stageB->DefinePrim(SdfPath("/A"));
    const Ufe::Path primOnA = primPath(stageA, "/A");
    const Ufe::Path primOnB = primPath(stageB, "/A");

    const Ufe::Selection aThenB = selectionOf({ primOnA, primOnB });
    ASSERT_EQ(aThenB.size(), 2u);
    EXPECT_EQ(MayaUsd::RenderSetupWindowCmd::selectedStage(aThenB), stageA);

    const Ufe::Selection bThenA = selectionOf({ primOnB, primOnA });
    ASSERT_EQ(bThenA.size(), 2u);
    EXPECT_EQ(MayaUsd::RenderSetupWindowCmd::selectedStage(bThenA), stageB);
}
