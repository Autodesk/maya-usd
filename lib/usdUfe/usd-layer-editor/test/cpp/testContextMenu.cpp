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

#include "layerLocking.h"

#include <QtWidgets/QApplication>

namespace UsdLayerEditor {

// ------------------------------------------------------------------
// Core operations — called directly on the window after selecting
// the appropriate tree row, so they don't depend on QMenu::exec().
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, ContextMenu_AddAnonymousSublayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->addAnonymousSublayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("addAnonymousSubLayer"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_MuteLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->muteLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("muteSubLayer"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_LockLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->lockLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("lockLayer"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_RemoveLayer_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->removeSubLayer();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("removeSubLayerPath"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->discardEdits();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("discardEdits"));
}

TEST_F(LayerEditorTestFixture, ContextMenu_PrintLayer_CallsSessionState)
{
    selectRow(firstSublayerIndex());
    _window->printLayer();
    QApplication::processEvents();
    EXPECT_GT(_sessionState._printLayerCallCount, 0);
}

TEST_F(LayerEditorTestFixture, ContextMenu_SelectPrimsWithSpec_CallsHook)
{
    selectRow(firstSublayerIndex());
    _window->selectPrimsWithSpec();
    QApplication::processEvents();
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("selectPrimsWithSpec"));
}

// ------------------------------------------------------------------
// Layer-type queries via the window's state methods
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, LayerQuery_SessionLayer_IsSessionLayer)
{
    selectRow(sessionLayerIndex());
    EXPECT_TRUE(_window->isSessionLayer());
}

TEST_F(LayerEditorTestFixture, LayerQuery_Sublayer_IsNotSessionLayer)
{
    selectRow(firstSublayerIndex());
    EXPECT_FALSE(_window->isSessionLayer());
}

TEST_F(LayerEditorTestFixture, LayerQuery_Sublayer_IsSubLayer)
{
    selectRow(firstSublayerIndex());
    EXPECT_TRUE(_window->isSubLayer());
}

TEST_F(LayerEditorTestFixture, LayerQuery_SessionLayer_IsNotSubLayer)
{
    selectRow(sessionLayerIndex());
    EXPECT_FALSE(_window->isSubLayer());
}

// ------------------------------------------------------------------
// Lock state affects isLocked() query
// ------------------------------------------------------------------

TEST_F(LayerEditorTestFixture, ContextMenu_LockedLayer_IsLocked)
{
    selectRow(firstSublayerIndex());
    // Lock the sublayer directly via the command hook.
    _window->lockLayer();
    QApplication::processEvents();

    // Re-select the row so the window refreshes its current item.
    selectRow(firstSublayerIndex());
    EXPECT_TRUE(_window->layerIsLocked())
        << "Layer should report locked after lockLayer()";
}

TEST_F(LayerEditorTestFixture, ContextMenu_UnlockedLayer_IsNotLocked)
{
    selectRow(firstSublayerIndex());
    EXPECT_FALSE(_window->layerIsLocked())
        << "Fresh sublayer should not be locked";
}

} // namespace UsdLayerEditor
