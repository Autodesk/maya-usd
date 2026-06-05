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
#include "layerTreeItem.h"

#include <pxr/usd/sdf/layer.h>

#include <QtCore/QMimeData>
#include <QtWidgets/QApplication>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

static void addSecondSublayer(PXR_NS::UsdStageRefPtr stage)
{
    auto rootLayer = stage->GetRootLayer();
    auto extra     = SdfLayer::CreateAnonymous("extra_sublayer");
    rootLayer->InsertSubLayerPath(extra->GetIdentifier(), 1);
}

TEST_F(LayerEditorTestFixture, DragDrop_MoveRowDown_CallsMoveSubLayerPath)
{
    addSecondSublayer(_sessionState.stage());
    QApplication::processEvents();

    auto rootLayer = _sessionState.stage()->GetRootLayer();
    auto paths     = rootLayer->GetSubLayerPaths();
    ASSERT_GE(paths.size(), 2u) << "Need at least 2 sublayers";

    const std::string draggedPath = paths[0];

    QModelIndex parentIndex = rootLayerIndex();
    QMimeData*  mimeData    = treeModel()->mimeData({ treeModel()->index(0, 0, parentIndex) });
    ASSERT_NE(mimeData, nullptr) << "Model must supply MIME data for drag";

    bool accepted = treeModel()->dropMimeData(mimeData, Qt::MoveAction, 2, 0, parentIndex);
    delete mimeData;

    ASSERT_TRUE(accepted) << "dropMimeData should accept a valid downward move";
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"))
        << "moveSubLayerPath should be called on reorder";

    // Moving the first sublayer down to the end must leave it last in the order.
    auto newPaths = rootLayer->GetSubLayerPaths();
    ASSERT_FALSE(newPaths.empty());
    EXPECT_EQ(newPaths[newPaths.size() - 1], draggedPath)
        << "Dragged layer should now be last";
}

TEST_F(LayerEditorTestFixture, DragDrop_MoveRowUp_CallsMoveSubLayerPath)
{
    addSecondSublayer(_sessionState.stage());
    QApplication::processEvents();

    auto rootLayer = _sessionState.stage()->GetRootLayer();
    auto paths     = rootLayer->GetSubLayerPaths();
    ASSERT_GE(paths.size(), 2u);

    const std::string draggedPath = paths[1];

    QModelIndex parentIndex = rootLayerIndex();
    QMimeData*  mimeData    = treeModel()->mimeData({ treeModel()->index(1, 0, parentIndex) });
    ASSERT_NE(mimeData, nullptr);

    bool accepted = treeModel()->dropMimeData(mimeData, Qt::MoveAction, 0, 0, parentIndex);
    delete mimeData;

    ASSERT_TRUE(accepted) << "dropMimeData should accept a valid upward move";
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"))
        << "moveSubLayerPath should be called on reorder";

    // Moving the second sublayer up to the top must leave it first in the order.
    auto newPaths = rootLayer->GetSubLayerPaths();
    ASSERT_FALSE(newPaths.empty());
    EXPECT_EQ(newPaths[0], draggedPath) << "Dragged layer should now be first";
}

// ── canDropMimeData validation ─────────────────────────────────────────────────

TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForNonMoveAction)
{
    QModelIndexList indexes = { firstSublayerIndex() };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);
    EXPECT_FALSE(treeModel()->canDropMimeData(
        mime.get(), Qt::CopyAction, 0, 0, rootLayerIndex()));
}

TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForWrongMimeType)
{
    auto mime = std::make_unique<QMimeData>();
    mime->setData("application/x-wrong", QByteArray("data"));
    EXPECT_FALSE(treeModel()->canDropMimeData(
        mime.get(), Qt::MoveAction, 0, 0, rootLayerIndex()));
}

TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForLockedParent)
{
    QModelIndexList indexes = { firstSublayerIndex() };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);

    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    TestUtils::lockLayerDirect(rootItem->layer());

    EXPECT_FALSE(treeModel()->canDropMimeData(
        mime.get(), Qt::MoveAction, 0, 0, rootLayerIndex()));

    TestUtils::unlockLayerDirect(rootItem->layer());
}

TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForReadOnlyParent)
{
    QModelIndexList indexes = { firstSublayerIndex() };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);

    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    rootItem->layer()->SetPermissionToEdit(false);
    rootItem->layer()->SetPermissionToSave(false);

    EXPECT_FALSE(treeModel()->canDropMimeData(
        mime.get(), Qt::MoveAction, 0, 0, rootLayerIndex()));

    rootItem->layer()->SetPermissionToEdit(true);
    rootItem->layer()->SetPermissionToSave(true);
}

TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsTrueForValidMove)
{
    QModelIndexList indexes = { firstSublayerIndex() };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);
    EXPECT_TRUE(treeModel()->canDropMimeData(
        mime.get(), Qt::MoveAction, 0, 0, rootLayerIndex()));
}

// ── dropMimeData ordering ─────────────────────────────────────────────────────

static void addTwoSublayers(PXR_NS::UsdStageRefPtr stage)
{
    auto root = stage->GetRootLayer();
    if (root->GetNumSubLayerPaths() < 2) {
        auto extra = SdfLayer::CreateAnonymous("extra_drop_test");
        root->InsertSubLayerPath(extra->GetIdentifier(), 1);
    }
}

TEST_F(LayerEditorTestFixture, DragDrop_Drop_AdjustsRowIndexWhenMovingUp)
{
    addTwoSublayers(_sessionState.stage());
    QApplication::processEvents();

    auto rootLayer = _sessionState.stage()->GetRootLayer();
    const std::string draggedPath = rootLayer->GetSubLayerPaths()[1];

    QModelIndex parent = rootLayerIndex();
    ASSERT_GE(treeModel()->rowCount(parent), 2);

    QModelIndexList indexes = { treeModel()->index(1, 0, parent) };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);

    _sessionState._commandHookImpl.clearCalls();
    bool accepted = treeModel()->dropMimeData(mime.get(), Qt::MoveAction, 0, 0, parent);
    QApplication::processEvents();

    ASSERT_TRUE(accepted) << "dropMimeData should accept the upward move";
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"));

    // Dropping row 1 at row 0 must land the dragged layer first (row-index adjustment).
    auto newPaths = rootLayer->GetSubLayerPaths();
    ASSERT_FALSE(newPaths.empty());
    EXPECT_EQ(newPaths[0], draggedPath) << "Dragged layer should now be first";
}

TEST_F(LayerEditorTestFixture, DragDrop_Drop_CallsMoveSubLayerPathOnSuccess)
{
    addTwoSublayers(_sessionState.stage());
    QApplication::processEvents();

    auto rootLayer = _sessionState.stage()->GetRootLayer();
    const std::string draggedPath = rootLayer->GetSubLayerPaths()[0];

    QModelIndex parent = rootLayerIndex();
    ASSERT_GE(treeModel()->rowCount(parent), 2);

    QModelIndexList indexes = { treeModel()->index(0, 0, parent) };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);

    _sessionState._commandHookImpl.clearCalls();
    bool accepted = treeModel()->dropMimeData(
        mime.get(), Qt::MoveAction, 2, 0, parent);
    QApplication::processEvents();

    ASSERT_TRUE(accepted) << "dropMimeData should accept the downward move";
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"));

    // Dragged layer (row 0) dropped at the end must be last in the order.
    auto newPaths = rootLayer->GetSubLayerPaths();
    ASSERT_FALSE(newPaths.empty());
    EXPECT_EQ(newPaths[newPaths.size() - 1], draggedPath)
        << "Dragged layer should now be last";
}

} // namespace UsdLayerEditor
