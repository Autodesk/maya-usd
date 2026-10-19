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
// The render setup entry button, and what an active render layer does to targeting and to the
// context menus. The DCC is stood in for by the render layer hooks of the test fixture.
//
#include "layerTreeItem.h"
#include "stringResources.h"
#include "testUtils.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include <testFixture.h>

namespace UsdLayerEditor {

namespace {

constexpr auto kButtonName = "LayerEditorRenderSetupButton";

struct MenuEntry
{
    QString text;
    bool    enabled;
};

// Builds the layer editor context menu and returns what it showed. The menu runs its own event
// loop, so a timer reads it and closes it from inside that loop.
std::vector<MenuEntry> showContextMenu(LayerEditorWindow& window)
{
    std::vector<MenuEntry> entries;
    auto                   timer = std::make_unique<QTimer>();
    int                    attempts = 0;
    QObject::connect(timer.get(), &QTimer::timeout, [&]() {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu && ++attempts < 200) {
            return;
        }
        if (menu) {
            for (auto* action : menu->actions()) {
                if (!action->isSeparator()) {
                    entries.push_back({ action->text(), action->isEnabled() });
                }
            }
            menu->close();
        }
        timer->stop();
    });
    timer->start(10);
    window.buildContextMenu(QPoint(0, 0));
    return entries;
}

const MenuEntry* findEntry(const std::vector<MenuEntry>& entries, const QString& text)
{
    for (const auto& entry : entries) {
        if (entry.text == text) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

// The button is created only when the DCC says its render setup system is the preferred one.
class LayerEditorWithRenderSetupFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        setRenderSetupSupported(true);
        LayerEditorTestFixture::SetUp();
    }

    // Makes the layer of the first sublayer the active render layer and refreshes the editor.
    LayerTreeItem* activateRenderLayerOnFirstSublayer()
    {
        auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
        _activeRenderLayer = item->layer();
        treeModel()->forceRefresh();
        _widget->updateButtonsOnIdle();
        QApplication::processEvents();
        return treeModel()->layerItemFromIndex(firstSublayerIndex());
    }

    void deactivateRenderLayer()
    {
        _activeRenderLayer = nullptr;
        treeModel()->forceRefresh();
        _widget->updateButtonsOnIdle();
        QApplication::processEvents();
    }
};

// ── The button ──────────────────────────────────────────────────────────────────

TEST_F(LayerEditorTestFixture, RenderSetupButton_AbsentWhenNotSupported)
{
    EXPECT_EQ(TestUtils::findButtonByObjectName(_widget, kButtonName), nullptr);
}

TEST_F(LayerEditorWithRenderSetupFixture, RenderSetupButton_CreatedWhenSupported)
{
    EXPECT_NE(TestUtils::findButtonByObjectName(_widget, kButtonName), nullptr);
}

TEST_F(LayerEditorWithRenderSetupFixture, RenderSetupButton_TooltipFollowsActiveRenderLayer)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, kButtonName);
    ASSERT_NE(btn, nullptr);
    QApplication::processEvents();
    EXPECT_EQ(
        btn->toolTip(),
        StringResources::getAsQString(StringResources::kRenderSetupTooltipInactive));

    activateRenderLayerOnFirstSublayer();
    EXPECT_EQ(
        btn->toolTip(), StringResources::getAsQString(StringResources::kRenderSetupTooltipActive));

    deactivateRenderLayer();
    EXPECT_EQ(
        btn->toolTip(),
        StringResources::getAsQString(StringResources::kRenderSetupTooltipInactive));
}

// Grey while no render layer is active, blue while one is, each with its own hover icon.
TEST_F(LayerEditorWithRenderSetupFixture, RenderSetupButton_IconFollowsActiveRenderLayer)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, kButtonName);
    ASSERT_NE(btn, nullptr);
    QApplication::processEvents();
    EXPECT_TRUE(btn->styleSheet().contains("render_setup_default"));
    EXPECT_TRUE(btn->styleSheet().contains("render_setup_hover"));
    EXPECT_FALSE(btn->styleSheet().contains("render_setup_active"));

    activateRenderLayerOnFirstSublayer();
    EXPECT_TRUE(btn->styleSheet().contains("render_setup_active_default"));
    EXPECT_TRUE(btn->styleSheet().contains("render_setup_active_hover"));

    deactivateRenderLayer();
    EXPECT_TRUE(btn->styleSheet().contains("render_setup_default"));
    EXPECT_FALSE(btn->styleSheet().contains("render_setup_active"));
}

TEST_F(LayerEditorWithRenderSetupFixture, RenderSetupButton_ClickOpensRenderSetupInBothStates)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, kButtonName);
    ASSERT_NE(btn, nullptr);

    btn->click();
    EXPECT_EQ(_openRenderSetupCalls, 1);

    activateRenderLayerOnFirstSublayer();
    btn->click();
    EXPECT_EQ(_openRenderSetupCalls, 2);
}

// A render layer that is active on another stage does not turn this stage's button blue.
TEST_F(LayerEditorWithRenderSetupFixture, RenderSetupButton_GreyWhenRenderLayerIsOnAnotherStage)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, kButtonName);
    ASSERT_NE(btn, nullptr);

    _activeRenderLayerStage = PXR_NS::UsdStage::CreateInMemory();
    activateRenderLayerOnFirstSublayer();

    EXPECT_EQ(
        btn->toolTip(),
        StringResources::getAsQString(StringResources::kRenderSetupTooltipInactive));
    EXPECT_FALSE(btn->styleSheet().contains("render_setup_active"));
}

// ── Targeting ───────────────────────────────────────────────────────────────────

TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerTargeting_NothingBlockedWithoutRenderLayer)
{
    treeModel()->forceRefresh();
    QApplication::processEvents();

    for (const auto& index : { sessionLayerIndex(), rootLayerIndex(), firstSublayerIndex() }) {
        auto* item = treeModel()->layerItemFromIndex(index);
        ASSERT_NE(item, nullptr);
        EXPECT_FALSE(item->stageHasActiveRenderLayer());
        EXPECT_FALSE(item->isActiveRenderLayer());
    }
}

// The render layer owns the edit target: no layer can be targeted, the render layer included.
TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerTargeting_NoLayerCanBeTargeted)
{
    auto* renderLayerItem = activateRenderLayerOnFirstSublayer();
    ASSERT_NE(renderLayerItem, nullptr);
    EXPECT_TRUE(renderLayerItem->isActiveRenderLayer());

    auto* sessionItem = treeModel()->layerItemFromIndex(sessionLayerIndex());
    auto* rootItem = treeModel()->layerItemFromIndex(rootLayerIndex());
    for (auto* item : { renderLayerItem, sessionItem, rootItem }) {
        EXPECT_TRUE(item->stageHasActiveRenderLayer());

        _sessionState._commandHookImpl.clearCalls();
        treeModel()->setEditTarget(item);
        EXPECT_FALSE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
    }
}

TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerTargeting_AllLayersTargetableAgainOnceInactive)
{
    activateRenderLayerOnFirstSublayer();
    deactivateRenderLayer();

    auto* rootItem = treeModel()->layerItemFromIndex(rootLayerIndex());
    EXPECT_FALSE(rootItem->stageHasActiveRenderLayer());

    _sessionState._commandHookImpl.clearCalls();
    treeModel()->setEditTarget(rootItem);
    EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("setEditTarget"));
}

// Looking at another stage is like looking at a regular stage.
TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerTargeting_OtherStageIsNotAffected)
{
    _activeRenderLayerStage = PXR_NS::UsdStage::CreateInMemory();
    activateRenderLayerOnFirstSublayer();

    for (const auto& index : { sessionLayerIndex(), rootLayerIndex(), firstSublayerIndex() }) {
        auto* item = treeModel()->layerItemFromIndex(index);
        ASSERT_NE(item, nullptr);
        EXPECT_FALSE(item->stageHasActiveRenderLayer());
    }
}

// ── Window queries and context menus ────────────────────────────────────────────

TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerWindow_QueriesDescribeTheSelectedLayer)
{
    EXPECT_FALSE(_window->stageHasActiveRenderLayer());

    activateRenderLayerOnFirstSublayer();

    selectRow(firstSublayerIndex());
    EXPECT_TRUE(_window->layerIsActiveRenderLayer());
    EXPECT_TRUE(_window->stageHasActiveRenderLayer());

    selectRow(rootLayerIndex());
    EXPECT_FALSE(_window->layerIsActiveRenderLayer());
    EXPECT_TRUE(_window->stageHasActiveRenderLayer());
}

TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerContextMenu_RenderLayerHasNoLayerAdditions)
{
    activateRenderLayerOnFirstSublayer();
    selectRow(firstSublayerIndex());

    const auto entries = showContextMenu(*_window);

    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(findEntry(entries, GET_LE_QSTR(kMenuAddSublayer)), nullptr);
    EXPECT_EQ(findEntry(entries, GET_LE_QSTR(kMenuAddParentLayer)), nullptr);
    EXPECT_EQ(findEntry(entries, GET_LE_QSTR(kMenuLoadSublayers)), nullptr);
}

TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerContextMenu_OtherLayersKeepLayerAdditions)
{
    activateRenderLayerOnFirstSublayer();
    selectRow(rootLayerIndex());

    const auto entries = showContextMenu(*_window);

    ASSERT_FALSE(entries.empty());
    EXPECT_NE(findEntry(entries, GET_LE_QSTR(kMenuAddSublayer)), nullptr);
    EXPECT_NE(findEntry(entries, GET_LE_QSTR(kMenuLoadSublayers)), nullptr);
}

TEST_F(LayerEditorWithRenderSetupFixture, RenderLayerContextMenu_SessionLayerCannotBeCleared)
{
    selectRow(sessionLayerIndex());
    const auto  before = showContextMenu(*_window);
    const auto* clearBefore = findEntry(before, GET_LE_QSTR(kMenuClear));
    ASSERT_NE(clearBefore, nullptr);
    EXPECT_TRUE(clearBefore->enabled);

    activateRenderLayerOnFirstSublayer();
    selectRow(sessionLayerIndex());
    const auto  after = showContextMenu(*_window);
    const auto* clearAfter = findEntry(after, GET_LE_QSTR(kMenuClear));
    ASSERT_NE(clearAfter, nullptr);
    EXPECT_FALSE(clearAfter->enabled);
}

} // namespace UsdLayerEditor
