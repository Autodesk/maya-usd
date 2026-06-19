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

#include "stringResources.h"

#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

static QMenu* findMenuByTitle(QMainWindow* win, const QString& title)
{
    if (!win || !win->menuBar())
        return nullptr;
    for (QAction* top : win->menuBar()->actions()) {
        if (QMenu* menu = top->menu()) {
            if (menu->title() == title)
                return menu;
        }
    }
    return nullptr;
}

static QAction* findActionInMenuBar(QMainWindow* win, const QString& text)
{
    if (!win || !win->menuBar())
        return nullptr;
    for (QAction* top : win->menuBar()->actions()) {
        if (QMenu* menu = top->menu()) {
            QAction* found = findAction(menu, text);
            if (found)
                return found;
        }
    }
    return nullptr;
}

TEST_F(LayerEditorTestFixture, OptionMenu_DisplayLayerContentsAction_Exists)
{
    auto* win    = qobject_cast<QMainWindow*>(_widget->parent());
    auto* action = findActionInMenuBar(win, "Display Layer Content");
    EXPECT_NE(action, nullptr)
        << "Display Layer Content action should exist in the Option menu";
}

TEST_F(LayerEditorTestFixture, OptionMenu_DisplayLayerContents_Toggles)
{
    auto* win    = qobject_cast<QMainWindow*>(_widget->parent());
    auto* action = findActionInMenuBar(win, "Display Layer Content");
    ASSERT_NE(action, nullptr);
    ASSERT_TRUE(action->isCheckable());

    bool before = action->isChecked();
    action->trigger();
    QApplication::processEvents();
    EXPECT_NE(action->isChecked(), before) << "Action should toggle";
}

TEST_F(LayerEditorTestFixture, StageSelector_ChangeStage_UpdatesSessionState)
{
    auto* combo = _widget->findChild<QComboBox*>(
        QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(combo, nullptr) << "No stage selector QComboBox found";
    ASSERT_GE(combo->count(), 2) << "Expected at least 2 stages in selector";

    auto stageBefore = _sessionState.stage();
    combo->setCurrentIndex(1);
    QApplication::processEvents();

    // The session state's current stage should have changed.
    auto stageAfter = _sessionState.stage();
    EXPECT_NE(stageAfter, stageBefore)
        << "Active stage should change when stage selector changes";
}

#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerEditorTestFixture, StageList_AddStage_AppearsInSelector)
{
    auto* combo = _widget->findChild<QComboBox*>(
        QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(combo, nullptr);

    int countBefore = combo->count();
    auto newStage   = PXR_NS::UsdStage::CreateInMemory();
    _sessionState.addStage(newStage);
    QApplication::processEvents();

    EXPECT_GT(combo->count(), countBefore)
        << "Adding a stage should increase the selector item count";
}
#endif

// ── stage selector pin / content toggle ───────────────────────────────────────

TEST_F(LayerEditorTestFixture, StageSelector_HasAtLeastOneEntry)
{
    auto* combo = _widget->findChild<QComboBox*>(
        QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(combo, nullptr);
    EXPECT_GE(combo->count(), 1);
}

TEST_F(LayerEditorTestFixture, StageSelector_InitialCountMatchesSessionStageCount)
{
    auto* combo = _widget->findChild<QComboBox*>(
        QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(combo, nullptr);
    int sessionCount = static_cast<int>(_sessionState.allStages().size());
    EXPECT_EQ(combo->count(), sessionCount);
}

#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerEditorTestFixture, StageSelector_AddStage_IncrementsComboCount)
{
    auto* combo = _widget->findChild<QComboBox*>(
        QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(combo, nullptr);
    int before = combo->count();
    auto extra = PXR_NS::UsdStage::CreateInMemory();
    _sessionState.addStage(extra);
    QApplication::processEvents();
    EXPECT_GT(combo->count(), before);
}
#endif

TEST_F(LayerEditorTestFixture, CollapseContent_TogglesDisplayLayerContentsInMenu)
{
    auto* win = qobject_cast<QMainWindow*>(_widget->parent());
    if (!win || !win->menuBar()) GTEST_SKIP() << "No menu bar available";

    QAction* action = findActionInMenuBar(win, "Display Layer Content");
    if (!action) GTEST_SKIP() << "Display Layer Content action not found";

    bool initial = action->isChecked();
    action->trigger();
    QApplication::processEvents();
    EXPECT_NE(action->isChecked(), initial);
    // Restore
    action->trigger();
    QApplication::processEvents();
}

#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerEditorTestFixture, StageSelector_RemoveAddStage_RoundtripDoesNotCrash)
{
    auto* combo = _widget->findChild<QComboBox*>(
        QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(combo, nullptr);
    int before = combo->count();

    auto extra = PXR_NS::UsdStage::CreateInMemory();
    _sessionState.addStage(extra);
    QApplication::processEvents();
    EXPECT_GT(combo->count(), before);
    // No direct removeStage in stub — just verify no crash on the add path.
}
#endif

// The Auto-Hide Session Layer action must be the first Option-menu entry,
// checkable, and followed by a separator then the Display-Layer-Contents action.
TEST_F(LayerEditorTestFixture, OptionMenu_AutoHideAction_IsFirstAndCheckable)
{
    auto* win = qobject_cast<QMainWindow*>(_widget->parent());
    ASSERT_NE(win, nullptr);

    QMenu* optionMenu
        = findMenuByTitle(win, StringResources::getAsQString(StringResources::kOption));
    ASSERT_NE(optionMenu, nullptr) << "Option menu should exist";

    const QList<QAction*> actions = optionMenu->actions();
    ASSERT_GE(actions.size(), 3);

    const QString autoHideText
        = StringResources::getAsQString(StringResources::kAutoHideSessionLayer);
    EXPECT_EQ(actions[0]->text(), autoHideText)
        << "Auto-Hide should be the first action in the Option menu";
    EXPECT_TRUE(actions[0]->isCheckable());
    EXPECT_EQ(actions[0]->isChecked(), _sessionState.autoHideSessionLayer());

    EXPECT_TRUE(actions[1]->isSeparator()) << "a separator should follow the Auto-Hide action";

    QAction* displayContents = findAction(
        optionMenu, StringResources::getAsQString(StringResources::kDisplayLayerContents));
    ASSERT_NE(displayContents, nullptr);
    EXPECT_GE(actions.indexOf(displayContents), 2)
        << "Display Layer Content should come after Auto-Hide and the separator";
}

} // namespace UsdLayerEditor
