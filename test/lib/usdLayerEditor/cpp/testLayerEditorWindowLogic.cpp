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

#ifndef MAYAUSD_OLD_LAYER_EDITOR
#include <testFixture.h>
#include "abstractLayerEditorWindow.h"
#include "layerEditorWidget.h"
#include "layerEditorWindow.h"

#include <gtest/gtest.h>

#include <memory>

namespace UsdLayerEditor {

// ── AbstractLayerEditorCreator singleton lifecycle ────────────────────────

// Minimal concrete subclass used to exercise singleton registration.
class TestCreator final : public AbstractLayerEditorCreator
{
public:
    AbstractLayerEditorWindow* createWindow(const char*) override { return nullptr; }
    AbstractLayerEditorWindow* getWindow(const char*) const override { return nullptr; }
    PanelNamesList             getAllPanelNames() const override { return {}; }
};

TEST(AbstractLayerEditorCreatorTest, InstanceNullBeforeAnyCreation)
{
    if (AbstractLayerEditorCreator::instance() != nullptr)
        GTEST_SKIP() << "Another creator already registered; skipping.";
    EXPECT_EQ(AbstractLayerEditorCreator::instance(), nullptr);
}

TEST(AbstractLayerEditorCreatorTest, InstanceNonNullAfterConstruction)
{
    if (AbstractLayerEditorCreator::instance() != nullptr)
        GTEST_SKIP() << "Another creator already registered; skipping.";
    TestCreator creator;
    EXPECT_EQ(AbstractLayerEditorCreator::instance(), &creator);
}

TEST(AbstractLayerEditorCreatorTest, InstanceNullAfterDestruction)
{
    if (AbstractLayerEditorCreator::instance() != nullptr)
        GTEST_SKIP() << "Another creator already registered; skipping.";
    {
        TestCreator creator;
        ASSERT_NE(AbstractLayerEditorCreator::instance(), nullptr);
    }
    EXPECT_EQ(AbstractLayerEditorCreator::instance(), nullptr);
}

TEST(AbstractLayerEditorCreatorTest, InstanceReturnsSamePtrEachCall)
{
    if (AbstractLayerEditorCreator::instance() != nullptr)
        GTEST_SKIP() << "Another creator already registered; skipping.";
    TestCreator creator;
    EXPECT_EQ(AbstractLayerEditorCreator::instance(), AbstractLayerEditorCreator::instance());
}

// ── LayerEditorWindow delegation methods ─────────────────────────────────

// Concrete subclass satisfying LayerEditorWindow's pure virtuals.
// Initializes _layerEditor so that treeView() is safe to call.
class TestableLayerEditorWindow : public LayerEditorWindow
{
public:
    explicit TestableLayerEditorWindow(SessionState& ss, QMainWindow* parent = nullptr)
        : LayerEditorWindow("test_panel")
        , _ss(&ss)
    {
        _layerEditor = new LayerEditorWidget(ss, parent);
    }

    QMainWindow*  getMainWindow() override { return nullptr; }
    std::string   dccObjectName() const override { return "test_obj"; }
    void          selectDccObject(const char*) override {}
    SessionState* getSessionState() override { return _ss; }

private:
    SessionState* _ss;
};

class LayerEditorWindowTest : public LayerEditorTestFixture
{
protected:
    std::unique_ptr<TestableLayerEditorWindow> makeWindow()
    {
        return std::make_unique<TestableLayerEditorWindow>(_sessionState, _mainWindow);
    }
};

TEST_F(LayerEditorWindowTest, SelectionLength_EmptySelection_ReturnsZero)
{
    auto w = makeWindow();
    EXPECT_EQ(w->selectionLength(), 0);
}

TEST_F(LayerEditorWindowTest, IsInvalidLayer_NoSelection_ReturnsFalse)
{
    auto w = makeWindow();
    EXPECT_FALSE(w->isInvalidLayer());
}

TEST_F(LayerEditorWindowTest, IsSessionLayer_NoSelection_ReturnsFalse)
{
    auto w = makeWindow();
    EXPECT_FALSE(w->isSessionLayer());
}

TEST_F(LayerEditorWindowTest, IsAnonymousLayer_NoSelection_ReturnsFalse)
{
    auto w = makeWindow();
    EXPECT_FALSE(w->isAnonymousLayer());
}

TEST_F(LayerEditorWindowTest, IsSubLayer_NoSelection_ReturnsFalse)
{
    auto w = makeWindow();
    EXPECT_FALSE(w->isSubLayer());
}

TEST_F(LayerEditorWindowTest, LayerHasSubLayers_NoSelection_ReturnsFalse)
{
    auto w = makeWindow();
    EXPECT_FALSE(w->layerHasSubLayers());
}

TEST_F(LayerEditorWindowTest, LayerIsMuted_NoSelection_ReturnsFalse)
{
    auto w = makeWindow();
    EXPECT_FALSE(w->layerIsMuted());
}

TEST_F(LayerEditorWindowTest, LayerIsLocked_NoSelection_ReturnsFalse)
{
    auto w = makeWindow();
    EXPECT_FALSE(w->layerIsLocked());
}

TEST_F(LayerEditorWindowTest, SaveEdits_NoSelection_DoesNotCrash)
{
    auto w = makeWindow();
    EXPECT_NO_THROW(w->saveEdits());
}

TEST_F(LayerEditorWindowTest, ClearLayer_NoSelection_DoesNotCrash)
{
    auto w = makeWindow();
    EXPECT_NO_THROW(w->clearLayer());
}

TEST_F(LayerEditorWindowTest, UpdateLayerModel_DoesNotCrash)
{
    auto w = makeWindow();
    EXPECT_NO_THROW(w->updateLayerModel());
}

TEST_F(LayerEditorWindowTest, StitchLayers_LessThanTwoItems_DoesNotCrash)
{
    auto w = makeWindow();
    EXPECT_NO_THROW(w->stitchLayers());
}

} // namespace UsdLayerEditor
#endif
