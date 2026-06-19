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
#include "layerContentsWidget.h"
#include "layerTreeItem.h"

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTextEdit>

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

// The array size limit (from the DCC registry) is applied when rendering layer
// contents: a small limit truncates the displayed array.
#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerContentsWidgetTest, SetLayer_RespectsArraySizeLimit)
{
    auto* cw = findContentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    auto* textEdit = cw->findChild<QTextEdit*>(QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(textEdit, nullptr);

    // Build a layer with a large array-valued attribute.
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto prim  = stage->DefinePrim(PXR_NS::SdfPath("/Test"));
    auto attr  = prim.CreateAttribute(
        PXR_NS::TfToken("arr"), PXR_NS::SdfValueTypeNames->IntArray);
    PXR_NS::VtIntArray values(100);
    for (int i = 0; i < 100; ++i)
        values[i] = i;
    attr.Set(values);
    auto layer = stage->GetRootLayer();

    ScopedLayerEditorDCCFunctions guard;

    // Override only the array-size getter, preserving any other environment functions.
    EnvironmentFns smallEnv = layerEditorDCCFunctions().environment;
    smallEnv.layerContentsArraySizeLimit = []() -> int64_t { return 2; };
    setEnvironmentFns(smallEnv);
    cw->setLayer(layer);
    QApplication::processEvents();
    const int smallLen = textEdit->toPlainText().length();

    EnvironmentFns largeEnv = layerEditorDCCFunctions().environment;
    largeEnv.layerContentsArraySizeLimit = []() -> int64_t { return 1000; };
    setEnvironmentFns(largeEnv);
    cw->setLayer(layer);
    QApplication::processEvents();
    const int largeLen = textEdit->toPlainText().length();

    EXPECT_LT(smallLen, largeLen)
        << "a smaller array size limit should truncate the displayed array, "
           "yielding shorter output";
}
#endif

} // namespace UsdLayerEditor
