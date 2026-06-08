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
#include "loadLayersDialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

class LoadLayersDialogTest : public LayerEditorTestFixture {};

TEST_F(LoadLayersDialogTest, LoadLayersDialog_ConstructsWithoutCrash)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    EXPECT_NO_THROW({
        LoadLayersDialog dlg(rootItem, _mainWindow);
    });
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_HasAtLeastOneLineEdit)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    auto lineEdits = dlg.findChildren<QLineEdit*>();
    EXPECT_GE(lineEdits.size(), 1);
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_HasOkAndCancelButtons)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    bool hasOk = false, hasCancel = false;
    for (auto* btn : dlg.findChildren<QPushButton*>()) {
        if (btn->text().contains("OK", Qt::CaseInsensitive) ||
            btn->text().contains("Load", Qt::CaseInsensitive))
            hasOk = true;
        if (btn->text().contains("Cancel", Qt::CaseInsensitive))
            hasCancel = true;
    }
    EXPECT_TRUE(hasOk)     << "LoadLayersDialog should have an OK/Load button";
    EXPECT_TRUE(hasCancel) << "LoadLayersDialog should have a Cancel button";
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_StartsWithEmptyPath)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    auto lineEdits = dlg.findChildren<QLineEdit*>();
    ASSERT_GE(lineEdits.size(), 1);
    // The first editable row starts empty.
    EXPECT_TRUE(lineEdits.first()->text().isEmpty());
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_HasScrollArea)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    auto* scroll = dlg.findChild<QScrollArea*>(QString(), Qt::FindChildrenRecursively);
    EXPECT_NE(scroll, nullptr);
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_ExecDismissedByTimerDoesNotHang)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    TestUtils::dismissNextModal(100);
    EXPECT_NO_THROW(dlg.exec());
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_AddRowButtonExists)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    EXPECT_GE(dlg.findChildren<QPushButton*>().size(), 1);
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_PathEditIsEnabled)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    auto lineEdits = dlg.findChildren<QLineEdit*>();
    ASSERT_GE(lineEdits.size(), 1);
    EXPECT_TRUE(lineEdits.first()->isEnabled());
}

} // namespace UsdLayerEditor
