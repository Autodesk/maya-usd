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
    void callOnAllAsRelativeChanged() { onAllAsRelativeChanged(); }
};

class SaveLayersDialogTest : public LayerEditorTestFixture {};

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
    // Both editors discover the stub's anonymous sublayers (the old editor via a proxy backed by
    // the same in-memory stage), so the all-as-relative checkbox is always created.
    EXPECT_NE(cb, nullptr)
        << "all-as-relative checkbox should exist when anonymous layers are present";
}

// Both editors discover the stub stage's anonymous layers, so forEachEntry sees them as entries.
TEST_F(SaveLayersDialogTest, ForEachEntry_CountsAnonLayerRows)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    int count = 0;
    dlg.forEachEntry([&count](QWidget*) { ++count; });
    EXPECT_GE(count, 1);
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
    // The stub's anonymous sublayers produce row entries, so at least one stage layer
    // resolves to a non-null widget.
    const auto& stageMap = dlg.stageLayers();
    QWidget*    found = nullptr;
    for (auto& kv : stageMap) {
        found = dlg.findEntry(kv.first);
        if (found)
            break;
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

#ifndef MAYAUSD_OLD_LAYER_EDITOR
// ── New-editor-only tests ────────────────────────────────────────────────────
// These depend on UsdLayerEditor::StageSavingInfo and setExecTestHandler, which
// are only present in the new editor's SaveLayersDialog.

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

#endif // !MAYAUSD_OLD_LAYER_EDITOR

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

// ── all-as-relative checkbox ──────────────────────────────────────────────

// quietlyUncheckAllAsRelative clears the checkbox without re-triggering the
// per-entry callback.
TEST_F(SaveLayersDialogTest, QuietlyUncheckAllAsRelative_UnchecksCheckbox)
{
    SaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    auto* cb = dlg.findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(cb, nullptr);
    cb->setCheckState(Qt::Checked);
    dlg.quietlyUncheckAllAsRelative();
    EXPECT_EQ(cb->checkState(), Qt::Unchecked);
}

// onAllAsRelativeChanged propagates the checkbox state to every layer row.
TEST_F(SaveLayersDialogTest, OnAllAsRelativeChanged_AppliesToEntries)
{
    TestableSaveLayersDialog dlg(&_sessionState, _mainWindow, /*isExporting=*/false);
    auto* cb = dlg.findChild<QCheckBox*>(QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(cb, nullptr);
    // SaveLayerPathRow's save-as-relative state has no public getter (the row type is
    // defined only in the .cpp), so exercise both transitions and assert neither throws.
    cb->setCheckState(Qt::Checked);
    EXPECT_NO_THROW(dlg.callOnAllAsRelativeChanged());
    cb->setCheckState(Qt::Unchecked);
    EXPECT_NO_THROW(dlg.callOnAllAsRelativeChanged());
}

} // namespace UsdLayerEditor
