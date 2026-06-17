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
#include "componentSaveWidget.h"

#include <QtCore/QString>

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace UsdLayerEditor {

class ComponentSaveWidgetTest : public LayerEditorTestFixture
{
protected:
    std::unique_ptr<ComponentSaveWidget> makeWidget(const std::string& dccPath = "proxy|shape")
    {
        return std::make_unique<ComponentSaveWidget>(_mainWindow, &_sessionState, dccPath);
    }
};

// ── construction ──────────────────────────────────────────────────────────────

TEST_F(ComponentSaveWidgetTest, Construction_DoesNotCrash)
{
    // Exercises ComponentSaveWidget() + setupUI() (~120 lines of uncovered code).
    EXPECT_NO_THROW(makeWidget());
}

// ── dccObjectPath ─────────────────────────────────────────────────────────────

TEST_F(ComponentSaveWidgetTest, DccObjectPath_ReturnsConstructorValue)
{
    auto w = makeWidget("my|proxy|shape");
    EXPECT_EQ(w->dccObjectPath(), "my|proxy|shape");
}

TEST_F(ComponentSaveWidgetTest, DccObjectPath_EmptyStringRoundTrips)
{
    auto w = makeWidget("");
    EXPECT_EQ(w->dccObjectPath(), "");
}

// ── componentName ─────────────────────────────────────────────────────────────

TEST_F(ComponentSaveWidgetTest, SetComponentName_RoundTrips)
{
    auto w = makeWidget();
    w->setComponentName("MyComponent");
    EXPECT_EQ(w->componentName(), "MyComponent");
}

TEST_F(ComponentSaveWidgetTest, SetComponentName_EmptyString)
{
    auto w = makeWidget();
    w->setComponentName("");
    EXPECT_EQ(w->componentName(), "");
}

TEST_F(ComponentSaveWidgetTest, SetComponentName_OverwritesPreviousValue)
{
    auto w = makeWidget();
    w->setComponentName("First");
    w->setComponentName("Second");
    EXPECT_EQ(w->componentName(), "Second");
}

// ── folderLocation ────────────────────────────────────────────────────────────

TEST_F(ComponentSaveWidgetTest, SetFolderLocation_RoundTrips)
{
    auto w = makeWidget();
    w->setFolderLocation("/tmp/my_folder");
    EXPECT_EQ(w->folderLocation(), "/tmp/my_folder");
}

TEST_F(ComponentSaveWidgetTest, SetFolderLocation_EmptyString)
{
    auto w = makeWidget();
    w->setFolderLocation("");
    EXPECT_EQ(w->folderLocation(), "");
}

// ── compact mode ──────────────────────────────────────────────────────────────

TEST_F(ComponentSaveWidgetTest, IsCompactMode_FalseByDefault)
{
    auto w = makeWidget();
    EXPECT_FALSE(w->isCompactMode());
}

TEST_F(ComponentSaveWidgetTest, SetCompactMode_True)
{
    auto w = makeWidget();
    w->setCompactMode(true);
    EXPECT_TRUE(w->isCompactMode());
}

TEST_F(ComponentSaveWidgetTest, SetCompactMode_RoundTrip)
{
    auto w = makeWidget();
    w->setCompactMode(true);
    w->setCompactMode(false);
    EXPECT_FALSE(w->isCompactMode());
}

// ── isExpanded / originalHeight (defaults) ───────────────────────────────────

TEST_F(ComponentSaveWidgetTest, IsExpanded_FalseByDefault)
{
    auto w = makeWidget();
    EXPECT_FALSE(w->isExpanded());
}

TEST_F(ComponentSaveWidgetTest, SetOriginalHeight_RoundTrips)
{
    auto w = makeWidget();
    w->setOriginalHeight(200);
    EXPECT_EQ(w->originalHeight(), 200);
}

} // namespace UsdLayerEditor
