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

#include "layerTreeView.h"

#include "abstractCommandHook.h"
#include "layerTreeItem.h"
#include "layerTreeItemDelegate.h"
#include "layerTreeModel.h"
#include "stringResources.h"

#include <QtGui/QColor>
#include <QtWidgets/QMenu>

using namespace UsdLayerEditor;

struct CallMethodParams
{
    const LayerItemVector* selection;
    QString                name;
    AbstractCommandHook*   commandHook;
};

namespace {
QColor BLACK_BACKGROUND(43, 43, 43);

typedef void (LayerTreeItem::*simpleLayerMethod)();

void doCallMethodOnSelection(const CallMethodParams& params, simpleLayerMethod method)
{
    if (params.selection->size() > 0) {
        UndoContext context(params.commandHook, params.name);

        for (auto item : *params.selection) {
            (item->*method)();
        }
    }
}

} // namespace

namespace UsdLayerEditor {

LayerTreeView::LayerTreeView(SessionState* in_sessionState, QWidget* in_parent)
    : QTreeView(in_parent)
{
    _model = new LayerTreeModel(in_sessionState, this);
    setModel(_model);
    connect(_model, &LayerTreeModel::selectLayerSignal, this, &LayerTreeView::selectLayerRquest);

    // clang-format off
    QString styleSheet = 
    "QTreeView { "
                "background: "+ BLACK_BACKGROUND.name() + ";"
                "show-decoration-selected: 0;"
                "}";
    // clang-format on
    setStyleSheet(styleSheet);
    setStyle(&_treeViewStyle);
    setHeaderHidden(true);
    setUniformRowHeights(true);
    setIndentation(16);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setMouseTracking(true);
    setExpandsOnDoubleClick(false);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);

    // custom row drawing
    _delegate = new LayerTreeItemDelegate(this);
    setItemDelegate(_delegate);
    connect(
        _model, &QAbstractItemModel::modelReset, _delegate, &LayerTreeItemDelegate::onModelReset);

    // context menu
    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

    // updates
    connect(_model, &QAbstractItemModel::modelReset, this, &LayerTreeView::onModelReset);

    // signals
    connect(this, &QAbstractItemView::doubleClicked, this, &LayerTreeView::onItemDoubleClicked);

    // renderSetuplike API
    auto actionButtons = LayerTreeItem::actionButtonsDefinition();
    for (auto actionInfo : actionButtons) {
        auto action = new QAction(actionInfo._name, this);
        connect(action, &QAction::triggered, this, &LayerTreeView::onMuteLayerButtonPushed);
        _actionButtons._staticActions.push_back(action);
    }
}

LayerTreeItem* LayerTreeView::layerItemFromIndex(const QModelIndex& index) const
{
    return _model->layerItemFromIndex(index);
}

LayerTreeModel* LayerTreeView::layerTreeModel() const { return _model.data(); }

void LayerTreeView::selectLayerRquest(const QModelIndex& index)
{
    // slot called when the user manually adds a sublayer with the UI
    // we use this to select the new layer
    setCurrentIndex(index);
    scrollTo(index);
}

void LayerTreeView::onItemDoubleClicked(const QModelIndex& index)
{
    if (index.isValid()) {
        auto layerTreeItem = layerItemFromIndex(index);
        if (layerTreeItem->isAnonymous()) {
            layerTreeItem->saveEdits();
        }
    }
}

void LayerTreeView::onModelReset() { expandAll(); }

LayerItemVector LayerTreeView::getSelectedLayerItems() const
{
    auto selection = selectionModel()->selectedRows();

    LayerItemVector result;
    result.reserve(selection.size());
    for (const auto& index : selection) {
        result.push_back(layerItemFromIndex(index));
    }

    // with the context menu,
    // you need to hold down ctrl/cmd and click a non selected item to get in this code
    // you then have a current item that is not in the selection
    auto clickedIndex = currentIndex();
    if (clickedIndex.isValid() && selection.indexOf(clickedIndex) == -1) {
        result.push_back(currentLayerItem());
    }
    return result;
}

void LayerTreeView::onAddParentLayer(const QString& undoName) const
{
    auto selection = getSelectedLayerItems();

    CallMethodParams params;
    params.selection = &selection;
    params.commandHook = _model->sessionState()->commandHook();
    params.name = undoName;

    // we add one new parent to each item in the selection
    // for undo, it's ok to directly create the anon layer with the API
    // because the mel command to add the path will hold to that anon layer if we undo
    UndoContext context(params.commandHook, params.name);
    for (auto item : *params.selection) {
        auto oldParent = item->parentLayerItem()->layer();
        // create an anon layer as the new parent
        auto anonLayer = PXR_NS::SdfLayer::CreateAnonymous(
            item->parentModel()->findNameForNewAnonymousLayer());
        // insert this selected item under it
        anonLayer->InsertSubLayerPath(item->layer()->GetIdentifier());
        // replace this selected item in its parent with the anon layer
        params.commandHook->replaceSubLayerPath(
            oldParent, item->subLayerPath(), anonLayer->GetIdentifier());
        // if there is only one, a common case, select it
        if (params.selection->size() == 1)
            item->parentModel()->selectUsdLayerOnIdle(anonLayer);
    }
}

void LayerTreeView::onMuteLayer(const QString& undoName) const
{
    auto selection = getSelectedLayerItems();

    CallMethodParams params;
    params.selection = &selection;
    params.commandHook = _model->sessionState()->commandHook();
    params.name = undoName;

    bool mute = !currentLayerItem()->isMuted();

    UndoContext context(params.commandHook, params.name);
    for (auto item : *params.selection) {
        item->parentModel()->toggleMuteLayer(item, &mute);
    }
}

void LayerTreeView::callMethodOnSelection(const QString& undoName, simpleLayerMethod method)
{
    CallMethodParams params;
    auto             selection = getSelectedLayerItems();
    params.selection = &selection;
    params.commandHook = _model->sessionState()->commandHook();
    params.name = undoName;
    doCallMethodOnSelection(params, method);
}

void LayerTreeView::paintEvent(QPaintEvent* event)
{
    PARENT_CLASS::paintEvent(event);
    // Overrides the paint event to make it so that place holder text is displayed when the list is
    // empty.
    if (model()->rowCount() == 0) {
        int  NO_LAYERS_IMAGE_SIZE = DPIScale(62);
        int  HALF_FONT_HEIGHT = DPIScale(7);
        auto NO_LAYERS_IMAGE = utils->createPixmap(":/RS_no_layer.png");
        QPen PLACEHOLDER_TEXT_PEN(QColor(128, 128, 128));

        QPainter painter(viewport());
        int      sz = NO_LAYERS_IMAGE_SIZE / 2;
        auto     pos = contentsRect().center() - QPoint(sz, sz + HALF_FONT_HEIGHT);
        painter.drawPixmap(pos, NO_LAYERS_IMAGE);

        auto oldPen = painter.pen();
        painter.setPen(PLACEHOLDER_TEXT_PEN);
        auto textRect = contentsRect();
        textRect.translate(0, sz);
        painter.drawText(
            textRect, Qt::AlignCenter, StringResources::getAsQString(StringResources::kNoLayers));
        painter.setPen(oldPen);
    }
}

bool LayerTreeView::event(QEvent* event)
{
    // override for dynamic tooltips
    if (event->type() == QEvent::ToolTip) {
        handleTooltips(dynamic_cast<QHelpEvent*>(event));
        return true;
    } else {
        return PARENT_CLASS::event(event);
    }
}

void LayerTreeView::handleTooltips(QHelpEvent* event)
{
    auto index = indexAt(event->pos());
    if (index.isValid()) {
        auto itemRect = visualRect(index);
        auto layerTreeItem = _model->layerItemFromIndex(index);
        itemRect = _delegate->getAdjustedItemRect(layerTreeItem, itemRect);
        auto targetRect = _delegate->getTargetIconRect(itemRect);
        auto textRect = _delegate->getTextRect(itemRect);
        if (targetRect.contains(event->pos())) {
            QString tip
                = StringResources::getAsQString(StringResources::kSetLayerAsTargetLayerTooltip);
            QToolTip::showText(event->globalPos(), tip);
            return;
        } else if (textRect.contains(event->pos())) {
            QString tip;
            if (layerTreeItem->isInvalidLayer()) {
                tip = StringResources::getAsQString(StringResources::kPathNotFound)
                    + layerTreeItem->subLayerPath().c_str();
            } else {
                tip = layerTreeItem->layer()->GetRealPath().c_str();
            }
            QToolTip::showText(event->globalPos(), tip);
            return;
        }
    }
    QToolTip::hideText();
    event->ignore();
}

void LayerTreeView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        auto index = indexAt(event->pos());
        // get the action button under the mouse if there is one
        auto action = getCurrentAction(event->modifiers(), event->pos(), index);
        _actionButtons._mouseReleaseAction = action;
        if (action) {
            _actionButtons._actionButtonPressed = true;
            event->accept();
            return;
        }
    }
    PARENT_CLASS::mousePressEvent(event);
}

// support for renderSetup-like action button API
void LayerTreeView::mouseMoveEvent(QMouseEvent* event)
{

    // dirty the tree view so it will repaint when mouse is over it
    // this is needed to change the icons when hovered over them
    _delegate->clearLastHitAction();
    auto region = childrenRegion();
    setDirtyRegion(region);
    // don't trigger D&D if button pressed
    if (!_actionButtons._actionButtonPressed && !_delegate->isTargetPressed()) {
        PARENT_CLASS::mouseMoveEvent(event);
    }
}

void LayerTreeView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _actionButtons._mouseReleaseAction) {
        auto index = indexAt(event->pos());
        auto action = getCurrentAction(event->modifiers(), event->pos(), index);
        _actionButtons._actionButtonPressed = false;
        if (action == _actionButtons._mouseReleaseAction) {
            // set the currently clicked on element active without selecting it
            selectionModel()->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
            // trigger the action to be executed
            action->trigger();
            event->accept();
            _actionButtons._mouseReleaseAction = nullptr;
            return;
        }
    }

    PARENT_CLASS::mouseReleaseEvent(event);
    _delegate->clearPressedTarget();
}

void LayerTreeView::keyPressEvent(QKeyEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        if (event->key() == Qt::Key_Delete) {
            CallMethodParams params;
            auto             selection = getSelectedLayerItems();
            params.selection = &selection;
            params.commandHook = _model->sessionState()->commandHook();
            params.name = "Remove";
            doCallMethodOnSelection(params, &LayerTreeItem::removeSubLayer);
            return;
        } else if (event->key() == Qt::Key_R) {
            _model->forceRefresh();
            return;
        }
    }
    return PARENT_CLASS::keyPressEvent(event);
}

// support for renderSetup-like action button API
QAction* LayerTreeView::getCurrentAction(
    QFlags<Qt::KeyboardModifier> modifiers,
    QPoint                       pos,
    QModelIndex                  index) const
{
    auto item = _model->itemFromIndex(index);
    if (item) {
        auto actionName = _delegate->lastHitAction();
        if (!actionName.isEmpty()) {
            for (auto a : _actionButtons._staticActions) {
                if (a->text() == actionName)
                    return a;
            }
        }
    }
    return nullptr;
}

void LayerTreeView::leaveEvent(QEvent* event)
{
    //
    _delegate->clearLastHitAction();
}

void LayerTreeView::onMuteLayerButtonPushed()
{
    auto item = currentLayerItem();
    if (item) {
        item->parentModel()->toggleMuteLayer(item);
    }
    // need to force redraw of everything otherwise redraw isn't right
    update();
}

} // namespace UsdLayerEditor
