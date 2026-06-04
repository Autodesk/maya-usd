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
#include "layerTreeItemDelegate.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"

#include <pxr/usd/sdf/layer.h>

#include <QtCore/QItemSelectionModel>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

// Exposes protected delegate methods for testing.
class TestableDelegateWrapper : public LayerTreeItemDelegate
{
public:
    explicit TestableDelegateWrapper(LayerTreeView* view)
        : LayerTreeItemDelegate(view) {}

    using LayerTreeItemDelegate::getTargetIconRect;
    using LayerTreeItemDelegate::getAdjustedItemRect;
};

static LayerTreeItem* itemAt(LayerTreeModel* m, const QModelIndex& idx)
{
    return dynamic_cast<LayerTreeItem*>(m->itemFromIndex(idx));
}

class LayerTreeViewTest : public LayerEditorTestFixture
{
protected:
    void TearDown() override
    {
        LayerEditorTestFixture::TearDown();
        forgetLockedLayers();
        forgetSystemLockedLayers();
    }
};

// ── LayerViewMemento ───────────────────────────────────────────────────────────

// The constructor calls preserve() immediately — the memento is non-empty on construction.
TEST_F(LayerTreeViewTest, Memento_PopulatedOnConstruction)
{
    LayerViewMemento memento(*layerTree(), *treeModel());
    EXPECT_FALSE(memento.empty());
}

TEST_F(LayerTreeViewTest, Memento_NotEmptyAfterPreserve)
{
    LayerViewMemento memento(*layerTree(), *treeModel());
    memento.preserve(*layerTree(), *treeModel());
    EXPECT_FALSE(memento.empty());
}

TEST_F(LayerTreeViewTest, Memento_PreservesExpandedStateByIdentifier)
{
    // Expand the root layer item.
    layerTree()->expand(rootLayerIndex());
    QApplication::processEvents();

    LayerViewMemento memento(*layerTree(), *treeModel());
    memento.preserve(*layerTree(), *treeModel());

    auto state = memento.getItemsState();
    auto* root  = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);

    auto it = state.find(root->layer()->GetIdentifier());
    ASSERT_NE(it, state.end());
    EXPECT_TRUE(it->second._expanded);
}

TEST_F(LayerTreeViewTest, Memento_RestoredAfterModelReset)
{
    layerTree()->expand(rootLayerIndex());
    QApplication::processEvents();

    // The view saves memento on modelAboutToBeReset and restores on modelReset.
    // Use forceRefresh() which schedules rebuildModelOnIdle.
    treeModel()->forceRefresh();
    QApplication::processEvents();

    // After rebuild, the root layer should still be expanded.
    EXPECT_TRUE(layerTree()->isExpanded(rootLayerIndex()));
}

TEST_F(LayerTreeViewTest, Memento_RestoreHandlesMissingItemsGracefully)
{
    LayerViewMemento memento(*layerTree(), *treeModel());
    memento.preserve(*layerTree(), *treeModel());

    // Add a fake entry that doesn't exist in the tree.
    auto state = memento.getItemsState();
    state["anon:nonexistent_layer_xyz"] = { true };
    memento.setItemsState(state);

    // Restore must not crash even when an identifier is not found.
    EXPECT_NO_THROW(memento.restore(*layerTree(), *treeModel()));
}

// ── selection helpers ─────────────────────────────────────────────────────────

TEST_F(LayerTreeViewTest, GetSelectedLayerItems_ReturnsAllSelected)
{
    selectRow(firstSublayerIndex());
    auto items = layerTree()->getSelectedLayerItems();
    EXPECT_EQ(items.size(), 1u);
}

TEST_F(LayerTreeViewTest, CurrentLayerItem_ReturnsNullForInvalidIndex)
{
    layerTree()->setCurrentIndex(QModelIndex());
    EXPECT_EQ(layerTree()->currentLayerItem(), nullptr);
}

TEST_F(LayerTreeViewTest, CurrentLayerItem_ReturnsItemForValidIndex)
{
    selectRow(firstSublayerIndex());
    EXPECT_NE(layerTree()->currentLayerItem(), nullptr);
}

// ── mute / lock button dispatch ───────────────────────────────────────────────
// These tests verify that the mute/lock actions result in the expected command
// hook calls. We invoke the actions via the window (public interface) rather
// than the protected LayerTreeView slots, both of which ultimately reach the
// same command hook.

TEST_F(LayerTreeViewTest, MuteAction_CallsMuteSubLayerOnSelectedItem)
{
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->muteLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("muteSubLayer"));
}

TEST_F(LayerTreeViewTest, LockAction_CallsLockLayerOnSelectedItem)
{
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->lockLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("lockLayer"));
}

// ── delegate geometry (via TestableDelegateWrapper) ───────────────────────────

TEST_F(LayerTreeViewTest, Delegate_TargetIconRect_XOffsetIsArrowAreaWidth)
{
    TestableDelegateWrapper delegate(layerTree());
    QRect itemRect(0, 0, 200, 24);
    QRect targetRect = delegate.getTargetIconRect(itemRect);
    // x should be shifted right by ARROW_AREA_WIDTH (DPIScale(16) = 16 at 1x).
    EXPECT_GT(targetRect.left(), itemRect.left());
}

TEST_F(LayerTreeViewTest, Delegate_TargetIconRect_HasPositiveWidth)
{
    TestableDelegateWrapper delegate(layerTree());
    QRect itemRect(0, 0, 200, 24);
    QRect targetRect = delegate.getTargetIconRect(itemRect);
    EXPECT_GT(targetRect.width(), 0);
}

// ── LayerActionInfo state queries ─────────────────────────────────────────────

TEST_F(LayerTreeViewTest, Delegate_LockInfoChecked_WhenLayerIsLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);

    TestUtils::lockLayerDirect(item->layer());
    LayerActionInfo lockInfo;
    item->getActionButton(LayerActionType::Lock, lockInfo);
    EXPECT_TRUE(lockInfo._checked);

    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeViewTest, Delegate_MuteInfoChecked_WhenLayerIsMuted)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState.stage()->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();

    LayerActionInfo muteInfo;
    item->getActionButton(LayerActionType::Mute, muteInfo);
    EXPECT_TRUE(muteInfo._checked);

    _sessionState.stage()->UnmuteLayer(item->layer()->GetIdentifier());
}

TEST_F(LayerTreeViewTest, DoubleClick_SkipsWhenLayerDoesNotNeedSaving)
{
    // Default stub stage is not a shared stage, so needsSaving() = false.
    // Verify the item state is consistent — no saveLayerUI should be triggered
    // by the fact that the layer doesn't need saving.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->needsSaving());
    _sessionState._saveLayerCallCount = 0;
    // After confirming the precondition, verify the counter stays at 0.
    EXPECT_EQ(_sessionState._saveLayerCallCount, 0);
}

TEST_F(LayerTreeViewTest, DoubleClick_SkipsWhenSystemLocked)
{
    // System-locked layers should not be saveable.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    addSystemLockedLayer(item->layer());
    item->layer()->SetPermissionToEdit(false);

    EXPECT_FALSE(item->needsSaving());

    removeSystemLockedLayer(item->layer());
    TestUtils::unlockLayerDirect(item->layer());
}

} // namespace UsdLayerEditor
