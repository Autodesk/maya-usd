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

#include "testFixture.h"
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
    using LayerTreeItemDelegate::actionAppearsChecked;
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

TEST_F(LayerTreeViewTest, Memento_EmptyBeforePreserve)
{
    LayerViewMemento memento(*layerTree(), *treeModel());
    EXPECT_TRUE(memento.empty());
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
    treeModel()->rebuildModel();
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

// ── double-click behavior ─────────────────────────────────────────────────────

TEST_F(LayerTreeViewTest, DoubleClick_SkipsWhenLayerDoesNotNeedSaving)
{
    // Default stub stage is not a shared stage, so needsSaving() = false.
    // Double-click should not invoke saveLayerUI.
    _sessionState._saveLayerCallCount = 0;
    layerTree()->onItemDoubleClicked(firstSublayerIndex());
    QApplication::processEvents();
    EXPECT_EQ(_sessionState._saveLayerCallCount, 0);
}

TEST_F(LayerTreeViewTest, DoubleClick_SkipsWhenSystemLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    addSystemLockedLayer(item->layer());
    item->layer()->SetPermissionToEdit(false);

    _sessionState._saveLayerCallCount = 0;
    layerTree()->onItemDoubleClicked(firstSublayerIndex());
    QApplication::processEvents();
    EXPECT_EQ(_sessionState._saveLayerCallCount, 0);

    removeSystemLockedLayer(item->layer());
    TestUtils::unlockLayerDirect(item->layer());
}

// ── mute / lock button dispatch ───────────────────────────────────────────────

TEST_F(LayerTreeViewTest, MuteButton_CallsMuteSubLayerOnSelectedItem)
{
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    layerTree()->onMuteLayerButtonPushed();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("muteSubLayer"));
}

TEST_F(LayerTreeViewTest, LockButton_CallsLockLayerOnSelectedItem)
{
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    layerTree()->onLockLayerButtonPushed();
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

} // namespace UsdLayerEditor
