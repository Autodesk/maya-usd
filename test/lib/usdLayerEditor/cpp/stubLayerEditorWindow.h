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

#include "layerEditorWidget.h"
#include "layerEditorWindow.h"
#include "stubSessionState.h"

#include <QtWidgets/QMainWindow>

namespace UsdLayerEditor {

class StubLayerEditorWindow : public LayerEditorWindow
{
public:
    StubLayerEditorWindow(StubSessionState& sessionState, QMainWindow* parent)
        : LayerEditorWindow("stub_panel")
        , _sessionState(sessionState)
        , _mainWindow(parent)
    {
        _layerEditor = new LayerEditorWidget(sessionState, parent);
    }

    LayerEditorWidget* widget() const { return _layerEditor; }

    // AbstractLayerEditorWindow pure virtuals
    std::string    dccObjectName() const override { return "stub_panel"; }
    void           selectDccObject(const char*) override { }
    SessionState*  getSessionState() override { return &_sessionState; }
    QMainWindow*   getMainWindow() override { return _mainWindow; }

private:
    StubSessionState& _sessionState;
    QMainWindow*      _mainWindow;
};

} // namespace UsdLayerEditor
