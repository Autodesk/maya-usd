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

#include "testFixture.h"

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

namespace UsdLayerEditor {

static QPushButton* findButtonByTooltip(QWidget* root, const QString& tooltip)
{
    for (auto* btn : root->findChildren<QPushButton*>()) {
        if (btn->toolTip().contains(tooltip, Qt::CaseInsensitive)) {
            return btn;
        }
    }
    return nullptr;
}

// Clicking "Add a New Layer" with nothing selected inserts an anonymous sublayer at the root.
TEST_F(LayerEditorTestFixture, NewLayerButton_Click_CallsAddAnonymousSubLayer)
{
    QPushButton* btn = findButtonByTooltip(_widget, "Add a New Layer");
    ASSERT_NE(btn, nullptr) << "Could not find New Layer button";
    ASSERT_TRUE(btn->isEnabled()) << "New Layer button should be enabled";

    btn->click();
    QApplication::processEvents();

    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("addAnonymousSubLayer"))
        << "addAnonymousSubLayer should have been called";
}

// The "Load an Existing Layer" button should be present and enabled.
// We do not click it here because it shows a modal file-picker dialog.
TEST_F(LayerEditorTestFixture, LoadLayerButton_ExistsAndEnabled)
{
    QPushButton* btn = findButtonByTooltip(_widget, "Load an Existing Layer");
    ASSERT_NE(btn, nullptr) << "Could not find Load Layer button";
    EXPECT_TRUE(btn->isEnabled()) << "Load Layer button should be enabled";
}

TEST_F(LayerEditorTestFixture, SaveStageButton_EnabledWhenDirty)
{
    QPushButton* btn = findButtonByTooltip(_widget, "Save all edits in the Layer Stack");
    ASSERT_NE(btn, nullptr) << "Could not find Save Stage button";

    auto stage = _sessionState.stage();
    ASSERT_TRUE(stage);
    stage->GetRootLayer()->SetComment("dirty");

    // Pump twice: first pass schedules the idle update, second fires it.
    QApplication::processEvents();
    QApplication::processEvents();

    EXPECT_TRUE(btn->isEnabled()) << "Save Stage button should be enabled when stage is dirty";
}

// Clicking the Save button may show a SaveLayersDialog (anonymous layers in the stage).
// We schedule a timer to dismiss the modal so the test does not hang.
TEST_F(LayerEditorTestFixture, SaveStageButton_Click_DismissesDialog)
{
    QPushButton* btn = findButtonByTooltip(_widget, "Save all edits in the Layer Stack");
    ASSERT_NE(btn, nullptr) << "Could not find Save Stage button";

    auto stage = _sessionState.stage();
    ASSERT_TRUE(stage);
    stage->GetRootLayer()->SetComment("dirty");
    QApplication::processEvents();
    QApplication::processEvents();

    ASSERT_TRUE(btn->isEnabled());

    // Dismiss any modal dialog that saveStage() might show.
    QTimer::singleShot(200, []() {
        QWidget* modal = QApplication::activeModalWidget();
        if (modal)
            modal->close();
    });

    btn->click();
    QApplication::processEvents();
    // Reaching here without a crash or hang is the pass criterion.
}

} // namespace UsdLayerEditor
