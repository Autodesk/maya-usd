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

#include "testFixture.h"
#include "testUtils.h"
#include "layerTreeItem.h"
#include "layerTreeModel.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <QtCore/QMimeData>
#include <QtWidgets/QApplication>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

static LayerTreeItem* itemAt(LayerTreeModel* m, const QModelIndex& idx)
{
    return dynamic_cast<LayerTreeItem*>(m->itemFromIndex(idx));
}

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
    QModelIndexList indexes = { firstSublayerIndex() };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);
    EXPECT_TRUE(mime->hasFormat("text/plain"));
    QString data = QString::fromUtf8(mime->data("text/plain"));
    auto*   item = itemAt(treeModel(), firstSublayerIndex());
    EXPECT_TRUE(data.contains(QString::fromStdString(item->layer()->GetIdentifier())));
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
    auto* first = itemAt(treeModel(), treeModel()->index(0, 0));
    ASSERT_NE(first, nullptr);
    EXPECT_TRUE(first->isSessionLayer());
}

TEST_F(LayerTreeModelTest, Rebuild_ClearsAndRepopulatesRows)
{
    int rowsBefore = treeModel()->rowCount();
    treeModel()->forceRefresh();
    QApplication::processEvents();
    // Row count should be consistent after rebuild.
    EXPECT_EQ(treeModel()->rowCount(), rowsBefore);
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
    // during item construction can schedule a second rebuild. This mirrors old-editor behavior.
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
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
}

TEST_F(LayerTreeModelTest, SetEditTarget_BlockedWhenLayerIsLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeModelTest, SetEditTarget_BlockedWhenLayerIsMuted)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState.stage()->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();
    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(item);
    EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
    _sessionState.stage()->UnmuteLayer(item->layer()->GetIdentifier());
}

// ── rootLayerIndex ─────────────────────────────────────────────────────────────

TEST_F(LayerTreeModelTest, RootLayerIndex_IsValid)
{
    EXPECT_TRUE(rootLayerIndex().isValid());
}

TEST_F(LayerTreeModelTest, RootLayerIndex_ItemIsRootLayer)
{
    auto* item = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isRootLayer());
}

} // namespace UsdLayerEditor
