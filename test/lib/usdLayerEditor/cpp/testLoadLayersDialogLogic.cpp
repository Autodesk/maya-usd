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
#include "loadLayersDialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

class LoadLayersDialogTest : public LayerEditorTestFixture {};

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

TEST_F(LoadLayersDialogTest, LoadLayersDialog_FindDirectoryToUse_WithNonEmptyPath)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    // Passing a file path: should strip the filename and return the directory.
    std::string result = dlg.findDirectoryToUse("/tmp/some/file.usd");
    EXPECT_EQ(result, "/tmp/some");
}

TEST_F(LoadLayersDialogTest, LoadLayersDialog_OnAddRow_IncreasesRowCount)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    int beforeCount = dlg.findChildren<QLineEdit*>().size();
    // onAddRow() is public (connected by LayerPathRow). Call it directly.
    dlg.onAddRow();
    QApplication::processEvents();
    int afterCount = dlg.findChildren<QLineEdit*>().size();
    EXPECT_GT(afterCount, beforeCount);
}

// Trigger onOk() with all-empty row text: all rows are skipped, accept() is
// called, and pathsToLoad() stays empty.
TEST_F(LoadLayersDialogTest, LoadLayersDialog_OnOk_WithEmptyPaths_AcceptsAndPathsEmpty)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    // Find and click the OK/Load button.
    QPushButton* okBtn = nullptr;
    for (auto* btn : dlg.findChildren<QPushButton*>()) {
        if (btn->text().contains("Load", Qt::CaseInsensitive) ||
            btn->text().contains("OK", Qt::CaseInsensitive)) {
            okBtn = btn;
            break;
        }
    }
    ASSERT_NE(okBtn, nullptr);
    TestUtils::dismissNextModal(50); // guard against exec() being called
    okBtn->click();
    QApplication::processEvents();
    EXPECT_TRUE(dlg.pathsToLoad().empty());
}

// Trigger onOk() with a non-existent path: checkIfPathIsSafeToAdd returns true
// for paths that cannot be opened (no such layer in the stack). The path is
// added to pathsToLoad() and accept() is called.
TEST_F(LoadLayersDialogTest, LoadLayersDialog_OnOk_WithNonExistentPath_AddsToPathList)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    // Set text in the first (non-inserter) line edit.
    auto lineEdits = dlg.findChildren<QLineEdit*>();
    ASSERT_GE(lineEdits.size(), 1);
    lineEdits.first()->setText("/nonexistent/layer_test.usd");
    QApplication::processEvents();
    // Click the OK button without dismissal since it calls accept() directly.
    QPushButton* okBtn = nullptr;
    for (auto* btn : dlg.findChildren<QPushButton*>()) {
        if (btn->text().contains("Load", Qt::CaseInsensitive) ||
            btn->text().contains("OK", Qt::CaseInsensitive)) {
            okBtn = btn;
            break;
        }
    }
    ASSERT_NE(okBtn, nullptr);
    okBtn->click();
    QApplication::processEvents();
    EXPECT_GE(dlg.pathsToLoad().size(), 1u);
}

// Trigger onOk() with a cancel: clicking cancel leaves pathsToLoad() empty.
TEST_F(LoadLayersDialogTest, LoadLayersDialog_OnCancel_LeavesPathsEmpty)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    LoadLayersDialog dlg(rootItem, _mainWindow);
    QPushButton* cancelBtn = nullptr;
    for (auto* btn : dlg.findChildren<QPushButton*>()) {
        if (btn->text().contains("Cancel", Qt::CaseInsensitive)) {
            cancelBtn = btn;
            break;
        }
    }
    ASSERT_NE(cancelBtn, nullptr);
    cancelBtn->click();
    QApplication::processEvents();
    EXPECT_TRUE(dlg.pathsToLoad().empty());
}

} // namespace UsdLayerEditor
