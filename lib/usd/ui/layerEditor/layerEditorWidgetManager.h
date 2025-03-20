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
#include <QtCore/QPointer>

namespace UsdLayerEditor {
class LayerEditorWidget;

/**
 * @brief Lightweight LayerEditorWidget manager class, created to have an minimal interface between
 *dll boundaries (in particular, for the _UsdLayerEditor project's code that creates python bindings
 *for accessing data in the widget)
 **/
class LayerEditorWidgetManager
{
public:
    ~LayerEditorWidgetManager() = default;

    static LayerEditorWidgetManager* getInstance();
    void                             setWidget(LayerEditorWidget* widget);

    std::vector<std::string> getSelectedLayers();
    void                     selectLayers(std::vector<std::string> layerIds);

private:
    LayerEditorWidgetManager();

    QPointer<LayerEditorWidget>                      _layerWidgetInstance;
    static std::unique_ptr<LayerEditorWidgetManager> instance;
};

} // namespace UsdLayerEditor