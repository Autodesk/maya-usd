//
// Copyright 2025 Autodesk
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

#include "layerEditorWidgetManager.h"

#include "layerEditorWidget.h"

namespace UsdLayerEditor {

std::unique_ptr<LayerEditorWidgetManager> LayerEditorWidgetManager::instance;

LayerEditorWidgetManager::LayerEditorWidgetManager()
    : _layerWidgetInstance(nullptr)
{
}

LayerEditorWidgetManager* LayerEditorWidgetManager::getInstance()
{
    if (!instance) {
        instance = std::unique_ptr<LayerEditorWidgetManager>(new LayerEditorWidgetManager);
    }
    return instance.get();
}

void LayerEditorWidgetManager::setWidget(LayerEditorWidget* widget)
{
    if (_layerWidgetInstance != nullptr) {
        TF_WARN("LayerEditorWidgetManager already has a LayerEditorWidget set. Overriding "
                "previously set widget.");
    }
    _layerWidgetInstance = widget;
}

std::vector<std::string> LayerEditorWidgetManager::getSelectedLayers()
{
    std::vector<std::string> selLayers;
    if (_layerWidgetInstance) {
        selLayers = _layerWidgetInstance->getSelectedLayers();
    } else {
        TF_CODING_ERROR(
            "No LayerEditorWidget set in the LayerEditorWidgetManager. No layers to retrieve.");
    }

    return selLayers;
}

void LayerEditorWidgetManager::selectLayers(std::vector<std::string> layerIds)
{
    if (_layerWidgetInstance) {
        _layerWidgetInstance->selectLayers(layerIds);
    } else {
        TF_CODING_ERROR(
            "No LayerEditorWidget set in the LayerEditorWidgetManager. Layers cannot be selected.");
    }
}

} // namespace UsdLayerEditor