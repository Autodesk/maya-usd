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

#ifndef MAYALAYEREDITORWINDOW_H
#define MAYALAYEREDITORWINDOW_H

#include "layerTreeView.h"
#include "mayaSessionState.h"

#include <mayaUsd/base/api.h>
#include <mayaUsd/commands/abstractLayerEditorWindow.h>

#include <QtCore/QPointer>
#include <QtWidgets/QMainWindow>

using namespace MAYAUSD_NS_DEF;
namespace UsdLayerEditor {

class LayerEditorWidget;
class LayerTreeView;

/**
 * @brief implements the maya panel that contains the USD layer editor
 *
 */
class MayaLayerEditorWindow
    : public QMainWindow
    , public MayaUsd::AbstractLayerEditorWindow
{
    Q_OBJECT
public:
    typedef QMainWindow PARENT_CLASS;

    MayaLayerEditorWindow(const char* panelName, QWidget* parent = nullptr);
    ~MayaLayerEditorWindow();

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
    std::string proxyShapeName() const override;

    void removeSubLayer() override;
    void saveEdits() override;
    void discardEdits() override;
    void addAnonymousSublayer() override;
    void addParentLayer() override;
    void loadSubLayers() override;
    void muteLayer() override;
    void printLayer() override;
    void clearLayer() override;
    void selectPrimsWithSpec() override;

    void selectProxyShape(const char* shapePath) override;

public:
    void onClearUIOnSceneReset();
    void onCreateUI();

    // slots:
    void onShowContextMenu(const QPoint& pos);

protected:
    MayaSessionState            _sessionState;
    QPointer<LayerEditorWidget> _layerEditor;
    std::string                 _panelName;

    LayerTreeView* treeView();
};

} // namespace UsdLayerEditor

#endif // MAYALAYEREDITORWINDOW_H
