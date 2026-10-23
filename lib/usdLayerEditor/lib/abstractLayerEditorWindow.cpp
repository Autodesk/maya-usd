//
// Copyright 2024 Autodesk
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

#include "abstractLayerEditorWindow.h"

namespace UsdLayerEditor {

AbstractLayerEditorCreator* AbstractLayerEditorCreator::_instance = nullptr;

AbstractLayerEditorCreator* AbstractLayerEditorCreator::instance() { return _instance; }

AbstractLayerEditorCreator::AbstractLayerEditorCreator() { _instance = this; }

AbstractLayerEditorCreator::~AbstractLayerEditorCreator() { _instance = nullptr; }

AbstractLayerEditorWindow::AbstractLayerEditorWindow(const char* panelName)
{
    // this empty implementation is necessary for linking
}

AbstractLayerEditorWindow::~AbstractLayerEditorWindow()
{
    // this empty implementation is necessary for linking
}

} // namespace UsdLayerEditor
