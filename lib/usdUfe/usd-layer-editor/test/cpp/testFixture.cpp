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

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

namespace UsdLayerEditor {

void LayerEditorTestFixture::SetUp()
{
    auto* win = new QMainWindow();
    _widget   = std::make_unique<LayerEditorWidget>(_sessionState, win);
    _widget->show();
    QApplication::processEvents();
    _sessionState._commandHookImpl.clearCalls();
    _sessionState._saveLayerCallCount  = 0;
    _sessionState._printLayerCallCount = 0;
    _sessionState._loadLayersCallCount = 0;
}

void LayerEditorTestFixture::TearDown()
{
    _widget.reset();
}

LayerTreeView* LayerEditorTestFixture::layerTree()
{
    return _widget->layerTree();
}

LayerTreeModel* LayerEditorTestFixture::treeModel()
{
    return layerTree()->layerTreeModel();
}

QModelIndex LayerEditorTestFixture::sessionLayerIndex()
{
    return treeModel()->rootLayerIndex();
}

QModelIndex LayerEditorTestFixture::firstSublayerIndex()
{
    return treeModel()->index(0, 0, sessionLayerIndex());
}

QAction* findAction(QMenu* menu, const QString& text)
{
    if (!menu)
        return nullptr;
    for (QAction* action : menu->actions()) {
        if (action->text() == text)
            return action;
        if (action->menu()) {
            QAction* found = findAction(action->menu(), text);
            if (found)
                return found;
        }
    }
    return nullptr;
}

} // namespace UsdLayerEditor
