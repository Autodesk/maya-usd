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
#include "layerContentsWidget.h"
#include "layerTreeItem.h"

#include <pxr/usd/sdf/layer.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QSplitter>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

// Locate the LayerContentsWidget inside the LayerEditorWidget.
static LayerContentsWidget* findContentsWidget(QWidget* root)
{
    return root->findChild<LayerContentsWidget*>(
        QString(), Qt::FindChildrenRecursively);
}

class LayerContentsWidgetTest : public LayerEditorTestFixture {};

TEST_F(LayerContentsWidgetTest, ContentsWidget_ExistsInLayout)
{
    auto* cw = findContentsWidget(_widget);
    EXPECT_NE(cw, nullptr);
}

TEST_F(LayerContentsWidgetTest, IsEmpty_TrueByDefault)
{
    auto* cw = findContentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    EXPECT_TRUE(cw->isEmpty());
}

TEST_F(LayerContentsWidgetTest, SetLayer_SetsIsEmptyFalseForLayerWithContent)
{
    auto* cw = findContentsWidget(_widget);
    ASSERT_NE(cw, nullptr);

    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(item, nullptr);
    item->layer()->SetComment("test content");

    cw->setLayer(item->layer());
    QApplication::processEvents();
    EXPECT_FALSE(cw->isEmpty());
}

TEST_F(LayerContentsWidgetTest, Clear_SetsIsEmptyTrue)
{
    auto* cw = findContentsWidget(_widget);
    ASSERT_NE(cw, nullptr);

    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(item, nullptr);
    cw->setLayer(item->layer());
    QApplication::processEvents();

    cw->clear();
    EXPECT_TRUE(cw->isEmpty());
}

TEST_F(LayerContentsWidgetTest, SetLayer_WithNullLayer_IsEmpty)
{
    auto* cw = findContentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    cw->setLayer(nullptr);
    QApplication::processEvents();
    EXPECT_TRUE(cw->isEmpty());
}

TEST_F(LayerContentsWidgetTest, SetLayer_DoesNotCrash)
{
    // displayLayerContents option controls widget visibility.
    auto* cw = findContentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    auto* item = dynamic_cast<LayerTreeItem*>(
        treeModel()->itemFromIndex(rootLayerIndex()));
    ASSERT_NE(item, nullptr);
    EXPECT_NO_THROW(cw->setLayer(item->layer()));
}

} // namespace UsdLayerEditor
