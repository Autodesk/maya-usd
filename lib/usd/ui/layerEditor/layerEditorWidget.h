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

#include <QtCore/QPointer>
#include <QtWidgets/QWidget>

class QMainWindow;
class QLayout;
class QPushButton;

namespace UsdLayerEditor {
class DirtyLayersCountBadge;
class LayerTreeView;
class SessionState;

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

public:
    LayerTreeView* layerTree() { return _treeView.data(); }

protected:
    void          setupLayout();
    QLayout*      setupLayout_toolbar();
    SessionState& _sessionState;
    QWidget*      _saveButtonParent { nullptr };
    struct
    {
        QPushButton*           _newLayer;
        QPushButton*           _loadLayer;
        QPushButton*           _saveStageButton;
        DirtyLayersCountBadge* _dirtyCountBadge;
    } _buttons;
    void updateNewLayerButton();
    void updateButtons();

    QPointer<LayerTreeView> _treeView;

    bool _updateButtonsOnIdle = false; // true if request to update on idle is pending
};

} // namespace UsdLayerEditor

#endif // LAYEREDITORWIDGET_H
