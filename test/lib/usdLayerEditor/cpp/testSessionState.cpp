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

#include "sessionState.h"
// stubSessionState.h is already included by testFixture.h in both old/new builds;
// including it here via relative path would find the wrong version in the old-editor build.
#ifndef MAYAUSD_OLD_LAYER_EDITOR
#include "stubSessionState.h"
#endif

#include <QtWidgets/QApplication>
#include <gtest/gtest.h>

namespace UsdLayerEditor {

// setAutoHideSessionLayer emits the autoHideSessionLayerSignal.
TEST_F(LayerEditorTestFixture, SessionState_SetAutoHideSessionLayer_EmitsSignal)
{
    int signalCount = 0;
    bool lastValue = false;
    QObject::connect(
        &_sessionState,
        &SessionState::autoHideSessionLayerSignal,
        &_sessionState,
        [&signalCount, &lastValue](bool v) {
            ++signalCount;
            lastValue = v;
        });

    // Call the base implementation directly, bypassing the stub override.
    _sessionState.SessionState::setAutoHideSessionLayer(true);
    EXPECT_EQ(signalCount, 1);
    EXPECT_TRUE(lastValue);

    _sessionState.SessionState::setAutoHideSessionLayer(false);
    EXPECT_EQ(signalCount, 2);
    EXPECT_FALSE(lastValue);
}

// setDisplayLayerContents writes the flag (accessible via the public getter) and emits.
TEST_F(LayerEditorTestFixture, SessionState_SetDisplayLayerContents_UpdatesAndEmits)
{
    int signalCount = 0;
    QObject::connect(
        &_sessionState,
        &SessionState::showDisplayLayerContents,
        &_sessionState,
        [&signalCount](bool) { ++signalCount; });

    // Read via base getter (stub does not override this one).
    bool initial = _sessionState.displayLayerContents();
    _sessionState.setDisplayLayerContents(!initial);
    EXPECT_EQ(signalCount, 1);
    EXPECT_EQ(_sessionState.displayLayerContents(), !initial);
}

#ifndef MAYAUSD_OLD_LAYER_EDITOR
// setDisplayLayerExpandAllValues writes the flag and emits showDisplayLayerContents.
TEST_F(LayerEditorTestFixture, SessionState_SetDisplayLayerExpandAllValues_UpdatesAndEmits)
{
    int signalCount = 0;
    QObject::connect(
        &_sessionState,
        &SessionState::showDisplayLayerContents,
        &_sessionState,
        [&signalCount](bool) { ++signalCount; });

    bool initial = _sessionState.displayLayerExpandAllValues();
    _sessionState.setDisplayLayerExpandAllValues(!initial);
    EXPECT_EQ(signalCount, 1);
    EXPECT_EQ(_sessionState.displayLayerExpandAllValues(), !initial);
}
#endif

#ifndef MAYAUSD_OLD_LAYER_EDITOR
// setDisplayLayerHideIndices writes the flag and emits showDisplayLayerContents.
TEST_F(LayerEditorTestFixture, SessionState_SetDisplayLayerHideIndices_UpdatesAndEmits)
{
    int signalCount = 0;
    QObject::connect(
        &_sessionState,
        &SessionState::showDisplayLayerContents,
        &_sessionState,
        [&signalCount](bool) { ++signalCount; });

    bool initial = _sessionState.displayLayerHideIndices();
    _sessionState.setDisplayLayerHideIndices(!initial);
    EXPECT_EQ(signalCount, 1);
    EXPECT_EQ(_sessionState.displayLayerHideIndices(), !initial);
}
#endif

#ifndef MAYAUSD_OLD_LAYER_EDITOR
// setStageEntry with a different entry emits currentStageChangedSignal.
TEST_F(LayerEditorTestFixture, SessionState_SetStageEntry_NewEntry_EmitsSignal)
{
    int signalCount = 0;
    QObject::connect(
        &_sessionState,
        &SessionState::currentStageChangedSignal,
        &_sessionState,
        [&signalCount]() { ++signalCount; });

    auto newStage = PXR_NS::UsdStage::CreateInMemory();
    SessionState::StageEntry newEntry;
    newEntry._id            = "test_stage";
    newEntry._stage         = newStage;
    newEntry._displayName   = "test_stage";
    newEntry._dccObjectPath = "test_stage";

    _sessionState.setStageEntry(newEntry);
    EXPECT_EQ(signalCount, 1);
}
#endif

// setStageEntry with the same entry must NOT emit.
TEST_F(LayerEditorTestFixture, SessionState_SetStageEntry_SameEntry_NoSignal)
{
    int signalCount = 0;
    QObject::connect(
        &_sessionState,
        &SessionState::currentStageChangedSignal,
        &_sessionState,
        [&signalCount]() { ++signalCount; });

    _sessionState.setStageEntry(_sessionState.stageEntry());
    EXPECT_EQ(signalCount, 0);
}

// targetLayer returns null when no stage is active.
#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerEditorTestFixture, SessionState_TargetLayer_NullWhenNoStage)
{
    StubSessionState emptyState;
    // Replace the stub's stage with an empty entry so the null path is hit.
    SessionState::StageEntry empty;
    emptyState.SessionState::setStageEntry(empty);
    EXPECT_EQ(emptyState.targetLayer(), nullptr);
}

// isValid returns false when no stage is set.
TEST_F(LayerEditorTestFixture, SessionState_IsValid_FalseWhenNoStage)
{
    StubSessionState emptyState;
    SessionState::StageEntry empty;
    emptyState.SessionState::setStageEntry(empty);
    EXPECT_FALSE(emptyState.isValid());
}
#endif // !MAYAUSD_OLD_LAYER_EDITOR

// isValid returns true with a valid stage.
TEST_F(LayerEditorTestFixture, SessionState_IsValid_TrueWithValidStage)
{
    EXPECT_TRUE(_sessionState.isValid());
}

} // namespace UsdLayerEditor
