#pragma once

//
// Copyright 2020 Autodesk
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

#include "abstractLayerEditorWindow.h"
#include "layerTreeView.h"

#include <QtCore/QPointer>

namespace UsdLayerEditor {

class LayerEditorWidget;
class LayerTreeView;

class LayerEditorAPI LayerEditorWindow : public AbstractLayerEditorWindow
{
public:

    LayerEditorWindow(const char* panelName);
    ~LayerEditorWindow();

    // tree commands
    int         selectionLength() override;
    bool        isInvalidLayer() override;
    bool        isSessionLayer() override;
    bool        isLayerDirty() override;
    bool        isSubLayer() override;
    bool        isAnonymousLayer() override;
    bool        isIncomingLayer() override;
    bool        layerNeedsSaving() override;
    bool        layerAppearsMuted() override;
    bool        layerIsMuted() override;
    bool        layerIsReadOnly() override;
    bool        layerAppearsLocked() override;
    bool        layerIsLocked() override;
    bool        layerAppearsSystemLocked() override;
    bool        layerIsSystemLocked() override;
    bool        layerHasSubLayers() override;

    void removeSubLayer() override;
    void saveEdits() override;
    void discardEdits() override;
    void addAnonymousSublayer() override;
    void addParentLayer() override;
    void loadSubLayers() override;
    void muteLayer() override;
    void printLayer() override;
    void clearLayer() override;
    void mergeWithSublayers() override;
    void selectPrimsWithSpec() override;
    void buildContextMenu(const QPoint& pos);
    void updateLayerModel() override;
    void lockLayer() override;
    void lockLayerAndSubLayers() override;
    void stitchLayers() override;


    virtual QMainWindow* getMainWindow() = 0;

protected:

    QPointer<LayerEditorWidget> _layerEditor;
    std::string                 _panelName;

    LayerTreeView* treeView();
};

} // namespace UsdLayerEditor
