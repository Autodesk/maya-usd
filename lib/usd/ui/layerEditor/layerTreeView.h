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

#ifndef LAYERTREEVIEW_H
#define LAYERTREEVIEW_H

#include "layerTreeViewStyle.h"
#include "sessionState.h"

#include <QtCore/QPointer>
#include <QtWidgets/QTreeView>

class QHelpEvent;

struct CallMethodParams;
namespace UsdLayerEditor {

class LayerTreeItem;
class LayerTreeItemDelegate;
class LayerTreeModel;
class SessionState;

typedef std::vector<LayerTreeItem*> LayerItemVector;
typedef void (LayerTreeItem::*simpleLayerMethod)();

/**
 * @brief Implements the Qt TreeView for USD layers. This widget is owned by the LayerEditorWidget.
 *
 */
class LayerTreeView : public QTreeView
{
    Q_OBJECT
public:
    typedef QTreeView PARENT_CLASS;
    LayerTreeView(SessionState* in_sessionState, QWidget* in_parent);

    // get properly typed item
    LayerTreeItem* layerItemFromIndex(const QModelIndex& index) const;

    // QTreeWidget-like method that returns the current item when one is selected
    LayerTreeItem* currentLayerItem() const
    {
        auto index = currentIndex();
        return index.isValid() ? layerItemFromIndex(index) : nullptr;
    }

    // returns array of selected item, including the current item
    LayerItemVector getSelectedLayerItems() const;

    // return property typed model
    LayerTreeModel* layerTreeModel() const;

    // calls a given method on all items in the selection, with the given string as the undo chunk
    // name
    void callMethodOnSelection(const QString& undoName, simpleLayerMethod method);

    // menu callbacks
    void onAddParentLayer(const QString& undoName) const;
    void onMuteLayer(const QString& undoName) const;

    // QWidgets overrides
    virtual void paintEvent(QPaintEvent* event) override;
    virtual bool event(QEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;

protected:
    // slot:
    void onModelReset();
    void onItemDoubleClicked(const QModelIndex& index);
    void onMuteLayerButtonPushed();

    // delayed signal to select a layer on idle
    void selectLayerRquest(const QModelIndex& index);

    LayerTreeViewStyle       _treeViewStyle;
    QPointer<LayerTreeModel> _model;
    LayerTreeItemDelegate*   _delegate;

    void handleTooltips(QHelpEvent* event);

    // the mute button area has a different implementation than
    // the target button. It is based on Maya's renderSetup
    struct ActionButtons
    {
        std::vector<QAction*> _staticActions;
        QAction*              _mouseReleaseAction = nullptr;
        bool                  _actionButtonPressed = false;
    } _actionButtons;
    // returns the action button the mouse was last over
    QAction*
    getCurrentAction(QFlags<Qt::KeyboardModifier> modifiers, QPoint pos, QModelIndex index) const;
};

} // namespace UsdLayerEditor

#endif // LAYERTREEVIEW_H
