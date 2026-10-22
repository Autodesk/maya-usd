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

#include "layerContextMenu.h"
#include "layerLocking.h"
#include "layerTreeItem.h"
#include "stringResources.h"
#include "testUtils.h"

#include <pxr/usd/usd/stage.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>

#include <functional>

#include <testFixture.h>

namespace UsdLayerEditor {

// ------------------------------------------------------------------
// Core operations — called directly on the window after selecting
// the appropriate tree row, so they don't depend on QMenu::exec().
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, ContextMenu_AddAnonymousSublayer_CallsHook)
{
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    // Capture the identifier before the action: addAnonymousSublayer triggers a
    // model rebuild on processEvents() that destroys this item, so it must not be
    // dereferenced afterward.
    const std::string layerId = item->layer()->GetIdentifier();
    selectRow(firstSublayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->addAnonymousSublayer();
    QApplication::processEvents();
    const auto* call = _sessionState._commandHookImpl.lastCallOf("addAnonymousSubLayer");
    ASSERT_NE(call, nullptr) << "addAnonymousSubLayer should have been called";
    EXPECT_EQ(call->args[0], layerId) << "addAnonymousSubLayer should target the selected layer";
}

TEST_F(LayerEditorTestFixture, ContextMenu_MuteLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState._commandHookImpl.clearCalls();
    _window->muteLayer();
    QApplication::processEvents();
    const auto* call = _sessionState._commandHookImpl.lastCallOf("muteSubLayer");
    ASSERT_NE(call, nullptr) << "muteSubLayer should have been called";
    EXPECT_EQ(call->args[0], layerId) << "muteSubLayer should target the selected layer";
}

TEST_F(LayerEditorTestFixture, ContextMenu_LockLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState._commandHookImpl.clearCalls();
    _window->lockLayer();
    QApplication::processEvents();
    const auto* call = _sessionState._commandHookImpl.lastCallOf("lockLayer");
    ASSERT_NE(call, nullptr) << "lockLayer should have been called";
    EXPECT_EQ(call->args[0], layerId) << "lockLayer should target the selected layer";
}

TEST_F(LayerEditorTestFixture, ContextMenu_RemoveLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState._commandHookImpl.clearCalls();
    _window->removeSubLayer();
    QApplication::processEvents();
    const auto* call = _sessionState._commandHookImpl.lastCallOf("removeSubLayerPath");
    ASSERT_NE(call, nullptr) << "removeSubLayerPath should have been called";
    // args = { parentIdentifier, removedSubLayerPath }; the removed path is the sublayer.
    EXPECT_EQ(call->args[1], layerId) << "removeSubLayerPath should target the selected sublayer";
}

TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_CallsHook)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState._commandHookImpl.clearCalls();
    _window->discardEdits();
    QApplication::processEvents();
    const auto* call = _sessionState._commandHookImpl.lastCallOf("discardEdits");
    ASSERT_NE(call, nullptr) << "discardEdits should have been called";
    EXPECT_EQ(call->args[0], layerId) << "discardEdits should target the selected layer";
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
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState._commandHookImpl.clearCalls();
    _window->selectPrimsWithSpec();
    QApplication::processEvents();
    const auto* call = _sessionState._commandHookImpl.lastCallOf("selectPrimsWithSpec");
    ASSERT_NE(call, nullptr) << "selectPrimsWithSpec should have been called";
    EXPECT_EQ(call->args[0], layerId) << "selectPrimsWithSpec should target the selected layer";
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

    EXPECT_TRUE(_window->layerIsLocked()) << "Layer should report locked after lockLayer()";
}

TEST_F(LayerEditorTestFixture, ContextMenu_UnlockedLayer_IsNotLocked)
{
    selectRow(firstSublayerIndex());
    EXPECT_FALSE(_window->layerIsLocked()) << "Fresh sublayer should not be locked";
}

// ── additional window actions ──────────────────────────────────────────────────

TEST_F(LayerEditorTestFixture, ContextMenu_ClearLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState._commandHookImpl.clearCalls();
    _window->clearLayer();
    QApplication::processEvents();
    const auto* call = _sessionState._commandHookImpl.lastCallOf("clearLayer");
    ASSERT_NE(call, nullptr) << "clearLayer should have been called";
    EXPECT_EQ(call->args[0], layerId) << "clearLayer should target the selected layer";
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
    auto* rootItem = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(rootLayerIndex()));
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
    auto* rootItem = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    ASSERT_TRUE(rootItem->hasSubLayers()) << "Root must have a sublayer for this test";

    selectRow(rootLayerIndex());
    _sessionState._commandHookImpl.clearCalls();
    _window->mergeWithSublayers();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("flattenLayer"));
}

// Discarding edits on an anonymous layer intentionally skips the confirm dialog
// (MAYA-104336): there is no on-disk content to lose.
TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_SkipsConfirmForAnonymousLayer)
{
    selectRow(firstSublayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isAnonymous());

    _modalDialogCount = 0;
    _sessionState._commandHookImpl.clearCalls();
    _window->discardEdits();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("discardEdits"));
    EXPECT_EQ(_modalDialogCount, 0) << "anonymous layer should discard without a confirm dialog";
}

// ── setEditTarget guards (via model) ──────────────────────────────────────────

TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsMuted)
{
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
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
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));

    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsSystemLocked)
{
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
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
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
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
    QString                _filePath;
    PXR_NS::SdfLayerRefPtr _fileLayer;

    void SetUp() override
    {
        LayerEditorTestFixture::SetUp();

        _filePath = QDir::tempPath() + "/le_discard_confirm_test.usda";
        QFile::remove(_filePath);
        _fileLayer = PXR_NS::SdfLayer::CreateNew(_filePath.toStdString());
        _fileLayer->SetComment("dirty content"); // non-anonymous + dirty → triggers confirm
        _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(_fileLayer->GetIdentifier(), 0);
        QApplication::processEvents();
    }

    void TearDown() override
    {
        LayerEditorTestFixture::TearDown();
        QFile::remove(_filePath);
    }

    // The injected file layer becomes the new index-0 child of root.
    QModelIndex fileLayerIndex() { return treeModel()->index(0, 0, rootLayerIndex()); }
};

// When the user confirms (answer = true), discardEdits runs and the hook is called.
TEST_F(DiscardConfirmFixture, ContextMenu_DiscardEdits_ConfirmAccepted_CallsHook)
{
    selectRow(fileLayerIndex());
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(fileLayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_FALSE(item->isAnonymous()) << "file layer must be non-anonymous to trigger confirm";

    _modalDialogAnswer = true;
    _modalDialogCount = 0;
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
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(fileLayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_FALSE(item->isAnonymous()) << "file layer must be non-anonymous to trigger confirm";

    _modalDialogAnswer = false;
    _modalDialogCount = 0;
    _sessionState._commandHookImpl.clearCalls();
    _window->discardEdits();
    QApplication::processEvents();

    EXPECT_GT(_modalDialogCount, 0) << "confirm dialog should have been shown";
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("discardEdits"))
        << "discardEdits should NOT be called when the user cancels";
}

// ------------------------------------------------------------------
// Menu contents — built with populateLayerContextMenu().
// ------------------------------------------------------------------

namespace {

QAction* findAction(const QMenu& menu, const QString& label)
{
    for (QAction* action : menu.actions()) {
        if (action->text() == label)
            return action;
    }
    return nullptr;
}

// Item labels in order, with "-" standing for a separator.
QStringList menuLabels(const QMenu& menu)
{
    QStringList labels;
    for (const QAction* action : menu.actions())
        labels << (action->isSeparator() ? QString("-") : action->text());
    return labels;
}

} // namespace

class ContextMenuContentFixture : public LayerEditorTestFixture
{
protected:
    void build(const std::function<void(QMenu&)>& addDCCItems = {})
    {
        populateLayerContextMenu(_menu, *_window, *layerTree(), addDCCItems);
    }

    void addToSelection(const QModelIndex& index)
    {
        layerTree()->selectionModel()->select(
            index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        QApplication::processEvents();
    }

    QMenu _menu;
};

TEST_F(ContextMenuContentFixture, SublayerMenu_HasExpectedItemsInOrder)
{
    selectRow(firstSublayerIndex());
    build();
    EXPECT_EQ(
        menuLabels(_menu),
        (QStringList { GET_LE_QSTR(kMenuSaveAs),
                       "-",
                       GET_LE_QSTR(kMenuAddSublayer),
                       GET_LE_QSTR(kMenuAddParentLayer),
                       GET_LE_QSTR(kMenuLoadSublayers),
                       "-",
                       GET_LE_QSTR(kMenuMute),
                       GET_LE_QSTR(kMenuLock),
                       GET_LE_QSTR(kMenuPrintToScriptEditor),
                       "-",
                       GET_LE_QSTR(kMenuSelectPrimsWithSpec),
                       "-",
                       GET_LE_QSTR(kMenuRemove),
                       GET_LE_QSTR(kMenuClear) }));
}

TEST_F(ContextMenuContentFixture, SessionLayerMenu_OmitsSaveAndLock)
{
    selectRow(sessionLayerIndex());
    build();
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuSaveAs)), nullptr);
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuSaveEdits)), nullptr);
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuLock)), nullptr);
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuRemove)), nullptr);
    EXPECT_NE(findAction(_menu, GET_LE_QSTR(kMenuClear)), nullptr);
}

TEST_F(ContextMenuContentFixture, MultiSelectNonSession_OffersStitch)
{
    selectRow(rootLayerIndex());
    addToSelection(firstSublayerIndex());
    build();
    EXPECT_NE(findAction(_menu, GET_LE_QSTR(kMenuStitchLayers)), nullptr);
    // Single-selection-only items are disabled.
    EXPECT_FALSE(findAction(_menu, GET_LE_QSTR(kMenuLoadSublayers))->isEnabled());
}

TEST_F(ContextMenuContentFixture, MultiSelectSessionAndNonSession_OmitsStitch)
{
    selectRow(sessionLayerIndex());
    addToSelection(rootLayerIndex());
    build();
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuStitchLayers)), nullptr);
}

TEST_F(ContextMenuContentFixture, NoCurrentLayer_MenuIsEmpty)
{
    layerTree()->selectionModel()->clearSelection();
    layerTree()->setCurrentIndex(QModelIndex());
    QApplication::processEvents();
    ASSERT_EQ(layerTree()->currentLayerItem(), nullptr);

    build([](QMenu& menu) { menu.addAction("DCC item"); });
    EXPECT_TRUE(_menu.actions().isEmpty());
}

TEST_F(ContextMenuContentFixture, SingleSelect_OmitsStitch)
{
    selectRow(firstSublayerIndex());
    build();
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuStitchLayers)), nullptr);
}

TEST_F(ContextMenuContentFixture, InvalidLayerMenu_OnlyHasEnabledRemove)
{
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath("does_not_exist.usda", 0);
    QApplication::processEvents();
    selectRow(firstSublayerIndex());
    ASSERT_TRUE(_window->isInvalidLayer());

    build();
    EXPECT_EQ(menuLabels(_menu), QStringList { GET_LE_QSTR(kMenuRemove) });
    EXPECT_TRUE(findAction(_menu, GET_LE_QSTR(kMenuRemove))->isEnabled());
}

TEST_F(ContextMenuContentFixture, SessionSublayers_CountAsSessionForStitch)
{
    auto sessionLayer = _sessionState.stage()->GetSessionLayer();
    for (int i = 0; i < 2; ++i) {
        auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sessionSublayer");
        sessionLayer->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    }
    QApplication::processEvents();
    const QModelIndex firstSessionSublayer = treeModel()->index(0, 0, sessionLayerIndex());
    const QModelIndex secondSessionSublayer = treeModel()->index(1, 0, sessionLayerIndex());
    ASSERT_TRUE(firstSessionSublayer.isValid());
    ASSERT_TRUE(secondSessionSublayer.isValid());

    selectRow(firstSessionSublayer);
    addToSelection(secondSessionSublayer);
    build();
    EXPECT_NE(findAction(_menu, GET_LE_QSTR(kMenuStitchLayers)), nullptr)
        << "Only session layers are selected";

    _menu.clear();
    addToSelection(rootLayerIndex());
    build();
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuStitchLayers)), nullptr)
        << "A session sublayer mixed with a non-session layer must not offer Stitch";
}

TEST_F(ContextMenuContentFixture, LayerAlsoInSessionStack_CountsAsSessionForStitch)
{
    // The same layer is a sublayer of both the session layer and the root layer.
    auto sharedLayer = PXR_NS::SdfLayer::CreateAnonymous("sharedSublayer");
    _sessionState.stage()->GetSessionLayer()->InsertSubLayerPath(sharedLayer->GetIdentifier(), 0);
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(sharedLayer->GetIdentifier(), 0);
    QApplication::processEvents();
    const QModelIndex sharedUnderRoot = treeModel()->index(0, 0, rootLayerIndex());
    const QModelIndex plainUnderRoot = treeModel()->index(1, 0, rootLayerIndex());
    ASSERT_TRUE(plainUnderRoot.isValid());

    // Its row is under the root, but it is a session layer, so mixing it with a plain
    // root sublayer must not offer Stitch.
    selectRow(sharedUnderRoot);
    addToSelection(plainUnderRoot);
    build();
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuStitchLayers)), nullptr);
}

TEST_F(ContextMenuContentFixture, UnlockedSublayer_ActionsAreEnabled)
{
    selectRow(firstSublayerIndex());
    build();
    for (const QString& label : { GET_LE_QSTR(kMenuAddSublayer),
                                  GET_LE_QSTR(kMenuAddParentLayer),
                                  GET_LE_QSTR(kMenuLoadSublayers),
                                  GET_LE_QSTR(kMenuMute),
                                  GET_LE_QSTR(kMenuLock),
                                  GET_LE_QSTR(kMenuPrintToScriptEditor),
                                  GET_LE_QSTR(kMenuSelectPrimsWithSpec),
                                  GET_LE_QSTR(kMenuRemove),
                                  GET_LE_QSTR(kMenuClear) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_TRUE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }
    EXPECT_EQ(
        findAction(_menu, GET_LE_QSTR(kMenuSaveAs))->isEnabled(), _window->layerNeedsSaving());
}

TEST_F(ContextMenuContentFixture, LockedSublayer_DisablesEditsAndOffersUnlock)
{
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const auto layer = item->layer();
    TestUtils::lockLayerDirect(layer);
    QApplication::processEvents();
    selectRow(firstSublayerIndex());
    build();

    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuLock)), nullptr);
    ASSERT_NE(findAction(_menu, GET_LE_QSTR(kMenuUnlock)), nullptr);
    EXPECT_TRUE(findAction(_menu, GET_LE_QSTR(kMenuUnlock))->isEnabled());
    for (const QString& label : { GET_LE_QSTR(kMenuAddSublayer),
                                  GET_LE_QSTR(kMenuLoadSublayers),
                                  GET_LE_QSTR(kMenuClear) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_FALSE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }
    // These edit the parent, so only a locked parent disables them.
    for (const QString& label : { GET_LE_QSTR(kMenuAddParentLayer), GET_LE_QSTR(kMenuRemove) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_TRUE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }

    TestUtils::unlockLayerDirect(layer);
}

TEST_F(ContextMenuContentFixture, SublayerOfLockedParent_DisablesAddParentAndRemove)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    const auto rootLayer = rootItem->layer();
    TestUtils::lockLayerDirect(rootLayer);
    QApplication::processEvents();
    selectRow(firstSublayerIndex());
    build();

    for (const QString& label : { GET_LE_QSTR(kMenuAddParentLayer), GET_LE_QSTR(kMenuRemove) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_FALSE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }
    // The sublayer itself is not locked.
    for (const QString& label : { GET_LE_QSTR(kMenuAddSublayer), GET_LE_QSTR(kMenuClear) }) {
        EXPECT_TRUE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }

    TestUtils::unlockLayerDirect(rootLayer);
}

TEST_F(ContextMenuContentFixture, SublayerOfSystemLockedParent_DisablesAddParentAndRemove)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    const auto rootLayer = rootItem->layer();
    addSystemLockedLayer(rootLayer);
    QApplication::processEvents();
    selectRow(firstSublayerIndex());
    build();

    for (const QString& label : { GET_LE_QSTR(kMenuAddParentLayer), GET_LE_QSTR(kMenuRemove) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_FALSE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }

    removeSystemLockedLayer(rootLayer);
}

TEST_F(ContextMenuContentFixture, SystemLockedSublayer_DisablesLockAndEdits)
{
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const auto layer = item->layer();
    addSystemLockedLayer(layer);
    QApplication::processEvents();
    selectRow(firstSublayerIndex());
    build();

    for (const QString& label : { GET_LE_QSTR(kMenuAddSublayer),
                                  GET_LE_QSTR(kMenuLoadSublayers),
                                  GET_LE_QSTR(kMenuLock),
                                  GET_LE_QSTR(kMenuClear) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_FALSE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }

    removeSystemLockedLayer(layer);
}

TEST_F(ContextMenuContentFixture, MutedSublayer_DisablesAddsAndOffersUnmute)
{
    auto* item = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState.stage()->MuteLayer(layerId);
    QApplication::processEvents();
    selectRow(firstSublayerIndex());
    build();

    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuMute)), nullptr);
    ASSERT_NE(findAction(_menu, GET_LE_QSTR(kMenuUnmute)), nullptr);
    EXPECT_TRUE(findAction(_menu, GET_LE_QSTR(kMenuUnmute))->isEnabled());
    for (const QString& label : { GET_LE_QSTR(kMenuAddSublayer),
                                  GET_LE_QSTR(kMenuAddParentLayer),
                                  GET_LE_QSTR(kMenuLoadSublayers) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_FALSE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }
    // Clear does not depend on the muted state.
    EXPECT_TRUE(findAction(_menu, GET_LE_QSTR(kMenuClear))->isEnabled());

    _sessionState.stage()->UnmuteLayer(layerId);
}

TEST_F(ContextMenuContentFixture, LayerWithSublayers_OffersMergeAndLockWithSublayers)
{
    selectRow(rootLayerIndex());
    build();
    for (const QString& label :
         { GET_LE_QSTR(kMenuMergeWithSublayers), GET_LE_QSTR(kMenuLockLayerAndSublayers) }) {
        ASSERT_NE(findAction(_menu, label), nullptr) << qPrintable(label);
        EXPECT_TRUE(findAction(_menu, label)->isEnabled()) << qPrintable(label);
    }
    // The root layer is not a sublayer.
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuMute)), nullptr);
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuRemove)), nullptr);
    EXPECT_FALSE(findAction(_menu, GET_LE_QSTR(kMenuAddParentLayer))->isEnabled());
}

TEST_F(ContextMenuContentFixture, LockedLayerWithSublayers_DisablesMerge)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    const auto rootLayer = rootItem->layer();
    TestUtils::lockLayerDirect(rootLayer);
    QApplication::processEvents();
    selectRow(rootLayerIndex());
    build();

    EXPECT_FALSE(findAction(_menu, GET_LE_QSTR(kMenuMergeWithSublayers))->isEnabled());
    EXPECT_EQ(findAction(_menu, GET_LE_QSTR(kMenuLockLayerAndSublayers)), nullptr);
    EXPECT_TRUE(findAction(_menu, GET_LE_QSTR(kMenuUnlockLayerAndSublayers))->isEnabled());

    TestUtils::unlockLayerDirect(rootLayer);
}

TEST_F(ContextMenuContentFixture, SessionLayerWithSublayers_MergeDisabledWhileRenderLayerActive)
{
    auto sessionLayer = _sessionState.stage()->GetSessionLayer();
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sessionSublayer");
    sessionLayer->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    QApplication::processEvents();

    selectRow(sessionLayerIndex());
    build();
    ASSERT_NE(findAction(_menu, GET_LE_QSTR(kMenuMergeWithSublayers)), nullptr);
    EXPECT_TRUE(findAction(_menu, GET_LE_QSTR(kMenuMergeWithSublayers))->isEnabled());

    _activeRenderLayer = sublayer;
    treeModel()->forceRefresh();
    QApplication::processEvents();
    selectRow(sessionLayerIndex());
    _menu.clear();
    build();
    ASSERT_NE(findAction(_menu, GET_LE_QSTR(kMenuMergeWithSublayers)), nullptr);
    EXPECT_FALSE(findAction(_menu, GET_LE_QSTR(kMenuMergeWithSublayers))->isEnabled());
}

TEST_F(ContextMenuContentFixture, DCCItems_AppearAfterSelectPrimsWithSpec)
{
    selectRow(firstSublayerIndex());
    build([](QMenu& menu) { menu.addAction("DCC item"); });
    const QStringList labels = menuLabels(_menu);
    const int         dccIndex = labels.indexOf("DCC item");
    ASSERT_GE(dccIndex, 1);
    EXPECT_EQ(labels[dccIndex - 1], GET_LE_QSTR(kMenuSelectPrimsWithSpec));
}

TEST_F(ContextMenuContentFixture, TriggeringAction_CallsWindow)
{
    selectRow(firstSublayerIndex());
    build();
    _sessionState._commandHookImpl.clearCalls();
    findAction(_menu, GET_LE_QSTR(kMenuSelectPrimsWithSpec))->trigger();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("selectPrimsWithSpec"));
}

} // namespace UsdLayerEditor
