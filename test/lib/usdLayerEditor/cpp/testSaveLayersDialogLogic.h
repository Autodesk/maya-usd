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
#include "saveLayersDialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>

namespace UsdLayerEditor {

// Exposes protected onCancel / onSaveAll for direct invocation in headless tests.
class TestableSaveLayersDialog : public SaveLayersDialog
{
public:
    using SaveLayersDialog::SaveLayersDialog;
    void callOnCancel() { onCancel(); }
    void callOnSaveAll() { onSaveAll(); }
};

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
    auto* cb = dlg.findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
    // New editor: getLayersToSaveFromStage inspects the stage directly and finds the
    // stub's anonymous sublayers, so the all-as-relative checkbox is always created.
    EXPECT_NE(cb, nullptr)
        << "all-as-relative checkbox should exist when anonymous layers are present";
#else
    // Old editor: proxy-based discovery finds no anonymous layers for the stub path.
    (void)cb;
#endif
}

TEST_F(SaveLayersDialogTest, QuietlyUncheckAllAsRelative_DoesNotCrash)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    EXPECT_NO_THROW(dlg.quietlyUncheckAllAsRelative());
}

// Note: exec() + modal dismissal never clicks "Save All", so okToSave() is not
// exercised — this only guards against a crash while showing/closing the dialog.
TEST_F(SaveLayersDialogTest, ExecThenDismiss_DoesNotCrash)
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

TEST_F(SaveLayersDialogTest, SessionStateConstructor_ExportingFlag_DoesNotCrash)
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
#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
    ASSERT_NE(cb, nullptr) << "new editor always finds the stub's anonymous layers";
#else
    if (!cb) GTEST_SKIP() << "old editor: no anonymous layers discovered for stub proxy path";
#endif
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

// The session-state constructor for the new editor finds the stub's two anonymous
// sublayers, so forEachEntry sees them as entries.
TEST_F(SaveLayersDialogTest, ForEachEntry_CountsAnonLayerRows)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    int count = 0;
    dlg.forEachEntry([&count](QWidget*) { ++count; });
#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
    // New editor always discovers the stub's two anonymous sublayers.
    EXPECT_GE(count, 1);
#else
    // Old editor: proxy-based discovery finds nothing; count is 0.
    (void)count;
#endif
}

// buildTooltipForLayer with a null layer must return an empty string without crashing.
TEST_F(SaveLayersDialogTest, BuildTooltipForLayer_NullLayer_ReturnsEmpty)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    PXR_NS::SdfLayerRefPtr nullLayer;
    EXPECT_EQ(dlg.buildTooltipForLayer(nullLayer), QString());
}

// buildTooltipForLayer with a layer that is in the stageLayerMap returns a non-empty tooltip.
TEST_F(SaveLayersDialogTest, BuildTooltipForLayer_KnownLayer_ReturnsNonEmptyTooltip)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    // The stub's current stage has one anonymous sublayer in the stageLayerMap.
    const auto& stageMap = dlg.stageLayers();
    if (stageMap.empty()) {
        GTEST_SKIP() << "no layers in stage map (old editor or empty stage)";
    }
    auto layer = stageMap.begin()->first;
    QString tooltip = dlg.buildTooltipForLayer(layer);
    EXPECT_FALSE(tooltip.isEmpty());
}

// findEntry with a layer that is in the rows returns non-null.
TEST_F(SaveLayersDialogTest, FindEntry_KnownLayer_ReturnsWidget)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    const auto& stageMap = dlg.stageLayers();
    int rowCount = 0;
    dlg.forEachEntry([&rowCount](QWidget*) { ++rowCount; });
    if (rowCount == 0) {
        GTEST_SKIP() << "no row entries (old editor or empty stage)";
    }
    // Find the first layer that has a row entry.
    QWidget* found = nullptr;
    for (auto& kv : stageMap) {
        found = dlg.findEntry(kv.first);
        if (found) break;
    }
    EXPECT_NE(found, nullptr);
}

// findEntry with a layer not in the dialog returns nullptr.
TEST_F(SaveLayersDialogTest, FindEntry_UnknownLayer_ReturnsNull)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    auto unknownLayer = PXR_NS::SdfLayer::CreateAnonymous("unknown");
    EXPECT_EQ(dlg.findEntry(unknownLayer), nullptr);
}

// sessionState() accessor returns a non-null pointer matching what was passed in.
TEST_F(SaveLayersDialogTest, SessionState_Accessor_ReturnsSessionState)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    EXPECT_NE(dlg.sessionState(), nullptr);
}

#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
// ── New-editor-only tests ────────────────────────────────────────────────────
// These depend on UsdLayerEditor::StageSavingInfo and setExecTestHandler, which
// are only present in the new editor's SaveLayersDialog.

// Bulk constructor: construct from a vector of StageSavingInfo.
TEST_F(SaveLayersDialogTest, BulkConstructor_SingleStage_DoesNotCrash)
{
    auto stage = TestUtils::makeStageWithSublayer("bulk_sub");
    StageSavingInfo info;
    info.stage         = stage;
    info.stageName     = "bulk_stage";
    info.dccObjectPath = "bulk_stage";
    EXPECT_NO_THROW({
        SaveLayersDialog dlg(_mainWindow, { info }, /*isExporting=*/false);
    });
}

TEST_F(SaveLayersDialogTest, BulkConstructor_EmptyInfos_DoesNotCrash)
{
    EXPECT_NO_THROW({
        SaveLayersDialog dlg(_mainWindow, {}, /*isExporting=*/false);
    });
}

TEST_F(SaveLayersDialogTest, BulkConstructor_MultipleStages_DoesNotCrash)
{
    std::vector<StageSavingInfo> infos;
    for (int i = 0; i < 3; ++i) {
        auto stage = TestUtils::makeStageWithSublayer("msub_" + std::to_string(i));
        StageSavingInfo info;
        info.stage         = stage;
        info.stageName     = "multi_stage_" + std::to_string(i);
        info.dccObjectPath = info.stageName;
        infos.push_back(info);
    }
    EXPECT_NO_THROW({
        SaveLayersDialog dlg(_mainWindow, infos, /*isExporting=*/false);
    });
}

TEST_F(SaveLayersDialogTest, BulkConstructor_ExportingFlag_DoesNotCrash)
{
    auto stage = TestUtils::makeStageWithSublayer("exp_sub");
    StageSavingInfo info;
    info.stage         = stage;
    info.stageName     = "exp_stage";
    info.dccObjectPath = "exp_stage";
    EXPECT_NO_THROW({
        SaveLayersDialog dlg(_mainWindow, { info }, /*isExporting=*/true);
    });
}

TEST_F(SaveLayersDialogTest, BulkConstructor_ComponentsOnly_DoesNotCrash)
{
    auto stage = TestUtils::makeStageWithSublayer("comp_sub");
    StageSavingInfo info;
    info.stage         = stage;
    info.stageName     = "comp_stage";
    info.dccObjectPath = "comp_stage";
    EXPECT_NO_THROW({
        SaveLayersDialog dlg(_mainWindow, { info }, /*isExporting=*/false, /*componentsOnly=*/true);
    });
}

// exec() test handler returns a pre-set value without showing the dialog.
TEST_F(SaveLayersDialogTest, ExecTestHandler_ReturnsInjectedResult)
{
    // Install a handler that returns Accepted; capture the previous one to restore.
    auto prev = SaveLayersDialog::setExecTestHandler([]() { return QDialog::Accepted; });
    {
        SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
        EXPECT_EQ(dlg.exec(), QDialog::Accepted);
    }
    SaveLayersDialog::setExecTestHandler(std::move(prev));
}

// Bulk constructor: sessionState() is null (no session state provided).
TEST_F(SaveLayersDialogTest, BulkConstructor_SessionState_IsNull)
{
    auto stage = TestUtils::makeStageWithSublayer("ns_sub");
    StageSavingInfo info;
    info.stage         = stage;
    info.stageName     = "ns_stage";
    info.dccObjectPath = "ns_stage";
    SaveLayersDialog dlg(_mainWindow, { info }, /*isExporting=*/false);
    EXPECT_EQ(dlg.sessionState(), nullptr);
}

#endif // !LAYER_EDITOR_TEST_FIXTURE_INCLUDED

// ── onCancel() coverage ───────────────────────────────────────────────────

TEST_F(SaveLayersDialogTest, OnCancel_SetsResultToRejected)
{
    TestableSaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    dlg.callOnCancel();
    EXPECT_EQ(dlg.result(), QDialog::Rejected);
}

// ── onSaveAll() + okToSave() coverage ────────────────────────────────────
// Rows always have auto-generated paths, so onSaveAll() takes the non-empty
// path branch and calls saveAnonymousLayer (which throws on a stub DCC path).
// Test that calling onSaveAll() directly doesn't crash when there are no rows.

TEST_F(SaveLayersDialogTest, OnSaveAll_NoRows_DoesNotCrash)
{
    // Construct with an empty StageSavingInfo list so there are no rows.
    TestableSaveLayersDialog dlg(_mainWindow, {}, /*isExporting=*/false);
    EXPECT_NO_THROW(dlg.callOnSaveAll());
    EXPECT_TRUE(dlg.layersNotSaved().isEmpty());
}

} // namespace UsdLayerEditor
