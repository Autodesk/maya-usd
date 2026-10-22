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

#ifndef USDLAYEREDITOR_LAYERCONTEXTMENU_H
#define USDLAYEREDITOR_LAYERCONTEXTMENU_H

#include "abstractLayerEditorWindow.h"
#include "layerEditorAPI.h"

#include <functional>

class QMenu;

namespace UsdLayerEditor {

class LayerTreeView;

/**
 * @brief Fills the layer editor context menu for the current selection.
 *
 * @param addDCCItems appends DCC-specific items after "Select Prims With Spec".
 */
LAYEREDITOR_UI_PUBLIC void populateLayerContextMenu(
    QMenu&                             menu,
    AbstractLayerEditorWindow&         window,
    LayerTreeView&                     treeView,
    const std::function<void(QMenu&)>& addDCCItems = {});

} // namespace UsdLayerEditor

#endif // USDLAYEREDITOR_LAYERCONTEXTMENU_H
