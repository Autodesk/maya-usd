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
#include "stageSelectorWidget.h"

#include <QtCore/QCoreApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>

#include <gtest/gtest.h>

#include <memory>

namespace UsdLayerEditor {

// Expose protected slots as public methods for headless testing.
class TestableStageSelectorWidget : public StageSelectorWidget
{
public:
    using StageSelectorWidget::StageSelectorWidget;

    void testUpdateFromSessionState() { updateFromSessionState(); }
    void testUpdateFromSessionStateWithEntry(SessionState::StageEntry const& entry)
    {
        updateFromSessionState(entry);
    }
    void testSessionStageChanged() { sessionStageChanged(); }
    void testSelectedIndexChanged(int index) { selectedIndexChanged(index); }
    void testUpdateContentButton() { updateContentButton(); }
    void testStageRenamed(SessionState::StageEntry const& entry) { stageRenamed(entry); }
    void testStageReset(SessionState::StageEntry const& entry) { stageReset(entry); }
    void testStagePinClicked() { stagePinClicked(); }
    void testCollapseContentClicked() { collapseContentClicked(); }

    QComboBox* dropDown() { return findChild<QComboBox*>(); }
};

class StageSelectorWidgetTest : public LayerEditorTestFixture
{
protected:
    std::unique_ptr<TestableStageSelectorWidget> makeWidget()
    {
        return std::make_unique<TestableStageSelectorWidget>(&_sessionState, nullptr);
    }
};

// ── updateFromSessionState ────────────────────────────────────────────────

// The combo mirrors the session's stage list.
TEST_F(StageSelectorWidgetTest, UpdateFromSessionState_PopulatesComboFromStageList)
{
    auto w = makeWidget();
    w->testUpdateFromSessionState();
    ASSERT_NE(w->dropDown(), nullptr);
    EXPECT_EQ(w->dropDown()->count(), static_cast<int>(_sessionState.allStages().size()));
}

TEST_F(StageSelectorWidgetTest, UpdateFromSessionState_Twice_KeepsComboCount)
{
    auto w = makeWidget();
    w->testUpdateFromSessionState();
    w->testUpdateFromSessionState();
    ASSERT_NE(w->dropDown(), nullptr);
    EXPECT_EQ(w->dropDown()->count(), static_cast<int>(_sessionState.allStages().size()));
}

// ── sessionStageChanged ───────────────────────────────────────────────────

// An externally-driven session stage change moves the combo to the matching entry.
TEST_F(StageSelectorWidgetTest, SessionStageChanged_SelectsMatchingComboEntry)
{
    auto w = makeWidget();
    w->testUpdateFromSessionState();
    ASSERT_NE(w->dropDown(), nullptr);
    ASSERT_GT(w->dropDown()->count(), 1);

    const auto& stages = _sessionState.allStages();
    _sessionState.setStageEntry(stages[1]);
    w->testSessionStageChanged();

    EXPECT_EQ(w->dropDown()->currentIndex(), 1);
}

// ── selectedIndexChanged ──────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, SelectedIndexChanged_IndexMinusOne_DoesNotCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testSelectedIndexChanged(-1));
}

// Selecting a combo entry pushes that entry to the session state.
TEST_F(StageSelectorWidgetTest, SelectedIndexChanged_SetsSessionStageToSelectedEntry)
{
    auto w = makeWidget();
    w->testUpdateFromSessionState();
    ASSERT_NE(w->dropDown(), nullptr);
    ASSERT_GT(w->dropDown()->count(), 0);

    w->dropDown()->setCurrentIndex(0);
    w->testSelectedIndexChanged(0);

    EXPECT_EQ(_sessionState.stageEntry(), _sessionState.allStages()[0]);
}

// ── updateContentButton ───────────────────────────────────────────────────

// The collapse-content button exists and refreshing its icon does not crash.
// (updateContentButton early-returns when the button is null, so a present
// button is what makes this path observable.)
TEST_F(StageSelectorWidgetTest, UpdateContentButton_ButtonPresent)
{
    auto w = makeWidget();
    EXPECT_FALSE(w->findChildren<QPushButton*>().isEmpty());
    EXPECT_NO_THROW(w->testUpdateContentButton());
}

// ── collapseContentClicked ────────────────────────────────────────────────

// Clicking the collapse button toggles the session's display-layer-contents flag.
TEST_F(StageSelectorWidgetTest, CollapseContentClicked_TogglesDisplayLayerContents)
{
    auto       w       = makeWidget();
    const bool initial = _sessionState.displayLayerContents();
    w->testCollapseContentClicked();
    EXPECT_EQ(_sessionState.displayLayerContents(), !initial);
}

// ── stagePinClicked ───────────────────────────────────────────────────────

// Clicking the pin button toggles whether the combo follows the UFE selection.
// The actual selection-following depends on DCC global selection state that isn't
// available in this headless test (and the old editor reads Maya UFE selection),
// so we only assert the toggle does not crash.
TEST_F(StageSelectorWidgetTest, StagePinClicked_TogglesWithoutCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testStagePinClicked());
    EXPECT_NO_THROW(w->testStagePinClicked());
}

// ── stageRenamed / stageReset ─────────────────────────────────────────────

// Renaming a stage updates the matching combo entry's text.
TEST_F(StageSelectorWidgetTest, StageRenamed_UpdatesComboItemText)
{
    auto w = makeWidget();
    w->testUpdateFromSessionState();
    ASSERT_NE(w->dropDown(), nullptr);
    ASSERT_GT(w->dropDown()->count(), 0);

    SessionState::StageEntry renamed = _sessionState.allStages()[0];
    renamed._displayName             = "RenamedStage";
    w->testStageRenamed(renamed);

    EXPECT_EQ(w->dropDown()->itemText(0), QString("RenamedStage"));
}

TEST_F(StageSelectorWidgetTest, StageReset_DefaultEntry_DoesNotCrash)
{
    auto w = makeWidget();
    SessionState::StageEntry entry;
    EXPECT_NO_THROW(w->testStageReset(entry));
}

// ── selectionChanged ──────────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, SelectionChanged_DoesNotCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->selectionChanged());
}

} // namespace UsdLayerEditor
