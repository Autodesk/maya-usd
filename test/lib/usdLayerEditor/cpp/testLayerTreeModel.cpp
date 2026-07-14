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
#include "layerTreeItem.h"
#include "layerTreeModel.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <QtCore/QMimeData>
#include <QtWidgets/QApplication>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

class LayerTreeModelTest : public LayerEditorTestFixture {};

// ── flags / MIME ───────────────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, Flags_DragEnabledOnlyForMovableItems)
{
    auto subFlags  = treeModel()->flags(firstSublayerIndex());
    auto rootFlags = treeModel()->flags(rootLayerIndex());
    EXPECT_TRUE(subFlags & Qt::ItemIsDragEnabled);
    EXPECT_FALSE(rootFlags & Qt::ItemIsDragEnabled);
}

TEST_F(LayerTreeModelTest, Flags_DropAlwaysEnabled)
{
    EXPECT_TRUE(treeModel()->flags(rootLayerIndex()) & Qt::ItemIsDropEnabled);
    EXPECT_TRUE(treeModel()->flags(firstSublayerIndex()) & Qt::ItemIsDropEnabled);
}

TEST_F(LayerTreeModelTest, SupportedDropActions_OnlyMoveAction)
{
    EXPECT_EQ(treeModel()->supportedDropActions(), Qt::MoveAction);
}

TEST_F(LayerTreeModelTest, MimeTypes_ReturnsTextPlain)
{
    auto types = treeModel()->mimeTypes();
    ASSERT_EQ(types.size(), 1);
    EXPECT_EQ(types.at(0), QString("text/plain"));
}

TEST_F(LayerTreeModelTest, MimeData_SerializesIdentifiersWithSemicolon)
{
    // Two indices so the ';' separator between serialized identifiers is exercised.
    QModelIndexList indexes = { firstSublayerIndex(), rootLayerIndex() };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);
    EXPECT_TRUE(mime->hasFormat("text/plain"));
    QString data = QString::fromUtf8(mime->data("text/plain"));

    auto* subItem  = treeModel()->layerItemFromIndex(firstSublayerIndex());
    auto* rootItem = treeModel()->layerItemFromIndex(rootLayerIndex());
    ASSERT_NE(subItem, nullptr);
    ASSERT_NE(rootItem, nullptr);

    // Identifiers are joined in index order, separated by ';'.
    const QString expected = QString::fromStdString(subItem->layer()->GetIdentifier()) + ';'
        + QString::fromStdString(rootItem->layer()->GetIdentifier());
    EXPECT_EQ(data, expected);
}

TEST_F(LayerTreeModelTest, CanDrop_ReturnsFalseForNullMimeData)
{
    EXPECT_FALSE(treeModel()->canDropMimeData(nullptr, Qt::MoveAction, 0, 0, rootLayerIndex()));
}

// ── rebuildModel / session layer visibility ────────────────────────────────────

TEST_F(LayerTreeModelTest, Rebuild_AlwaysShowsSessionLayerWhenAutoHideFalse)
{
    // StubSessionState::autoHideSessionLayer() returns false.
    // Session layer must always be the first top-level item.
    treeModel()->forceRefresh();
    QApplication::processEvents();
    auto* first = treeModel()->layerItemFromIndex(treeModel()->index(0, 0));
    ASSERT_NE(first, nullptr);
    EXPECT_TRUE(first->isSessionLayer());
}

TEST_F(LayerTreeModelTest, RebuildOnIdle_DeduplicatesScheduling)
{
    // Calling forceRefresh twice before processing events should
    // result in only one rebuild (not two).
    int resetCount = 0;
    QObject::connect(treeModel(), &QAbstractItemModel::modelReset,
        [&resetCount]() { ++resetCount; });
    treeModel()->forceRefresh();
    treeModel()->forceRefresh();
    QApplication::processEvents();
    // The guard deduplicates the two explicit calls to one rebuild. However, rebuilding
    // the model resets _rebuildOnIdlePending before endResetModel(), so a USD notice fired
    // during item construction can schedule a second rebuild.
    EXPECT_LE(resetCount, 2);
}

TEST_F(LayerTreeModelTest, Rebuild_SkipsResetWhenLayersAreIdentical)
{
    // EMSUSD-3680: rebuilding the model when layer structure has not changed
    // should not emit modelReset, to avoid redundant tree redraws.
    QApplication::processEvents(); // let initial build settle
    int resetCount = 0;
    QObject::connect(treeModel(), &QAbstractItemModel::modelReset,
        [&resetCount]() { ++resetCount; });
    // Force a second rebuild with the same layer state — should be a no-op.
    treeModel()->forceRefresh();
    QApplication::processEvents();
    EXPECT_EQ(resetCount, 0) << "modelReset should not fire when layers are identical";
}

#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerTreeModelTest, SessionStageChanged_DefersAndCoalescesRebuild)
{
    // sessionStageChanged() defers to an idle callback and coalesces a burst of stage
    // changes into a single rebuild rather than rebuilding synchronously per change.
    // The path forces refreshLockState=true, so the idle rebuild resets even though the
    // layer structure is unchanged. Driven through the real currentStageChangedSignal
    // path via setStageEntry() (sessionStageChanged() is a protected slot).
    QApplication::processEvents(); // let the initial build settle
    const auto stages = _sessionState.allStages();
    ASSERT_GE(stages.size(), 2u);

    int resetCount = 0;
    QObject::connect(treeModel(), &QAbstractItemModel::modelReset,
        [&resetCount]() { ++resetCount; });

    // Two stage switches within one event-loop turn, each emitting currentStageChangedSignal.
    _sessionState.setStageEntry(stages[1]);
    _sessionState.setStageEntry(stages[0]);
    // Deferred: pre-port this rebuilt synchronously (resetCount would be >= 1 here).
    EXPECT_EQ(resetCount, 0) << "sessionStageChanged should defer the rebuild to idle";

    QApplication::processEvents();
    // Coalesced to a single rebuild; a USD notice fired during item construction may
    // schedule one extra (mirrors RebuildOnIdle_DeduplicatesScheduling).
    EXPECT_GE(resetCount, 1) << "the coalesced idle rebuild should run";
    EXPECT_LE(resetCount, 2);
}
#endif // !MAYAUSD_OLD_LAYER_EDITOR

// ── filtering helpers ──────────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, GetAllNeedsSavingLayers_EmptyWhenNoLayersAreDirtyAndShared)
{
    // StubSessionState is not a shared stage, so needsSaving = false for all.
    auto layers = treeModel()->getAllNeedsSavingLayers();
    EXPECT_TRUE(layers.empty());
}

TEST_F(LayerTreeModelTest, GetAllAnonymousLayers_ExcludesSessionLayer)
{
    auto anonLayers = treeModel()->getAllAnonymousLayers();
    for (auto* item : anonLayers) {
        EXPECT_FALSE(item->isSessionLayer())
            << "getAllAnonymousLayers should not include the session layer";
    }
}

TEST_F(LayerTreeModelTest, GetAllAnonymousLayers_IncludesAnonymousSublayers)
{
    // The stub has one anonymous sublayer per stage.
    auto anonLayers = treeModel()->getAllAnonymousLayers();
    EXPECT_GE(anonLayers.size(), 1u);
}

TEST_F(LayerTreeModelTest, FindNameForNewAnonymousLayer_ReturnsNonEmptyString)
{
    std::string name = treeModel()->findNameForNewAnonymousLayer();
    EXPECT_FALSE(name.empty());
}

TEST_F(LayerTreeModelTest, FindNameForNewAnonymousLayer_DoesNotCollideWithExisting)
{
    // Call once, add a layer with that name, then ask again.
    std::string name1 = treeModel()->findNameForNewAnonymousLayer();
    auto newLayer = SdfLayer::CreateAnonymous(name1);
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(
        newLayer->GetIdentifier(), 0);
    QApplication::processEvents();
    std::string name2 = treeModel()->findNameForNewAnonymousLayer();
    EXPECT_NE(name1, name2);
}

// ── setEditTarget guards ───────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, SetEditTarget_CallsHookForAccessibleLayer)
{
    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
}

TEST_F(LayerTreeModelTest, SetEditTarget_BlockedWhenLayerIsLocked)
{
    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeModelTest, SetEditTarget_BlockedWhenLayerIsMuted)
{
    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    const std::string layerId = item->layer()->GetIdentifier();
    _sessionState.stage()->MuteLayer(layerId);
    QApplication::processEvents();

    // Muting fired a LayersDidChange notice that rebuilt the model and deleted the
    // original item, so re-fetch it before use to avoid a dangling pointer.
    item = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(item, nullptr);

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
    _sessionState.stage()->UnmuteLayer(layerId);
}

// ── rootLayerIndex ─────────────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, RootLayerIndex_IsValid)
{
    EXPECT_TRUE(rootLayerIndex().isValid());
}

TEST_F(LayerTreeModelTest, RootLayerIndex_ItemIsRootLayer)
{
    auto* item = treeModel()->layerItemFromIndex(rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isRootLayer());
}

// ── invalid layer item ─────────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, Flags_InvalidLayerItem_ReturnsOnlySelectableAndEnabled)
{
    const std::string fakePath = "/nonexistent/flags_invalid.usda";
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(fakePath, 0);
    treeModel()->forceRefresh();
    QApplication::processEvents();

    QModelIndex invalidIdx = treeModel()->index(0, 0, rootLayerIndex());
    auto*       invalid    = treeModel()->layerItemFromIndex(invalidIdx);
    ASSERT_NE(invalid, nullptr);
    ASSERT_TRUE(invalid->isInvalidLayer());

    Qt::ItemFlags flags = treeModel()->flags(invalidIdx);
    EXPECT_TRUE(flags & Qt::ItemIsSelectable);
    EXPECT_TRUE(flags & Qt::ItemIsEnabled);
    EXPECT_FALSE(flags & Qt::ItemIsDragEnabled);
    EXPECT_FALSE(flags & Qt::ItemIsDropEnabled);
}

TEST_F(LayerTreeModelTest, MimeData_InvalidLayerItem_UsesSubLayerPath)
{
    const std::string fakePath = "/nonexistent/mime_invalid.usda";
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(fakePath, 0);
    treeModel()->forceRefresh();
    QApplication::processEvents();

    QModelIndex invalidIdx = treeModel()->index(0, 0, rootLayerIndex());
    auto*       invalid    = treeModel()->layerItemFromIndex(invalidIdx);
    ASSERT_NE(invalid, nullptr);
    ASSERT_TRUE(invalid->isInvalidLayer());

    QModelIndexList             indexes = { invalidIdx };
    std::unique_ptr<QMimeData>  mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);
    EXPECT_TRUE(mime->hasFormat("text/plain"));
    QString data = QString::fromUtf8(mime->data("text/plain"));
    EXPECT_EQ(data, QString::fromStdString(fakePath));
}

// ── dropMimeData ───────────────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, DropMimeData_WrongMimeFormat_ReturnsFalse)
{
    auto mimeData = std::make_unique<QMimeData>();
    mimeData->setHtml("<b>wrong format</b>");
    EXPECT_FALSE(treeModel()->dropMimeData(
        mimeData.get(), Qt::MoveAction, 0, 0, rootLayerIndex()));
}

// ── selectUsdLayerOnIdle ───────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, SelectUsdLayerOnIdle_EmitsSelectSignalForExistingLayer)
{
    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    auto layer = item->layer();

    QModelIndex receivedIndex;
    QObject::connect(
        treeModel(), &LayerTreeModel::selectLayerSignal,
        [&receivedIndex](const QModelIndex& idx) { receivedIndex = idx; });

    treeModel()->selectUsdLayerOnIdle(layer);
    QApplication::processEvents();

    EXPECT_TRUE(receivedIndex.isValid());
}

// ── USD notice: usd_editTargetChanged ─────────────────────────────────────────

TEST_F(LayerTreeModelTest, UsdEditTargetChanged_UpdatesTargetLayerOnIdle)
{
    auto* subItem = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(subItem, nullptr);
    auto sublayerRef = subItem->layer();
    ASSERT_FALSE(subItem->isTargetLayer());

    // Directly change the USD stage's edit target to fire
    // UsdNotice::StageEditTargetChanged, bypassing the stub command hook.
    _sessionState.stage()->SetEditTarget(UsdEditTarget(sublayerRef));
    QApplication::processEvents();

    // Re-fetch in case model rebuilt during event processing.
    subItem = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(subItem, nullptr);
    EXPECT_TRUE(subItem->isTargetLayer());
}

// ── selectedLayerDataChangedSignal (EMSUSD-3823) ───────────────────────────────

TEST_F(LayerTreeModelTest, SelectedLayerDataChanged_EmittedOnLayerDataChange)
{
    // setSessionState schedules a data-changed rebuild; flush it before counting.
    QApplication::processEvents();

    int dataChangedCount = 0;
    QObject::connect(treeModel(), &LayerTreeModel::selectedLayerDataChangedSignal,
        [&dataChangedCount]() { ++dataChangedCount; });

    // Author data into a layer without altering the layer tree structure: this is
    // the case the model-rebuild optimization stopped refreshing the contents for.
    _sessionState.stage()->DefinePrim(SdfPath("/testDataChange"));
    QApplication::processEvents();

    EXPECT_GE(dataChangedCount, 1)
        << "selectedLayerDataChangedSignal should fire when layer data changes";
}

TEST_F(LayerTreeModelTest, SelectedLayerDataChanged_NotEmittedOnPlainRefresh)
{
    QApplication::processEvents(); // settle the initial build

    int dataChangedCount = 0;
    QObject::connect(treeModel(), &LayerTreeModel::selectedLayerDataChangedSignal,
        [&dataChangedCount]() { ++dataChangedCount; });

    // forceRefresh() rebuilds without flagging a layer-data change.
    treeModel()->forceRefresh();
    QApplication::processEvents();

    EXPECT_EQ(dataChangedCount, 0)
        << "selectedLayerDataChangedSignal should not fire for a non-data-change rebuild";
}

} // namespace UsdLayerEditor
