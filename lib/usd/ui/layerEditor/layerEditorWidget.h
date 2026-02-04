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

#ifndef LAYEREDITORWIDGET_H
#define LAYEREDITORWIDGET_H

#include "layerTreeItem.h"
#include "layerTreeView.h"

#include <QtCore/QBasicTimer>
#include <QtCore/QPointer>
#include <QtWidgets/QWidget>

class QMainWindow;
class QLayout;
class QPushButton;

namespace UsdLayerEditor {
class DirtyLayersCountBadge;
class LayerTreeView;
class SessionState;
class LayerContentsWidget;

/**
 * @brief Widget that manages a menu, a combo box to select a USD stage, and  USD Layer Tree view
 *
 * This widget is meant to be hosted by a parent QMainWindow, where the menu will be created
 **/

class LayerEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LayerEditorWidget(SessionState& in_sessionState, QMainWindow* in_parent = nullptr);

Q_SIGNALS:

public Q_SLOTS:
    void onNewLayerButtonClicked();
    void onLoadLayersButtonClicked();
    void onSaveStageButtonClicked();
    void updateButtonsOnIdle();
    void showDisplayLayerContents(bool show);
    void onSplitterMoved(int pos, int index);
    void onLazyUpdateLayerContents();

public:
    LayerTreeView*           layerTree() { return _treeView.data(); }
    std::vector<std::string> getSelectedLayers();
    void                     selectLayers(const std::vector<std::string>& layerIdentifiers);

protected:
    void          setupLayout();
    QLayout*      setupLayout_toolbar();
    SessionState& _sessionState;
    struct
    {
        QPushButton*           _newLayer { nullptr };
        QPushButton*           _loadLayer { nullptr };
        QPushButton*           _saveStageButton { nullptr };
        DirtyLayersCountBadge* _dirtyCountBadge { nullptr };
    } _buttons;

    void setupDefaultMenu(QMainWindow* in_parent);
    struct
    {
        QAction* _autoHide { nullptr };
        QAction* _displayLayerContents { nullptr };
    } _actions;
    void updateNewLayerButton();
    void updateButtons();

    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    void timerEvent(QTimerEvent* event) override;
    void updateLayerContentsWidget();

private:
    QPointer<LayerTreeView>       _treeView;
    QPointer<LayerContentsWidget> _layerContents;
    QBasicTimer                   _layerContentsTimer;

    bool _updateButtonsOnIdle = false; // true if request to update on idle is pending
};

} // namespace UsdLayerEditor

#endif // LAYEREDITORWIDGET_H
