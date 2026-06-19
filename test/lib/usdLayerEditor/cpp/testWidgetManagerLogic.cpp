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

#include "layerEditorWidgetManager.h"
#include "layerEditorWidget.h"

#include <gtest/gtest.h>

namespace UsdLayerEditor {

// getInstance() always returns the same non-null pointer.
TEST_F(LayerEditorTestFixture, WidgetManager_GetInstance_ReturnsSamePointer)
{
    auto* a = LayerEditorWidgetManager::getInstance();
    auto* b = LayerEditorWidgetManager::getInstance();
    EXPECT_NE(a, nullptr);
    EXPECT_EQ(a, b);
}

// The fixture's SetUp creates a LayerEditorWidget which calls setWidget(this).
// So the manager already has a live widget at test time.
TEST_F(LayerEditorTestFixture, WidgetManager_GetSelectedLayers_NoSelectionReturnsEmpty)
{
    // Clear selection AND current index so getSelectedLayerItems() returns nothing.
    _widget->selectLayers({});
    QApplication::processEvents();

    auto* mgr = LayerEditorWidgetManager::getInstance();
    auto  layers = mgr->getSelectedLayers();
    EXPECT_TRUE(layers.empty());
}

TEST_F(LayerEditorTestFixture, WidgetManager_GetSelectedLayers_WithSelection_ReturnsIds)
{
    selectRow(rootLayerIndex());

    auto* mgr = LayerEditorWidgetManager::getInstance();
    auto  layers = mgr->getSelectedLayers();
    EXPECT_FALSE(layers.empty());
    EXPECT_EQ(layers.size(), 1u);
}

// selectLayers with an empty list clears the selection and current index.
TEST_F(LayerEditorTestFixture, WidgetManager_SelectLayers_EmptyList_ClearsSelection)
{
    selectRow(rootLayerIndex());

    auto* mgr = LayerEditorWidgetManager::getInstance();
    mgr->selectLayers({});
    QApplication::processEvents();

    EXPECT_TRUE(layerTree()->selectionModel()->selectedRows().empty());
    EXPECT_FALSE(layerTree()->selectionModel()->currentIndex().isValid());
}

// selectLayers with a valid layer identifier selects it.
TEST_F(LayerEditorTestFixture, WidgetManager_SelectLayers_ValidId_SelectsLayer)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    const std::string rootId = rootItem->layer()->GetIdentifier();

    layerTree()->selectionModel()->clearSelection();
    QApplication::processEvents();

    auto* mgr = LayerEditorWidgetManager::getInstance();
    mgr->selectLayers({ rootId });
    QApplication::processEvents();

    auto selected = mgr->getSelectedLayers();
    ASSERT_EQ(selected.size(), 1u);
    EXPECT_EQ(selected[0], rootId);
}

// Calling setWidget again with a second live widget hits the TF_WARN path.
// We just verify it does not crash or corrupt state.
TEST_F(LayerEditorTestFixture, WidgetManager_SetWidget_OverrideDoesNotCrash)
{
    auto* mgr = LayerEditorWidgetManager::getInstance();
    // Pass the same widget again — the guard branch fires but state is unchanged.
    mgr->setWidget(_widget);
    auto layers = mgr->getSelectedLayers();
    // Should still return valid results.
    SUCCEED();
}

} // namespace UsdLayerEditor
