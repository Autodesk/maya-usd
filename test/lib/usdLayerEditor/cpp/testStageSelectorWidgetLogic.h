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
#include "stageSelectorWidget.h"

#include <QtCore/QCoreApplication>

#include <gtest/gtest.h>

#include <memory>

namespace UsdLayerEditor {

// Expose protected slots as public methods for headless testing.
class TestableStageSelectorWidget : public StageSelectorWidget
{
public:
    using StageSelectorWidget::StageSelectorWidget;

    void testUpdateFromSessionState()
    {
        updateFromSessionState();
    }
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
};

class StageSelectorWidgetTest : public LayerEditorTestFixture
{
protected:
    std::unique_ptr<TestableStageSelectorWidget> makeWidget()
    {
        return std::make_unique<TestableStageSelectorWidget>(&_sessionState, nullptr);
    }
};

// ── construction ──────────────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, Construction_DoesNotCrash)
{
    EXPECT_NO_THROW(makeWidget());
}

// ── updateFromSessionState ────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, UpdateFromSessionState_DefaultEntry_DoesNotCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testUpdateFromSessionState());
}

TEST_F(StageSelectorWidgetTest, UpdateFromSessionState_Twice_DoesNotCrash)
{
    auto w = makeWidget();
    w->testUpdateFromSessionState();
    EXPECT_NO_THROW(w->testUpdateFromSessionState());
}

// ── sessionStageChanged ───────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, SessionStageChanged_DoesNotCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testSessionStageChanged());
}

// ── selectedIndexChanged ──────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, SelectedIndexChanged_IndexMinusOne_DoesNotCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testSelectedIndexChanged(-1));
}

TEST_F(StageSelectorWidgetTest, SelectedIndexChanged_IndexZero_DoesNotCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testSelectedIndexChanged(0));
}

// ── updateContentButton ───────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, UpdateContentButton_DoesNotCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testUpdateContentButton());
}

// ── stagePinClicked ───────────────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, StagePinClicked_TogglesWithoutCrash)
{
    auto w = makeWidget();
    EXPECT_NO_THROW(w->testStagePinClicked());
    EXPECT_NO_THROW(w->testStagePinClicked());
}

// ── stageRenamed / stageReset ─────────────────────────────────────────────

TEST_F(StageSelectorWidgetTest, StageRenamed_DefaultEntry_DoesNotCrash)
{
    auto w = makeWidget();
    SessionState::StageEntry entry;
    EXPECT_NO_THROW(w->testStageRenamed(entry));
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
