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
#pragma once

#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
#include "testFixture.h"
#endif
#include "testUtils.h"
#include "layerLocking.h"
#include "layerTreeItem.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QApplication>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

static LayerTreeItem* itemAt(LayerTreeModel* model, const QModelIndex& idx)
{
    return dynamic_cast<LayerTreeItem*>(model->itemFromIndex(idx));
}

class LayerTreeItemTest : public LayerEditorTestFixture
{
protected:
    void TearDown() override
    {
        LayerEditorTestFixture::TearDown();
        forgetLockedLayers();
        forgetSystemLockedLayers();
    }
};

// ── isMuted / appearsMuted ────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsMuted_ReturnsFalseByDefault)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMuted());
}

TEST_F(LayerTreeItemTest, IsMuted_ReturnsTrueAfterStageMute)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    auto  stage = _sessionState.stage();
    stage->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();
    EXPECT_TRUE(item->isMuted());
    stage->UnmuteLayer(item->layer()->GetIdentifier());
}

TEST_F(LayerTreeItemTest, AppearsMuted_FalseWhenNeitherSelfNorParentMuted)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->appearsMuted());
}

TEST_F(LayerTreeItemTest, AppearsMuted_TrueWhenSelfIsMuted)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    auto  stage = _sessionState.stage();
    stage->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();
    EXPECT_TRUE(item->appearsMuted());
    stage->UnmuteLayer(item->layer()->GetIdentifier());
}

// ── isReadOnly ────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsReadOnly_FalseForNormalSublayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isReadOnly());
}

// ── isDirty / needsSaving ─────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsDirty_FalseForCleanLayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isDirty());
}

TEST_F(LayerTreeItemTest, IsDirty_TrueAfterLayerModified)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    item->layer()->SetComment("mark dirty");
    EXPECT_TRUE(item->isDirty());
}

TEST_F(LayerTreeItemTest, NeedsSaving_FalseForSessionLayer)
{
    auto* item = itemAt(treeModel(), sessionLayerIndex());
    ASSERT_NE(item, nullptr);
    item->layer()->SetComment("mark dirty");
    // Session layer: needsSaving always false regardless of dirty state.
    EXPECT_FALSE(item->needsSaving());
}

// ── isLocked / appearsLocked ──────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsLocked_FalseByDefault)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isLocked());
}

TEST_F(LayerTreeItemTest, IsLocked_TrueWhenPermissionToEditRevoked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    EXPECT_TRUE(item->isLocked());
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, AppearsLocked_FalseForRootItemWithUnlockedSelf)
{
    auto* item = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->appearsLocked());
}

TEST_F(LayerTreeItemTest, AppearsLocked_TrueWhenParentIsLocked)
{
    // The sublayer's parent in the tree is the root layer item.
    auto* parentItem = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(parentItem, nullptr);
    TestUtils::lockLayerDirect(parentItem->layer());

    auto* child = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(child, nullptr);
    EXPECT_TRUE(child->appearsLocked());

    TestUtils::unlockLayerDirect(parentItem->layer());
}

TEST_F(LayerTreeItemTest, AppearsLocked_DoesNotCheckSelf)
{
    // A locked item does NOT report appearsLocked for itself — only propagation from parent.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    EXPECT_FALSE(item->appearsLocked());
    TestUtils::unlockLayerDirect(item->layer());
}

// ── isSystemLocked / appearsSystemLocked ──────────────────────────────────────

TEST_F(LayerTreeItemTest, IsSystemLocked_FalseByDefault)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isSystemLocked());
}

TEST_F(LayerTreeItemTest, IsSystemLocked_TrueAfterSystemLockApplied)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    addSystemLockedLayer(item->layer());
    item->layer()->SetPermissionToEdit(false);
    EXPECT_TRUE(item->isSystemLocked());
    removeSystemLockedLayer(item->layer());
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, AppearsSystemLocked_FalseWhenParentNotSystemLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->appearsSystemLocked());
}

// ── isMovable ─────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsMovable_FalseForSessionLayer)
{
    auto* item = itemAt(treeModel(), sessionLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMovable());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseForRootLayer)
{
    auto* item = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMovable());
}

TEST_F(LayerTreeItemTest, IsMovable_TrueForNormalSublayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isMovable());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseWhenLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    EXPECT_FALSE(item->isMovable());
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseWhenAppearsLocked)
{
    auto* parent = itemAt(treeModel(), rootLayerIndex());
    TestUtils::lockLayerDirect(parent->layer());
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMovable());
    TestUtils::unlockLayerDirect(parent->layer());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseWhenMuted)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState.stage()->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();
    EXPECT_FALSE(item->isMovable());
    _sessionState.stage()->UnmuteLayer(item->layer()->GetIdentifier());
}

// ── misc ──────────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsTargetLayer_TrueForCurrentEditTarget)
{
    // Root layer is the default edit target.
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->isTargetLayer());
}

TEST_F(LayerTreeItemTest, HasSubLayers_TrueWhenSublayersExist)
{
    // StubSessionState creates a root layer with one sublayer.
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->hasSubLayers());
}

TEST_F(LayerTreeItemTest, HasSubLayers_FalseForLeafSublayer)
{
    auto* sub = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(sub, nullptr);
    EXPECT_FALSE(sub->hasSubLayers());
}

TEST_F(LayerTreeItemTest, IsAnonymous_TrueForAnonymousLayer)
{
    auto* sub = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(sub, nullptr);
    // StubSessionState creates anonymous sublayers.
    EXPECT_TRUE(sub->isAnonymous());
}

TEST_F(LayerTreeItemTest, GetActionButton_LockCheckedMatchesIsLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());

    LayerActionInfo info;
    item->getActionButton(LayerActionType::Lock, info);
    EXPECT_TRUE(info._checked);

    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, GetActionButton_MuteCheckedMatchesIsMuted)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState.stage()->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();

    LayerActionInfo info;
    item->getActionButton(LayerActionType::Mute, info);
    EXPECT_TRUE(info._checked);

    _sessionState.stage()->UnmuteLayer(item->layer()->GetIdentifier());
}

TEST_F(LayerTreeItemTest, ActionButtons_MuteAppliesToSublayerOnly)
{
    const auto& buttons = LayerTreeItem::actionButtonsDefinition();
    auto        it      = buttons.find(LayerActionType::Mute);
    ASSERT_NE(it, buttons.end());
    EXPECT_TRUE(IsLayerActionAllowed(it->second, LayerMasks_SubLayer));
    EXPECT_FALSE(IsLayerActionAllowed(it->second, LayerMasks_Root));
}

TEST_F(LayerTreeItemTest, ActionButtons_LockAppliesToRootAndSublayer)
{
    const auto& buttons = LayerTreeItem::actionButtonsDefinition();
    auto        it      = buttons.find(LayerActionType::Lock);
    ASSERT_NE(it, buttons.end());
    EXPECT_TRUE(IsLayerActionAllowed(it->second, LayerMasks_Root));
    EXPECT_TRUE(IsLayerActionAllowed(it->second, LayerMasks_SubLayer));
}

// ── isIdenticalItem ────────────────────────────────────────────────────────────

#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
TEST_F(LayerTreeItemTest, IsIdenticalItem_NullOtherReturnsFalse)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIdenticalItem(nullptr));
}

TEST_F(LayerTreeItemTest, IsIdenticalItem_SamePointerReturnsTrue)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIdenticalItem(item));
}

TEST_F(LayerTreeItemTest, IsIdenticalItem_DifferentLayerReturnsFalse)
{
    // Root layer item vs first sublayer item — different layers.
    auto* root = itemAt(treeModel(), treeModel()->rootLayerIndex());
    ASSERT_NE(root, nullptr);
    auto* sub = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(sub, nullptr);
    EXPECT_FALSE(root->isIdenticalItem(sub));
}
#endif

} // namespace UsdLayerEditor
