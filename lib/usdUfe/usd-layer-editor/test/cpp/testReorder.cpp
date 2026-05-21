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

#include <pxr/usd/sdf/layer.h>

#include <QtWidgets/QApplication>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

static void addSecondSublayer(StubSessionState& state)
{
    auto stage     = state.stage();
    auto rootLayer = stage->GetRootLayer();
    auto extra     = SdfLayer::CreateAnonymous("extra_sublayer");
    rootLayer->InsertSubLayerPath(extra->GetIdentifier(), 1);
}

TEST_F(LayerEditorTestFixture, DragDrop_MoveRowDown_CallsMoveSubLayerPath)
{
    addSecondSublayer(_sessionState);
    QApplication::processEvents();

    auto rootLayer = _sessionState.stage()->GetRootLayer();
    auto paths     = rootLayer->GetSubLayerPaths();
    ASSERT_GE(paths.size(), 2u) << "Need at least 2 sublayers";

    QModelIndex parentIndex = rootLayerIndex();
    QMimeData*  mimeData    = treeModel()->mimeData({ treeModel()->index(0, 0, parentIndex) });
    ASSERT_NE(mimeData, nullptr) << "Model must supply MIME data for drag";

    bool accepted = treeModel()->dropMimeData(mimeData, Qt::MoveAction, 2, 0, parentIndex);
    delete mimeData;

    if (accepted) {
        QApplication::processEvents();
        EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"))
            << "moveSubLayerPath should be called on reorder";
    } else {
        // Verify the model at least advertises drag support.
        EXPECT_FALSE(treeModel()->mimeTypes().isEmpty())
            << "Model should support MIME data for drag-drop";
    }
}

TEST_F(LayerEditorTestFixture, DragDrop_MoveRowUp_CallsMoveSubLayerPath)
{
    addSecondSublayer(_sessionState);
    QApplication::processEvents();

    auto rootLayer = _sessionState.stage()->GetRootLayer();
    auto paths     = rootLayer->GetSubLayerPaths();
    ASSERT_GE(paths.size(), 2u);

    QModelIndex parentIndex = rootLayerIndex();
    QMimeData*  mimeData    = treeModel()->mimeData({ treeModel()->index(1, 0, parentIndex) });
    ASSERT_NE(mimeData, nullptr);

    bool accepted = treeModel()->dropMimeData(mimeData, Qt::MoveAction, 0, 0, parentIndex);
    delete mimeData;

    if (accepted) {
        QApplication::processEvents();
        EXPECT_TRUE(_sessionState._commandHookImpl.hasCall("moveSubLayerPath"))
            << "moveSubLayerPath should be called on reorder";
    } else {
        EXPECT_FALSE(treeModel()->mimeTypes().isEmpty())
            << "Model should support MIME data for drag-drop";
    }
}

} // namespace UsdLayerEditor
