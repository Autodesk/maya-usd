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

#include "generatedIconButton.h"
#include "layerContentsWidget.h"
#include "layerEditorWidget.h"
#include "layerTreeItem.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QIcon>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSplitter>

#include <gtest/gtest.h>

namespace UsdLayerEditor {

// Helper: find LayerContentsWidget inside the LayerEditorWidget.
static LayerContentsWidget* contentsWidget(QWidget* root)
{
    return root->findChild<LayerContentsWidget*>(QString(), Qt::FindChildrenRecursively);
}

// ── getSelectedLayers / selectLayers ─────────────────────────────────────────

// With nothing selected, getSelectedLayers returns empty.
TEST_F(LayerEditorTestFixture, Widget_GetSelectedLayers_NothingSelected_ReturnsEmpty)
{
#ifdef MAYAUSD_OLD_LAYER_EDITOR
    // Old editor's selectLayers({}) does not clear currentIndex, so getSelectedLayers
    // falls back to the current item and is non-empty. Known parity gap, no production
    // impact. See test/lib/oldUsdLayerEditor/cpp/PARITY_FAILURES_DETAIL.md (Failure 4).
    GTEST_SKIP() << "Expected old-editor parity gap: selectLayers({}) does not clear currentIndex.";
#endif
    // Use selectLayers({}) which also clears currentIndex, so the fallback in
    // getSelectedLayerItems() does not return a stale current-index item.
    _widget->selectLayers({});
    QApplication::processEvents();

    auto layers = _widget->getSelectedLayers();
    EXPECT_TRUE(layers.empty());
}

// After selecting the root layer, getSelectedLayers returns its identifier.
TEST_F(LayerEditorTestFixture, Widget_GetSelectedLayers_RootSelected_ReturnsId)
{
    selectRow(rootLayerIndex());

    auto layers = _widget->getSelectedLayers();
    ASSERT_EQ(layers.size(), 1u);

    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    EXPECT_EQ(layers[0], rootItem->layer()->GetIdentifier());
}

// selectLayers with an empty vector clears the selection and current index.
TEST_F(LayerEditorTestFixture, Widget_SelectLayers_Empty_ClearsSelection)
{
#ifdef MAYAUSD_OLD_LAYER_EDITOR
    // Old editor's selectLayers({}) clears selected rows but leaves currentIndex valid.
    // Known parity gap, no production impact. See
    // test/lib/oldUsdLayerEditor/cpp/PARITY_FAILURES_DETAIL.md (Failure 5).
    GTEST_SKIP() << "Expected old-editor parity gap: selectLayers({}) does not clear currentIndex.";
#endif
    selectRow(rootLayerIndex());
    _widget->selectLayers({});
    QApplication::processEvents();

    // Both selectedRows() and currentIndex() should be cleared.
    EXPECT_TRUE(layerTree()->selectionModel()->selectedRows().empty());
    EXPECT_FALSE(layerTree()->selectionModel()->currentIndex().isValid());
}

// selectLayers with a valid identifier selects the correct layer.
TEST_F(LayerEditorTestFixture, Widget_SelectLayers_ValidId_SelectsLayer)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    const std::string rootId = rootItem->layer()->GetIdentifier();

    layerTree()->selectionModel()->clearSelection();
    QApplication::processEvents();

    _widget->selectLayers({ rootId });
    QApplication::processEvents();

    auto selected = _widget->getSelectedLayers();
    ASSERT_EQ(selected.size(), 1u);
    EXPECT_EQ(selected[0], rootId);
}

// selectLayers with an unknown identifier is a no-op (no crash, no selection).
TEST_F(LayerEditorTestFixture, Widget_SelectLayers_UnknownId_NoOp)
{
    layerTree()->selectionModel()->clearSelection();
    QApplication::processEvents();

    _widget->selectLayers({ "anon:nonexistent_layer_identifier_xyz" });
    QApplication::processEvents();

    EXPECT_TRUE(layerTree()->selectionModel()->selectedRows().empty());
}

// ── showDisplayLayerContents ──────────────────────────────────────────────────

// showDisplayLayerContents(true) makes the LayerContentsWidget visible.
TEST_F(LayerEditorTestFixture, Widget_ShowDisplayLayerContents_True_MakesWidgetVisible)
{
    _widget->showDisplayLayerContents(true);
    QApplication::processEvents();

    auto* cw = contentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    EXPECT_TRUE(cw->isVisible());
}

// showDisplayLayerContents(false) hides the LayerContentsWidget.
TEST_F(LayerEditorTestFixture, Widget_ShowDisplayLayerContents_False_HidesWidget)
{
    _widget->showDisplayLayerContents(false);
    QApplication::processEvents();

    auto* cw = contentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    EXPECT_FALSE(cw->isVisible());
}

// Showing then hiding leaves the contents widget hidden.
TEST_F(LayerEditorTestFixture, Widget_ShowDisplayLayerContents_HideAfterShow_HidesWidget)
{
    _widget->showDisplayLayerContents(true);
    QApplication::processEvents();
    _widget->showDisplayLayerContents(false);
    QApplication::processEvents();

    auto* cw = contentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    EXPECT_FALSE(cw->isVisible());
}

// ── onSplitterMoved ───────────────────────────────────────────────────────────

// index==1 with the pane open drives the layer-contents update branch.
TEST_F(LayerEditorTestFixture, Widget_OnSplitterMoved_ContentsIndex_DoesNotCrash)
{
    _widget->showDisplayLayerContents(true);
    QApplication::processEvents();

    // index 1 is the layer-contents pane; pos=200 means the pane is open.
    _widget->onSplitterMoved(200, 1);
    QApplication::processEvents();
    SUCCEED();
}

// index != 1 is a no-op early-return branch.
TEST_F(LayerEditorTestFixture, Widget_OnSplitterMoved_OtherIndex_DoesNotCrash)
{
    _widget->onSplitterMoved(100, 0);
    QApplication::processEvents();
    SUCCEED();
}

// ── onLazyUpdateLayerContents / timerEvent ───────────────────────────────────

// Scheduling the lazy update and processing events exercises the timer path.
TEST_F(LayerEditorTestFixture, Widget_OnLazyUpdateLayerContents_DoesNotCrash)
{
    _widget->showDisplayLayerContents(true);
    selectRow(rootLayerIndex());
    QApplication::processEvents();

    _widget->onLazyUpdateLayerContents();
    QApplication::processEvents();
    SUCCEED();
}

// ── updateButtonsOnIdle ───────────────────────────────────────────────────────

TEST_F(LayerEditorTestFixture, Widget_UpdateButtonsOnIdle_DoesNotCrash)
{
    _widget->updateButtonsOnIdle();
    QApplication::processEvents();
    SUCCEED();
}

// ── onSaveStageButtonClicked ──────────────────────────────────────────────────
// Drives saveStage(); the bulk save dialog is suppressed by the fixture's exec
// test handler, so this exercises the save path without showing UI.
TEST_F(LayerEditorTestFixture, Widget_OnSaveStageButtonClicked_DoesNotCrash)
{
    EXPECT_NO_THROW(_widget->onSaveStageButtonClicked());
    QApplication::processEvents();
}

// ── GeneratedIconButton paint states ──────────────────────────────────────────
// paint() picks base / hover / disabled pixmaps; rendering in each state must
// produce visibly different output.
TEST_F(LayerEditorTestFixture, GeneratedIconButton_PaintReflectsEnabledAndHoverState)
{
    QPixmap iconPixmap(16, 16);
    iconPixmap.fill(QColor(100, 100, 100));
    GeneratedIconButton button(_mainWindow, QIcon(iconPixmap), 16);
    button.resize(16, 16);

    auto renderToImage = [&button]() {
        QPixmap target(button.size());
        target.fill(Qt::transparent);
        button.render(&target);
        return target.toImage();
    };

    QImage base = renderToImage();

    // Enter event sets the hover state; paint must use the (brighter) hover pixmap.
    QEvent enterEvent(QEvent::Enter);
    QCoreApplication::sendEvent(&button, &enterEvent);
    QImage hover = renderToImage();

    // Disabled: paint must use the (more transparent) disabled pixmap.
    button.setEnabled(false);
    QImage disabled = renderToImage();

    EXPECT_NE(base, hover);
    EXPECT_NE(base, disabled);
}

} // namespace UsdLayerEditor
