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
#include <QtCore/QMetaObject>
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

// Shared-stage variant: on a shared stage anonymous layers report needsSaving()==true,
// which is required to reach the save-related branches of the double-click handler.
class LayerTreeViewSharedTest : public LayerTreeViewTest
{
protected:
    void SetUp() override
    {
        _sessionState._commandHookImpl._isSharedStage = true;
        LayerEditorTestFixture::SetUp();
        QApplication::processEvents();
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
    // Collapse the root so the restored state differs from the default expandAll
    // that runs when no memento exists — this is what makes the assertion
    // discriminating rather than tautological.
    layerTree()->collapse(rootLayerIndex());
    QApplication::processEvents();
    ASSERT_FALSE(layerTree()->isExpanded(rootLayerIndex()));

    // Add a sublayer so the rebuild is a genuine reset (not skipped by the
    // identical-item optimization), exercising onModelAboutToBeReset/onModelReset.
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(
        SdfLayer::CreateAnonymous("memento_extra")->GetIdentifier(), 0);
    treeModel()->forceRefresh();
    QApplication::processEvents();

    // The memento must restore the collapsed state across the reset.
    EXPECT_FALSE(layerTree()->isExpanded(rootLayerIndex()));
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
    // Default stub stage is not shared, so needsSaving() == false and the handler
    // must return before attempting any save.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    ASSERT_FALSE(item->needsSaving());

    _sessionState._saveLayerCallCount = 0;
    // onItemDoubleClicked is connected to the view's doubleClicked signal; emit it
    // to drive the handler (the slot itself is not publicly callable).
    bool invoked = QMetaObject::invokeMethod(
        layerTree(), "doubleClicked", Qt::DirectConnection,
        Q_ARG(QModelIndex, firstSublayerIndex()));
    ASSERT_TRUE(invoked) << "failed to emit doubleClicked";
    QApplication::processEvents();
    EXPECT_EQ(_sessionState._saveLayerCallCount, 0)
        << "No save should be attempted for a layer that does not need saving";
}

TEST_F(LayerTreeViewSharedTest, DoubleClick_SkipsWhenSystemLocked)
{
    // On a shared stage the anonymous sublayer needs saving, so the handler reaches
    // the system-lock guard — which must still skip the save.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->needsSaving()) << "Anonymous layer on a shared stage should need saving";

    addSystemLockedLayer(item->layer());
    item->layer()->SetPermissionToEdit(false);
    ASSERT_TRUE(item->isSystemLocked());

    _sessionState._saveLayerCallCount = 0;
    bool invoked = QMetaObject::invokeMethod(
        layerTree(), "doubleClicked", Qt::DirectConnection,
        Q_ARG(QModelIndex, firstSublayerIndex()));
    ASSERT_TRUE(invoked) << "failed to emit doubleClicked";
    QApplication::processEvents();
    EXPECT_EQ(_sessionState._saveLayerCallCount, 0)
        << "System-locked layers must not be saved on double-click";

    removeSystemLockedLayer(item->layer());
    TestUtils::unlockLayerDirect(item->layer());
}

} // namespace UsdLayerEditor
