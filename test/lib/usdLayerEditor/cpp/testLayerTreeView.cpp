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

#include <testFixture.h>
#include "testUtils.h"
#include "layerLocking.h"
#include "layerTreeItem.h"
#include "layerTreeItemDelegate.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"

#include <pxr/usd/sdf/layer.h>

#include <QtCore/QItemSelectionModel>
#include <QtCore/QMetaObject>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>

#include <algorithm>

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
        setSharedStage(true);
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

TEST_F(LayerTreeViewTest, Memento_PreservesExpandedStateByIdentifier)
{
    // Expand the root layer item.
    layerTree()->expand(rootLayerIndex());
    QApplication::processEvents();

    LayerViewMemento memento(*layerTree(), *treeModel());
    memento.preserve(*layerTree(), *treeModel());

    auto state = memento.getItemsState();
    auto* root  = treeModel()->layerItemFromIndex(rootLayerIndex());
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
    // Collapse the root so a restore that re-expands it is observable.
    layerTree()->collapse(rootLayerIndex());
    QApplication::processEvents();
    ASSERT_FALSE(layerTree()->isExpanded(rootLayerIndex()));

    LayerViewMemento memento(*layerTree(), *treeModel());
    memento.preserve(*layerTree(), *treeModel());

    auto* root = treeModel()->layerItemFromIndex(rootLayerIndex());
    ASSERT_NE(root, nullptr);
    const std::string rootId = root->layer()->GetIdentifier();

    auto state = memento.getItemsState();
    // Set a real layer's expanded state so a successful restore is verifiable.
    state[rootId]._expanded = true;
    // Plus a fake entry that doesn't exist in the tree.
    state["anon:nonexistent_layer_xyz"] = { true };
    memento.setItemsState(state);

    const int rowsBefore = treeModel()->rowCount(rootLayerIndex());

    // Restore must not crash even when an identifier is not found.
    EXPECT_NO_THROW(memento.restore(*layerTree(), *treeModel()));

    // The real layer's expanded state was applied.
    EXPECT_TRUE(layerTree()->isExpanded(rootLayerIndex()));
    // The bogus entry created no row.
    EXPECT_EQ(treeModel()->rowCount(rootLayerIndex()), rowsBefore);
}

// ── selection helpers ─────────────────────────────────────────────────────────

TEST_F(LayerTreeViewTest, GetSelectedLayerItems_ReturnsAllSelected)
{
    selectRow(firstSublayerIndex());
    auto items = layerTree()->getSelectedLayerItems();
    EXPECT_EQ(items.size(), 1u);
}

TEST_F(LayerTreeViewTest, GetSelectedLayerItems_ReturnsAllSelectedForMultiSelection)
{
    selectRow(firstSublayerIndex());
    layerTree()->selectionModel()->select(
        rootLayerIndex(), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    QApplication::processEvents();

    auto* sub  = treeModel()->layerItemFromIndex(firstSublayerIndex());
    auto* root = treeModel()->layerItemFromIndex(rootLayerIndex());
    ASSERT_NE(sub, nullptr);
    ASSERT_NE(root, nullptr);

    auto items = layerTree()->getSelectedLayerItems();
    ASSERT_EQ(items.size(), 2u);
    EXPECT_NE(std::find(items.begin(), items.end(), sub), items.end());
    EXPECT_NE(std::find(items.begin(), items.end(), root), items.end());
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
    auto* selected = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(selected, nullptr);
    const std::string selectedId = selected->layer()->GetIdentifier();

    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->muteLayer();
    QApplication::processEvents();

    const CommandCall* call = _sessionState._commandHookImpl.lastCallOf("muteSubLayer");
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->args.empty());
    EXPECT_EQ(call->args[0], selectedId)
        << "muteSubLayer must act on the selected layer";
}

TEST_F(LayerTreeViewTest, LockAction_CallsLockLayerOnSelectedItem)
{
    auto* selected = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(selected, nullptr);
    const std::string selectedId = selected->layer()->GetIdentifier();

    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->lockLayer();
    QApplication::processEvents();

    const CommandCall* call = _sessionState._commandHookImpl.lastCallOf("lockLayer");
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->args.empty());
    EXPECT_EQ(call->args[0], selectedId)
        << "lockLayer must act on the selected layer";
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

TEST_F(LayerTreeViewTest, DoubleClick_SkipsWhenLayerDoesNotNeedSaving)
{
    // Default stub stage is not shared, so needsSaving() == false and the handler
    // must return before attempting any save.
    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
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
    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
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

// ── layerItemFromIndex / layerTreeModel ───────────────────────────────────────

TEST_F(LayerTreeViewTest, LayerItemFromIndex_ValidIndex_ReturnsNonNull)
{
    EXPECT_NE(layerTree()->layerItemFromIndex(rootLayerIndex()), nullptr);
}

TEST_F(LayerTreeViewTest, LayerItemFromIndex_InvalidIndex_ReturnsNull)
{
    EXPECT_EQ(layerTree()->layerItemFromIndex(QModelIndex()), nullptr);
}

TEST_F(LayerTreeViewTest, LayerTreeModel_MatchesTreeModel)
{
    EXPECT_EQ(layerTree()->layerTreeModel(), treeModel());
}

// ── getSelectedLayerItems empty case ─────────────────────────────────────────

TEST_F(LayerTreeViewTest, GetSelectedLayerItems_EmptyByDefault)
{
    layerTree()->clearSelection();
    layerTree()->setCurrentIndex(QModelIndex());
    auto items = layerTree()->getSelectedLayerItems();
    EXPECT_TRUE(items.empty());
}

// ── expand / collapse children ────────────────────────────────────────────────
// expandChildren/collapseChildren/shouldExpandOrCollapseAll are protected, so
// we subclass LayerTreeView to expose them for tests.

class TestableLayerTreeView : public LayerTreeView
{
public:
    explicit TestableLayerTreeView(SessionState* s, QWidget* parent = nullptr)
        : LayerTreeView(s, parent) {}

    using LayerTreeView::expandChildren;
    using LayerTreeView::collapseChildren;
    using LayerTreeView::shouldExpandOrCollapseAll;
    using LayerTreeView::onMuteLayerButtonPushed;
    using LayerTreeView::onLockLayerButtonPushed;
};

TEST_F(LayerTreeViewTest, CollapseChildren_AlsoCollapsesRoot)
{
    TestableLayerTreeView tree(&_sessionState, _mainWindow);
    tree.show();
    QApplication::processEvents();

    QModelIndex root = tree.layerTreeModel()->rootLayerIndex();
    tree.expand(root);
    QApplication::processEvents();
    ASSERT_TRUE(tree.isExpanded(root));

    // collapseChildren collapses the index itself as well as all its descendants.
    tree.collapseChildren(root);
    QApplication::processEvents();
    EXPECT_FALSE(tree.isExpanded(root));
}

TEST_F(LayerTreeViewTest, ExpandChildren_ExpandsCollapsedIndex)
{
    TestableLayerTreeView tree(&_sessionState, _mainWindow);
    tree.show();
    QApplication::processEvents();

    QModelIndex root = tree.layerTreeModel()->rootLayerIndex();
    tree.collapse(root);
    QApplication::processEvents();
    ASSERT_FALSE(tree.isExpanded(root));

    tree.expandChildren(root);
    QApplication::processEvents();
    EXPECT_TRUE(tree.isExpanded(root));
}

// ── mute / lock button-push slots ──────────────────────────────────────────────
// onMuteLayerButtonPushed/onLockLayerButtonPushed act on the current item (the
// action-button path), distinct from the selection-based onMuteLayer/onLockLayer.

TEST_F(LayerTreeViewTest, MuteLayerButtonPushed_CallsMuteSubLayerOnCurrentItem)
{
    TestableLayerTreeView tree(&_sessionState, _mainWindow);
    tree.show();
    QApplication::processEvents();

    QModelIndex root = tree.layerTreeModel()->rootLayerIndex();
    tree.setCurrentIndex(tree.layerTreeModel()->index(0, 0, root));
    ASSERT_NE(tree.currentLayerItem(), nullptr);
    const std::string currentId = tree.currentLayerItem()->layer()->GetIdentifier();

    _sessionState._commandHookImpl.clearCalls();
    tree.onMuteLayerButtonPushed();
    QApplication::processEvents();

    const CommandCall* call = _sessionState._commandHookImpl.lastCallOf("muteSubLayer");
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->args.empty());
    EXPECT_EQ(call->args[0], currentId)
        << "muteSubLayer must act on the current item";
}

TEST_F(LayerTreeViewTest, LockLayerButtonPushed_CallsLockLayerOnCurrentItem)
{
    TestableLayerTreeView tree(&_sessionState, _mainWindow);
    tree.show();
    QApplication::processEvents();

    QModelIndex root = tree.layerTreeModel()->rootLayerIndex();
    tree.setCurrentIndex(tree.layerTreeModel()->index(0, 0, root));
    ASSERT_NE(tree.currentLayerItem(), nullptr);
    const std::string currentId = tree.currentLayerItem()->layer()->GetIdentifier();

    _sessionState._commandHookImpl.clearCalls();
    tree.onLockLayerButtonPushed();
    QApplication::processEvents();

    const CommandCall* call = _sessionState._commandHookImpl.lastCallOf("lockLayer");
    ASSERT_NE(call, nullptr);
    ASSERT_FALSE(call->args.empty());
    EXPECT_EQ(call->args[0], currentId)
        << "lockLayer must act on the current item";
}

// ── keyboard handling ──────────────────────────────────────────────────────────

TEST_F(LayerTreeViewTest, KeyPress_Delete_RemovesSelectedSublayer)
{
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();

    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier);
    layerTree()->keyPressEvent(&keyEvent);
    QApplication::processEvents();

    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("removeSubLayerPath"));
}

TEST_F(LayerTreeViewTest, KeyPress_R_RefreshesModel)
{
    // Insert a sublayer directly into the stack (bypassing the model) so a refresh
    // is observable as an additional row under the root.
    const int before = treeModel()->rowCount(rootLayerIndex());
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(
        SdfLayer::CreateAnonymous("refresh_extra")->GetIdentifier(), 0);

    // Without event processing the model has not yet rebuilt, so the new row is absent.
    EXPECT_EQ(treeModel()->rowCount(rootLayerIndex()), before);

    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier);
    layerTree()->keyPressEvent(&keyEvent);
    QApplication::processEvents();

    EXPECT_EQ(treeModel()->rowCount(rootLayerIndex()), before + 1);
}

// ── add parent layer ───────────────────────────────────────────────────────────

TEST_F(LayerTreeViewTest, AddParentLayer_ReplacesSelectedWithAnonymousParent)
{
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();

    layerTree()->onAddParentLayer("Add Parent Layer");
    QApplication::processEvents();

    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("replaceSubLayerPath"));
}

} // namespace UsdLayerEditor
