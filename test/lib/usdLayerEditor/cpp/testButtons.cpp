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

#include <testFixture.h>
#include "testUtils.h"
#include "layerLocking.h"
#include "layerTreeItem.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

#include <pxr/usd/usd/stage.h>

namespace UsdLayerEditor {

// The Save Stage button is only created, shown, and enable-managed on a shared
// stage; updateButtons() leaves it hidden and unmanaged otherwise.
class ButtonsSharedStageFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        setSharedStage(true);
        LayerEditorTestFixture::SetUp();
        QApplication::processEvents();
    }
};

// Shared stage backed by a real file: root layer is non-anonymous and initially
// clean, so the Save button starts disabled and only enables when dirty.
// Not compiled for the old editor: switchToCustomStage is new-editor-only API.
#ifndef MAYAUSD_OLD_LAYER_EDITOR
class SaveStageCleanNonAnonFixture : public LayerEditorTestFixture
{
protected:
    QString _stagePath;

    void SetUp() override
    {
        setSharedStage(true);
        LayerEditorTestFixture::SetUp();

        _stagePath = QDir::tempPath() + "/le_save_clean_test.usda";
        QFile::remove(_stagePath);
        auto stage = PXR_NS::UsdStage::CreateNew(_stagePath.toStdString());
        _sessionState.switchToCustomStage(stage);
        QApplication::processEvents();
        QApplication::processEvents();
    }

    void TearDown() override
    {
        LayerEditorTestFixture::TearDown();
        QFile::remove(_stagePath);
    }
};
#endif

// Clicking "Add a New Layer" with nothing selected inserts an anonymous sublayer at the root.
TEST_F(LayerEditorTestFixture, NewLayerButton_Click_CallsAddAnonymousSubLayer)
{
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr) << "Could not find New Layer button";
    ASSERT_TRUE(btn->isEnabled()) << "New Layer button should be enabled";

    _sessionState._commandHookImpl.clearCalls();
    btn->click();
    QApplication::processEvents();

    const auto* call = _sessionState._commandHookImpl.lastCallOf("addAnonymousSubLayer");
    ASSERT_NE(call, nullptr) << "addAnonymousSubLayer should have been called";
    EXPECT_EQ(call->args[0], _sessionState.stage()->GetRootLayer()->GetIdentifier())
        << "addAnonymousSubLayer should target the root layer when nothing is selected";
}

// The "Load an Existing Layer" button should be present and enabled.
// We do not click it here because it shows a modal file-picker dialog.
TEST_F(LayerEditorTestFixture, LoadLayerButton_ExistsAndEnabled)
{
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Load an Existing Layer");
    ASSERT_NE(btn, nullptr) << "Could not find Load Layer button";
    EXPECT_TRUE(btn->isEnabled()) << "Load Layer button should be enabled";
}

TEST_F(ButtonsSharedStageFixture, SaveStageButton_EnabledWhenDirty)
{
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Save all edits in the Layer Stack");
    ASSERT_NE(btn, nullptr) << "Could not find Save Stage button";

    auto stage = _sessionState.stage();
    ASSERT_TRUE(stage);
    // Note: the stub stage's root is anonymous, so needsSaving() is already true before
    // this SetComment call (anonymous layers are treated as always needing to be saved).
    // This test confirms that setting a comment (making it additionally dirty) keeps the
    // button enabled — for the disabled-then-enabled transition see
    // SaveStageButton_DisabledInitially_EnabledWhenDirty.
    stage->GetRootLayer()->SetComment("dirty");

    // Pump twice: updateButtons() uses QTimer::singleShot(0,...), so the first
    // processEvents() posts the deferred update and the second fires it.
    QApplication::processEvents();
    QApplication::processEvents();

    EXPECT_TRUE(btn->isVisible()) << "Save Stage button should be shown on a shared stage";
    EXPECT_TRUE(btn->isEnabled()) << "Save Stage button should be enabled when stage is dirty";
}

// ── updateNewLayerButton enable/disable matrix ────────────────────────────────

// When nothing is selected the button falls back to the root layer and stays enabled.
TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledWhenNoSelectionDefaultsToRoot)
{
    layerTree()->selectionModel()->clearSelection();
    QApplication::processEvents();
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->isEnabled());
}

TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledForRootLayer)
{
    selectRow(rootLayerIndex());
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->isEnabled());
}

TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledForSessionLayer)
{
    selectRow(sessionLayerIndex());
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->isEnabled());
}

TEST_F(LayerEditorTestFixture, NewLayerButton_DisabledWhenSelectionIsLocked)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    TestUtils::lockLayerDirect(rootItem->layer());

    selectRow(rootLayerIndex());
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isEnabled());

    TestUtils::unlockLayerDirect(rootItem->layer());
}

TEST_F(LayerEditorTestFixture, NewLayerButton_DisabledWhenSelectionIsSystemLocked)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);
    addSystemLockedLayer(rootItem->layer());
    rootItem->layer()->SetPermissionToEdit(false);

    selectRow(rootLayerIndex());
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isEnabled());

    removeSystemLockedLayer(rootItem->layer());
    TestUtils::unlockLayerDirect(rootItem->layer());
}

TEST_F(LayerEditorTestFixture, SaveButton_HiddenWhenStageIsNotShared)
{
    // When the stage is not a shared stage, updateButtons() hides the save button.
    QApplication::processEvents();
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Save all edits");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isVisible());
}

// Selecting a sublayer enables the button — clicking adds a sibling at that position.
TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledForSublayerSelection)
{
    selectRow(firstSublayerIndex());
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->isEnabled());
}

TEST_F(LayerEditorTestFixture, NewLayerButton_Click_WithSublayerSelectionAddsSibling)
{
    selectRow(firstSublayerIndex());
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    ASSERT_TRUE(btn->isEnabled());

    _sessionState._commandHookImpl.clearCalls();
    btn->click();
    QApplication::processEvents();

    // Adding a sibling means inserting into the selected layer's parent (root).
    const auto* call = _sessionState._commandHookImpl.lastCallOf("addAnonymousSubLayer");
    ASSERT_NE(call, nullptr) << "addAnonymousSubLayer should be called on the parent when adding a sibling";
    EXPECT_EQ(call->args[0], _sessionState.stage()->GetRootLayer()->GetIdentifier())
        << "parent should be the root layer when a direct sublayer of root is selected";
}

TEST_F(LayerEditorTestFixture, ToolbarButtons_HaveObjectNames)
{
    EXPECT_TRUE(_widget->findChild<QPushButton*>("LayerEditorAddLayerButton"));
    EXPECT_TRUE(_widget->findChild<QPushButton*>("LayerEditorImportLayerButton"));
    EXPECT_TRUE(_widget->findChild<QPushButton*>("LayerEditorSaveAllButton"));
}

// ── Save Stage button: disabled → enabled transition ─────────────────────────

// With a file-backed (non-anonymous), clean stage the Save button starts disabled.
// Making the stage dirty must enable it — the transition proves the button actually
// tracks needsSaving() rather than being permanently enabled by isAnonymous().
#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(SaveStageCleanNonAnonFixture, SaveStageButton_DisabledInitially_EnabledWhenDirty)
{
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Save all edits in the Layer Stack");
    ASSERT_NE(btn, nullptr) << "Could not find Save Stage button";

    EXPECT_TRUE(btn->isVisible()) << "Save Stage button should be shown for a shared stage";
    EXPECT_FALSE(btn->isEnabled()) << "Save Stage button should be disabled for a clean non-anonymous stage";

    _sessionState.stage()->GetRootLayer()->SetComment("dirty");
    QApplication::processEvents();
    QApplication::processEvents();

    EXPECT_TRUE(btn->isEnabled()) << "Save Stage button should be enabled after stage becomes dirty";
}
#endif

// ── New Layer button: disabled → enabled transition ───────────────────────────

// Selecting a locked layer disables the button; switching to an unlocked layer
// must re-enable it, demonstrating the disabled→enabled path.
TEST_F(LayerEditorTestFixture, NewLayerButton_ReenablesAfterSwitchFromLockedToUnlockedSelection)
{
    auto* rootItem = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(rootItem, nullptr);

    TestUtils::lockLayerDirect(rootItem->layer());
    selectRow(rootLayerIndex());
    QPushButton* btn = TestUtils::findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr);
    EXPECT_FALSE(btn->isEnabled()) << "New Layer button should be disabled for a locked layer";

    TestUtils::unlockLayerDirect(rootItem->layer());
    selectRow(firstSublayerIndex()); // select an unlocked layer
    EXPECT_TRUE(btn->isEnabled()) << "New Layer button should be re-enabled after switching to unlocked selection";
}

} // namespace UsdLayerEditor
