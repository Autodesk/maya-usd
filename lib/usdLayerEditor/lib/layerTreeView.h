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

#ifndef USDLAYEREDITOR_LAYERTREEVIEW_H
#define USDLAYEREDITOR_LAYERTREEVIEW_H

#include "layerEditorAPI.h"
#include "layerTreeViewStyle.h"
#include "sessionState.h"

#include <usdUfe/utils/uiCallback.h>

#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/notice.h>

#include <QtCore/QPointer>
#include <QtWidgets/QTreeView>

class QHelpEvent;

struct CallMethodParams;
namespace UsdLayerEditor {

class LayerTreeItem;
class LayerTreeItemDelegate;
class LayerTreeModel;
class LayerTreeView;
class SessionState;

typedef std::vector<LayerTreeItem*> LayerItemVector;
typedef void                        (LayerTreeItem::*simpleLayerMethod)(QWidget* in_parent);

/**
 * @brief State of the layer tree view and layer model.
 * Used to save and restore the state when the model is rebuilt.
 *
 */
class LayerEditorAPI LayerViewMemento
{
public:
    LayerViewMemento(const LayerTreeView&, const LayerTreeModel&);

    void preserve(const LayerTreeView&, const LayerTreeModel&);
    void restore(LayerTreeView&, LayerTreeModel&);

    bool empty() const { return _itemsState.empty(); }

    using ItemId = std::string;
    struct ItemState
    {
        bool _expanded = false;
        bool _selected = false;
        bool _current = false;
    };

    std::map<ItemId, ItemState> getItemsState() { return _itemsState; }
    void setItemsState(const std::map<ItemId, ItemState>& newState) { _itemsState = newState; }

private:
    std::map<ItemId, ItemState> _itemsState;
    int                         _horizontalScrollbarPosition { 0 };
    int                         _verticalScrollbarPosition { 0 };
};

/**
 * @brief Implements the Qt TreeView for USD layers. This widget is owned by the LayerEditorWidget.
 *
 */
class LayerTreeView
    : public QTreeView
    , public PXR_NS::TfWeakBase
{
    Q_OBJECT
public:
    typedef QTreeView PARENT_CLASS;
    LayerEditorAPI    LayerTreeView(SessionState* in_sessionState, QWidget* in_parent);
    LayerEditorAPI ~LayerTreeView() override;

    // get properly typed item
    LayerEditorAPI LayerTreeItem* layerItemFromIndex(const QModelIndex& index) const;

    // QTreeWidget-like method that returns the current item when one is selected
    LayerEditorAPI LayerTreeItem* currentLayerItem() const
    {
        auto index = currentIndex();
        return index.isValid() ? layerItemFromIndex(index) : nullptr;
    }

    // returns array of selected item, including the current item
    LayerEditorAPI LayerItemVector getSelectedLayerItems() const;

    // return property typed model
    LayerEditorAPI LayerTreeModel* layerTreeModel() const;

    // calls a given method on all items in the selection, with the given string as the undo chunk
    // name
    LayerEditorAPI void callMethodOnSelection(const QString& undoName, simpleLayerMethod method);

    // Override the layer tree styled item delegate.
    LayerEditorAPI void setCustomLayerItemDelegate(LayerTreeItemDelegate*);

    // menu callbacks
    LayerEditorAPI void onAddParentLayer(const QString& undoName) const;
    LayerEditorAPI void onMuteLayer(const QString& undoName) const;
    LayerEditorAPI void onLockLayer(const QString& undoName) const;
    LayerEditorAPI void
    onLockLayerAndSublayers(const QString& undoName, bool includeSublayers) const;

    // QWidgets overrides
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

protected:
    LayerEditorAPI void updateMouseCursor();

    // slot:
    LayerEditorAPI void onModelAboutToBeReset();
    LayerEditorAPI void onModelReset();
    LayerEditorAPI void onItemDoubleClicked(const QModelIndex& index);
    LayerEditorAPI void onExpanded(const QModelIndex& index);
    LayerEditorAPI void onCollapsed(const QModelIndex& index);
    LayerEditorAPI void onMuteLayerButtonPushed();
    LayerEditorAPI void onLockLayerButtonPushed();

    // Notice listener method for layer muting changes.
    LayerEditorAPI void onLayerMutingChanged(const pxr::UsdNotice::LayerMutingChanged& notice);

    LayerEditorAPI bool shouldExpandOrCollapseAll() const;
    LayerEditorAPI void expandChildren(const QModelIndex& index);
    LayerEditorAPI void collapseChildren(const QModelIndex& index);

    // delayed signal to select a layer on idle
    LayerEditorAPI void selectLayerRequest(const QModelIndex& index);

    // Updates the _cachedModelState using stage data from the session
    void updateFromSessionState();

    LayerTreeViewStyle       _treeViewStyle;
    QPointer<LayerTreeModel> _model;
    LayerTreeItemDelegate*   _delegate = nullptr;
    pxr::TfNotice::Key       _layerMutingNoticeKey;

    std::unique_ptr<LayerViewMemento> _cachedModelState;

    UsdUfe::UICallback::Ptr _refreshCallback;

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
