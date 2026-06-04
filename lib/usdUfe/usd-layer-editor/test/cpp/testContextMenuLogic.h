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
#pragma once

#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
#include "testFixture.h"
#endif
#include "testUtils.h"
#include "layerLocking.h"
#include "layerTreeItem.h"

#include <QtWidgets/QApplication>

namespace UsdLayerEditor {

// ------------------------------------------------------------------
// Core operations — called directly on the window after selecting
// the appropriate tree row, so they don't depend on QMenu::exec().
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, ContextMenu_AddAnonymousSublayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->addAnonymousSublayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("addAnonymousSubLayer"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_MuteLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->muteLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("muteSubLayer"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_LockLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->lockLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("lockLayer"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_RemoveLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->removeSubLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("removeSubLayerPath"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->discardEdits();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("discardEdits"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_PrintLayer_CallsSessionState)
{
    selectRow(firstSublayerIndex());
    _window->printLayer();
    QApplication::processEvents();
    EXPECT_GT(_sessionState._printLayerCallCount, 0);
}

TEST_F(LayerEditorTestFixture, ContextMenu_SelectPrimsWithSpec_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->selectPrimsWithSpec();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("selectPrimsWithSpec"));
}

// ------------------------------------------------------------------
// Layer-type queries via the window's state methods
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, LayerQuery_SessionLayer_IsSessionLayer)
{
    selectRow(sessionLayerIndex());
    EXPECT_TRUE(_window->isSessionLayer());
}

TEST_F(LayerEditorTestFixture, LayerQuery_Sublayer_IsNotSessionLayer)
{
    selectRow(firstSublayerIndex());
    EXPECT_FALSE(_window->isSessionLayer());
}

TEST_F(LayerEditorTestFixture, LayerQuery_Sublayer_IsSubLayer)
{
    selectRow(firstSublayerIndex());
    EXPECT_TRUE(_window->isSubLayer());
}

TEST_F(LayerEditorTestFixture, LayerQuery_SessionLayer_IsNotSubLayer)
{
    selectRow(sessionLayerIndex());
    EXPECT_FALSE(_window->isSubLayer());
}

// ------------------------------------------------------------------
// Lock state affects isLocked() query
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, ContextMenu_LockedLayer_IsLocked)
{
    selectRow(firstSublayerIndex());
    // Lock the sublayer directly via the command hook.
    _window->lockLayer();
    QApplication::processEvents();

    // Re-select the row so the window refreshes its current item.
    selectRow(firstSublayerIndex());
    EXPECT_TRUE(_window->layerIsLocked())
        << "Layer should report locked after lockLayer()";
}

TEST_F(LayerEditorTestFixture, ContextMenu_UnlockedLayer_IsNotLocked)
{
    selectRow(firstSublayerIndex());
    EXPECT_FALSE(_window->layerIsLocked())
        << "Fresh sublayer should not be locked";
}

// ── additional window actions ──────────────────────────────────────────────────

TEST_F(LayerEditorTestFixture, ContextMenu_ClearLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->clearLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("clearLayer"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_SaveEdits_DoesNotCrash)
{
    selectRow(firstSublayerIndex());
    _sessionState._saveLayerCallCount = 0;
    // saveEdits on an anonymous layer — must not crash regardless of path taken.
    EXPECT_NO_THROW({
        _window->saveEdits();
        QApplication::processEvents();
    });
}

TEST_F(LayerEditorTestFixture, ContextMenu_MergeWithSublayers_BlockedWhenNoSublayers)
{
    // A leaf sublayer has no children — mergeWithSublayers should be a no-op.
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->mergeWithSublayers();
    QApplication::processEvents();
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("stitchLayers"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_MergeWithSublayers_BlockedWhenLayerIsLocked)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    TestUtils::lockLayerDirect(rootItem->layer());

    selectRow(rootLayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->mergeWithSublayers();
    QApplication::processEvents();
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("stitchLayers"));

    TestUtils::unlockLayerDirect(rootItem->layer());
}

TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_SkipsConfirmForAnonymousLayer)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isAnonymous());

    _sessionState._commandHookImpl.clearCalls();
    _window->discardEdits();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("discardEdits"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_SkipsConfirmForCleanLayer)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_FALSE(item->isDirty());

    _sessionState._commandHookImpl.clearCalls();
    _window->discardEdits();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("discardEdits"));
}

// ── setEditTarget guards (via model) ──────────────────────────────────────────

TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsMuted)
{
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    _sessionState.stage()->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));

    _sessionState.stage()->UnmuteLayer(item->layer()->GetIdentifier());
}

TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsLocked)
{
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));

    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsSystemLocked)
{
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    addSystemLockedLayer(item->layer());
    item->layer()->SetPermissionToEdit(false);

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));

    removeSystemLockedLayer(item->layer());
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerEditorTestFixture, SetEditTarget_AllowedForNormalSublayer)
{
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
}

} // namespace UsdLayerEditor
