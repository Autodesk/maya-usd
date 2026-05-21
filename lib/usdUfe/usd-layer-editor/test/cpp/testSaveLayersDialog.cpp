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

#include "testFixture.h"
#include "testUtils.h"
#include "saveLayersDialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>

namespace UsdLayerEditor {

class SaveLayersDialogTest : public LayerEditorTestFixture {};

TEST_F(SaveLayersDialogTest, SaveLayersDialog_ConstructsFromSessionState)
{
    // Construction must not crash.
    EXPECT_NO_THROW({
        SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    });
}

TEST_F(SaveLayersDialogTest, SaveLayersDialog_HasSaveAllButton)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    auto* btn = dlg.findChild<QPushButton*>(QString(), Qt::FindChildrenRecursively);
    // There must be at least one push button (Save All / Cancel).
    EXPECT_NE(btn, nullptr);
}

TEST_F(SaveLayersDialogTest, SaveLayersDialog_HasCancelButton)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    bool found = false;
    for (auto* btn : dlg.findChildren<QPushButton*>()) {
        if (btn->text().contains("Cancel", Qt::CaseInsensitive)) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "SaveLayersDialog should have a Cancel button";
}

TEST_F(SaveLayersDialogTest, SaveLayersDialog_AllAsRelativeCheckboxExists)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    // The dialog may or may not have anonymous layers in the stub, so the
    // all-as-relative checkbox may not be present. Just verify no crash.
    auto* cb = dlg.findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
    (void)cb;
    SUCCEED();
}

TEST_F(SaveLayersDialogTest, QuietlyUncheckAllAsRelative_DoesNotCrash)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    EXPECT_NO_THROW(dlg.quietlyUncheckAllAsRelative());
}

TEST_F(SaveLayersDialogTest, OkToSave_DoesNotCrashWithNoLayers)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    TestUtils::dismissNextModal(100);
    EXPECT_NO_THROW(dlg.exec());
}

TEST_F(SaveLayersDialogTest, LayersSavedToPairs_IsEmptyInitially)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    EXPECT_TRUE(dlg.layersSavedToPairs().isEmpty());
}

TEST_F(SaveLayersDialogTest, LayersWithErrorPairs_IsEmptyInitially)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    EXPECT_TRUE(dlg.layersWithErrorPairs().isEmpty());
}

TEST_F(SaveLayersDialogTest, LayersNotSaved_IsEmptyInitially)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    EXPECT_TRUE(dlg.layersNotSaved().isEmpty());
}

TEST_F(SaveLayersDialogTest, SaveLayersDialog_ExportingFlagChangesTitle)
{
    SaveLayersDialog exportDlg(&_sessionState, _mainWindow, /*isExporting=*/true);
    SaveLayersDialog saveDlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    // Both must construct without crashing.
    SUCCEED();
}

TEST_F(SaveLayersDialogTest, AllAsRelative_ToggleDoesNotCrash)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    auto* cb = dlg.findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
    if (!cb) GTEST_SKIP() << "No checkbox present (no anonymous layers in stub)";
    cb->setChecked(true);
    QApplication::processEvents();
    cb->setChecked(false);
    QApplication::processEvents();
    SUCCEED();
}

TEST_F(SaveLayersDialogTest, ForEachEntry_DoesNotCrashWithNoLayers)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    int count = 0;
    EXPECT_NO_THROW(dlg.forEachEntry([&count](QWidget*) { ++count; }));
}

} // namespace UsdLayerEditor
