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

#include "stubCommandHook.h"
#include "stubLayerEditorWindow.h"
#include "stubSessionState.h"

#include "layerEditorWidget.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"

#include <gtest/gtest.h>

#include <QtCore/QModelIndex>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <memory>

namespace UsdLayerEditor {

class LayerEditorTestFixture : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    LayerTreeView*  layerTree();
    LayerTreeModel* treeModel();
    QModelIndex     sessionLayerIndex();
    QModelIndex     rootLayerIndex();
    QModelIndex     firstSublayerIndex();

    // Select a single tree row so LayerEditorWindow state queries are valid.
    void selectRow(const QModelIndex& index);

    StubSessionState                        _sessionState;
    std::unique_ptr<StubLayerEditorWindow>  _window;
    QMainWindow*                            _mainWindow { nullptr };

    // Convenience: the widget owned by _window.
    LayerEditorWidget* _widget { nullptr };
};

// Find a named action in a menu (searches recursively into submenus).
QAction* findAction(QMenu* menu, const QString& text);

} // namespace UsdLayerEditor
