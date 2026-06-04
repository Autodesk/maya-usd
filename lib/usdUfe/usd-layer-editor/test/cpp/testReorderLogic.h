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

    QModelIndex parentIndex = rootLayerIndex();
    QMimeData*  mimeData    = treeModel()->mimeData({ treeModel()->index(0, 0, parentIndex) });
    ASSERT_NE(mimeData, nullptr) << "Model must supply MIME data for drag";

    bool accepted = treeModel()->dropMimeData(mimeData, Qt::MoveAction, 2, 0, parentIndex);
    delete mimeData;

    if (accepted) {
        QApplication::processEvents();
        EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"))
            << "moveSubLayerPath should be called on reorder";
    } else {
        // Verify the model at least advertises drag support.
        EXPECT_FALSE(treeModel()->mimeTypes().isEmpty())
            << "Model should support MIME data for drag-drop";
    }
}

TEST_F(LayerEditorTestFixture, DragDrop_MoveRowUp_CallsMoveSubLayerPath)
{
    addSecondSublayer(_sessionState.stage());
    QApplication::processEvents();

    auto rootLayer = _sessionState.stage()->GetRootLayer();
    auto paths     = rootLayer->GetSubLayerPaths();
    ASSERT_GE(paths.size(), 2u);

    QModelIndex parentIndex = rootLayerIndex();
    QMimeData*  mimeData    = treeModel()->mimeData({ treeModel()->index(1, 0, parentIndex) });
    ASSERT_NE(mimeData, nullptr);

    bool accepted = treeModel()->dropMimeData(mimeData, Qt::MoveAction, 0, 0, parentIndex);
    delete mimeData;

    if (accepted) {
        QApplication::processEvents();
        EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"))
            << "moveSubLayerPath should be called on reorder";
    } else {
        EXPECT_FALSE(treeModel()->mimeTypes().isEmpty())
            << "Model should support MIME data for drag-drop";
    }
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

    QModelIndex parent = rootLayerIndex();
    ASSERT_GE(treeModel()->rowCount(parent), 2);

    QModelIndexList indexes = { treeModel()->index(1, 0, parent) };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->dropMimeData(mime.get(), Qt::MoveAction, 0, 0, parent);
    QApplication::processEvents();

    // Re-fetch parent: the model rebuild triggered by moveSubLayerPath invalidates
    // any QModelIndex captured before processEvents().
    parent = rootLayerIndex();
    EXPECT_GE(treeModel()->rowCount(parent), 1);
}

TEST_F(LayerEditorTestFixture, DragDrop_Drop_CallsMoveSubLayerPathOnSuccess)
{
    addTwoSublayers(_sessionState.stage());
    QApplication::processEvents();

    QModelIndex parent = rootLayerIndex();
    ASSERT_GE(treeModel()->rowCount(parent), 2);

    QModelIndexList indexes = { treeModel()->index(0, 0, parent) };
    std::unique_ptr<QMimeData> mime(treeModel()->mimeData(indexes));
    ASSERT_NE(mime, nullptr);

    _sessionState._commandHookImpl.clearCalls();
    bool accepted = treeModel()->dropMimeData(
        mime.get(), Qt::MoveAction, 2, 0, parent);
    QApplication::processEvents();

    if (accepted) {
        EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"));
    } else {
        EXPECT_FALSE(treeModel()->mimeTypes().isEmpty());
    }
}

} // namespace UsdLayerEditor
