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

#include <testFixture.h>
#include "testUtils.h"
#include "layerLocking.h"
#include "layerTreeItem.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtWidgets/QApplication>

#include <pxr/usd/usd/stage.h>

namespace UsdLayerEditor {

// ------------------------------------------------------------------
// Core operations — called directly on the window after selecting
// the appropriate tree row, so they don't depend on QMenu::exec().
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, ContextMenu_AddAnonymousSublayer_CallsHook)
{
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->addAnonymousSublayer();
    QApplication::processEvents();
    ASSERT_TRUE(_sessionState._commandHookImpl.hasCall("addAnonymousSubLayer"));
    EXPECT_EQ(_sessionState._commandHookImpl.lastCall().args[0], item->layer()->GetIdentifier())
        << "addAnonymousSubLayer should target the selected layer";
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
    // saveEdits on an anonymous layer routes through the save-layer UI hook
    // (SessionState::saveLayerUI), not the command hook. The stub cancels the
    // dialog, so the only observable effect is that the hook was invoked.
    EXPECT_NO_THROW({
        _window->saveEdits();
        QApplication::processEvents();
    });
    EXPECT_GT(_sessionState._saveLayerCallCount, 0);
}

TEST_F(LayerEditorTestFixture, ContextMenu_MergeWithSublayers_BlockedWhenNoSublayers)
{
    // A leaf sublayer has no children — mergeWithSublayers should be a no-op.
    // mergeWithSublayers dispatches flattenLayer (not stitchLayers), so its
    // absence is what proves the merge was blocked.
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->mergeWithSublayers();
    QApplication::processEvents();
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("flattenLayer"));
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
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("flattenLayer"));

    TestUtils::unlockLayerDirect(rootItem->layer());
}

// Positive control: an unlocked layer that has sublayers must dispatch flattenLayer.
// Without this, the two "blocked" tests above would pass even if merge never worked.
TEST_F(LayerEditorTestFixture, ContextMenu_MergeWithSublayers_CallsFlattenWhenLayerHasSublayers)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    ASSERT_TRUE(rootItem->hasSubLayers()) << "Root must have a sublayer for this test";

    selectRow(rootLayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->mergeWithSublayers();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("flattenLayer"));
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
    // System-lock only — SetPermissionToEdit(false) would also block setEditTarget via
    // isLocked(), but this test is specifically verifying the isSystemLocked() predicate.
    addSystemLockedLayer(item->layer());

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));

    removeSystemLockedLayer(item->layer());
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

// ── discardEdits confirm path ─────────────────────────────────────────────────
// The confirm dialog fires only for layers that are both non-anonymous AND dirty.
// This fixture injects such a layer into the stage after widget construction.

class DiscardConfirmFixture : public LayerEditorTestFixture
{
protected:
    QString                  _filePath;
    PXR_NS::SdfLayerRefPtr   _fileLayer;

    void SetUp() override
    {
        LayerEditorTestFixture::SetUp();

        _filePath = QDir::tempPath() + "/le_discard_confirm_test.usda";
        QFile::remove(_filePath);
        _fileLayer = PXR_NS::SdfLayer::CreateNew(_filePath.toStdString());
        _fileLayer->SetComment("dirty content"); // non-anonymous + dirty → triggers confirm
        _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(
            _fileLayer->GetIdentifier(), 0);
        QApplication::processEvents();
    }

    void TearDown() override
    {
        LayerEditorTestFixture::TearDown();
        QFile::remove(_filePath);
    }

    // The injected file layer becomes the new index-0 child of root.
    QModelIndex fileLayerIndex()
    {
        return treeModel()->index(0, 0, rootLayerIndex());
    }
};

// When the user confirms (answer = true), discardEdits runs and the hook is called.
TEST_F(DiscardConfirmFixture, ContextMenu_DiscardEdits_ConfirmAccepted_CallsHook)
{
    selectRow(fileLayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(fileLayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_FALSE(item->isAnonymous()) << "file layer must be non-anonymous to trigger confirm";

    _modalDialogAnswer = true;
    _modalDialogCount  = 0;
    _sessionState._commandHookImpl.clearCalls();
    _window->discardEdits();
    QApplication::processEvents();

    EXPECT_GT(_modalDialogCount, 0) << "confirm dialog should have been shown";
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("discardEdits"))
        << "discardEdits should be called when the user confirms";
}

// When the user cancels (answer = false), discardEdits is NOT called.
TEST_F(DiscardConfirmFixture, ContextMenu_DiscardEdits_ConfirmRejected_DoesNotCallHook)
{
    selectRow(fileLayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(fileLayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_FALSE(item->isAnonymous()) << "file layer must be non-anonymous to trigger confirm";

    _modalDialogAnswer = false;
    _modalDialogCount  = 0;
    _sessionState._commandHookImpl.clearCalls();
    _window->discardEdits();
    QApplication::processEvents();

    EXPECT_GT(_modalDialogCount, 0) << "confirm dialog should have been shown";
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("discardEdits"))
        << "discardEdits should NOT be called when the user cancels";
}

} // namespace UsdLayerEditor
