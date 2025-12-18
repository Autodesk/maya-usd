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

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <QtGui/QColor>
#include <QtGui/QCursor>
#include <QtWidgets/QMenu>

using namespace UsdLayerEditor;

struct CallMethodParams
{
    const LayerItemVector* selection;
    QString                name;
    AbstractCommandHook*   commandHook;
};

namespace {
QColor BLACK_BACKGROUND(55, 55, 55);

typedef void (LayerTreeItem::*simpleLayerMethod)(QWidget* parent);

void doCallMethodOnSelection(
    const CallMethodParams& params,
    simpleLayerMethod       method,
    QWidget*                in_parent)
{
    if (params.selection->size() > 0) {
        UndoContext context(params.commandHook, params.name);

        for (auto item : *params.selection) {
            (item->*method)(in_parent);
        }
    }
}

class LayerTreeViewRefreshCallback : public UsdUfe::UICallback
{
public:
    LayerTreeViewRefreshCallback(LayerTreeView* treeView)
        : UICallback()
        , _treeView(treeView)
    {
    }

    void
    operator()(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& callbackData) override
    {
        if (!_treeView)
            return;

        _treeView->repaint();
    }

private:
    LayerTreeView* _treeView;
};

} // namespace

namespace UsdLayerEditor {

LayerTreeView::LayerTreeView(SessionState* in_sessionState, QWidget* in_parent)
    : QTreeView(in_parent)
{
    _model = new LayerTreeModel(in_sessionState, this);
    setModel(_model);
    connect(_model, &LayerTreeModel::selectLayerSignal, this, &LayerTreeView::selectLayerRequest);

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
    updateMouseCursor();

    // custom row drawing
    _delegate = new LayerTreeItemDelegate(this);
    setItemDelegate(_delegate);
    connect(
        _model, &QAbstractItemModel::modelReset, _delegate, &LayerTreeItemDelegate::onModelReset);

    // context menu
    setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

    // updates
    connect(
        _model,
        &QAbstractItemModel::modelAboutToBeReset,
        this,
        &LayerTreeView::onModelAboutToBeReset);
    connect(_model, &QAbstractItemModel::modelReset, this, &LayerTreeView::onModelReset);

    // signals
    connect(this, &QAbstractItemView::doubleClicked, this, &LayerTreeView::onItemDoubleClicked);
    connect(this, &QTreeView::expanded, this, &LayerTreeView::onExpanded);
    connect(this, &QTreeView::collapsed, this, &LayerTreeView::onCollapsed);

    connect(
        _model->sessionState(),
        &SessionState::stageListChangedSignal,
        this,
        &LayerTreeView::updateFromSessionState);

    auto buttonDefinitions = LayerTreeItem::actionButtonsDefinition();
    auto muteActionIter = buttonDefinitions.find(LayerActionType::Mute);
    if (muteActionIter != buttonDefinitions.end()) {
        LayerActionInfo muteActionInfo = muteActionIter->second;
        auto            muteAction = new QAction(muteActionInfo._name, this);
        connect(muteAction, &QAction::triggered, this, &LayerTreeView::onMuteLayerButtonPushed);
        _actionButtons._staticActions.push_back(muteAction);
    }
    auto lockActionIter = buttonDefinitions.find(LayerActionType::Lock);
    if (lockActionIter != buttonDefinitions.end()) {
        LayerActionInfo lockActionInfo = lockActionIter->second;
        auto            lockAction = new QAction(lockActionInfo._name, this);
        connect(lockAction, &QAction::triggered, this, &LayerTreeView::onLockLayerButtonPushed);
        _actionButtons._staticActions.push_back(lockAction);
    }

    _refreshCallback = std::make_shared<LayerTreeViewRefreshCallback>(this);
    UsdUfe::registerUICallback(PXR_NS::TfToken("onRefreshSystemLock"), _refreshCallback);

    TfWeakPtr<LayerTreeView> me(this);
    _layerMutingNoticeKey = TfNotice::Register(me, &LayerTreeView::onLayerMutingChanged);
}

LayerTreeView::~LayerTreeView()
{
    if (_refreshCallback) {
        UsdUfe::unregisterUICallback(PXR_NS::TfToken("onRefreshSystemLock"), _refreshCallback);
        _refreshCallback.reset();
    }

    // Stop listening to layer muting.
    TfNotice::Revoke(_layerMutingNoticeKey);
}

void LayerTreeView::onLayerMutingChanged(const UsdNotice::LayerMutingChanged&) { repaint(); }

LayerTreeItem* LayerTreeView::layerItemFromIndex(const QModelIndex& index) const
{
    return _model->layerItemFromIndex(index);
}

LayerTreeModel* LayerTreeView::layerTreeModel() const { return _model.data(); }

void LayerTreeView::selectLayerRequest(const QModelIndex& index)
{
    // slot called when the user manually adds a sublayer with the UI
    // we use this to select the new layer
    setCurrentIndex(index);
    scrollTo(index);
}

void LayerTreeView::onItemDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    auto layerTreeItem = layerItemFromIndex(index);
    if (!layerTreeItem->needsSaving())
        return;

    // Note: system-locked layers cannot be saved.
    if (layerTreeItem->isSystemLocked() || layerTreeItem->appearsSystemLocked())
        return;

    layerTreeItem->saveEdits(this);
}

bool LayerTreeView::shouldExpandOrCollapseAll() const
{
    // Internal private function to check if the expand and collapse of
    // items should be recursive. Currently, this is controlled by the
    // fact the user is pressing the SHIFT key on the keyboard.
    int modifiers = 0;
    MGlobal::executeCommand("getModifiers", modifiers);

    // Magic constant 2 is how the getModifiers reports the SHIFT key.
    // This is a public command and the shift value is only declared in
    // its documentation. Being a public command, it is practically
    // guaranteed to never change, so hard-coding the value is not a problem.
    const bool shiftHeld = ((modifiers % 2) != 0);
    return shiftHeld;
}

void LayerTreeView::onExpanded(const QModelIndex& index)
{
    if (!shouldExpandOrCollapseAll())
        return;

    expandChildren(index);
}

void LayerTreeView::onCollapsed(const QModelIndex& index)
{
    if (!shouldExpandOrCollapseAll())
        return;

    collapseChildren(index);
}

void LayerTreeView::expandChildren(const QModelIndex& index) { expandRecursively(index); }

void LayerTreeView::collapseChildren(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    // Recursively collapse each child node.
    const int count = index.model()->rowCount(index);
    for (int i = 0; i < count; i++) {
        const QModelIndex& child = index.model()->index(i, 0, index);
        collapseChildren(child);
    }

    collapse(index);
}

LayerViewMemento::LayerViewMemento(const LayerTreeView& view, const LayerTreeModel& model)
{
    preserve(view, model);
}

void LayerViewMemento::preserve(const LayerTreeView& view, const LayerTreeModel& model)
{
    if (QScrollBar* hsb = view.horizontalScrollBar()) {
        _horizontalScrollbarPosition = hsb->value();
    }
    if (QScrollBar* vsb = view.verticalScrollBar()) {
        _verticalScrollbarPosition = vsb->value();
    }

    auto                  selectionModel = view.selectionModel();
    const LayerItemVector items = model.getAllItems();
    if (items.size() == 0)
        return;

    auto currentIndex = selectionModel->currentIndex();

    for (const LayerTreeItem* item : items) {
        if (!item)
            continue;

        PXR_NS::SdfLayerRefPtr layer = item->layer();
        if (!layer)
            continue;

        const ItemId    id = layer->GetIdentifier();
        const ItemState state = { view.isExpanded(item->index()),
                                  selectionModel->isSelected(item->index()),
                                  item->index() == currentIndex };

        _itemsState[id] = state;
    }
}

void LayerViewMemento::restore(LayerTreeView& view, LayerTreeModel& model)
{
    const LayerItemVector items = model.getAllItems();

    QtDisableRepaintUpdates disableUpdates(view);

    QItemSelection* selection = nullptr;
    const auto      selectionModel = view.selectionModel();

    for (const LayerTreeItem* item : items) {
        if (!item)
            continue;

        bool expanded = false;

        PXR_NS::SdfLayerRefPtr layer = item->layer();
        if (layer) {
            const ItemId id = layer->GetIdentifier();
            const auto   state = _itemsState.find(id);
            if (state != _itemsState.end()) {
                expanded = state->second._expanded;
                if (state->second._selected) {
                    if (!selection) {
                        selection = new QItemSelection();
                    }
                    selection->select(item->index(), item->index());
                }
                if (state->second._current) {
                    selectionModel->setCurrentIndex(item->index(), QItemSelectionModel::NoUpdate);
                }
            }
        }

        view.setExpanded(item->index(), expanded);
    }

    if (selection != nullptr) {
        selectionModel->select(
            *selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        delete selection;
    }

    if (QScrollBar* hsb = view.horizontalScrollBar()) {
        if (hsb->value() != _horizontalScrollbarPosition) {
            hsb->setValue(_horizontalScrollbarPosition);
            hsb->valueChanged(_horizontalScrollbarPosition);
        }
    }
    if (QScrollBar* vsb = view.verticalScrollBar()) {
        if (vsb->value() != _verticalScrollbarPosition) {
            vsb->setValue(_verticalScrollbarPosition);
            vsb->valueChanged(_verticalScrollbarPosition);
        }
    }
}

void LayerTreeView::updateFromSessionState()
{
    if (_cachedModelState == nullptr) {
        return;
    }

    auto allStages = _model->sessionState()->allStages();
    std::map<LayerViewMemento::ItemId, LayerViewMemento::ItemState> newState;
    std::map<LayerViewMemento::ItemId, LayerViewMemento::ItemState> oldState
        = _cachedModelState->getItemsState();

    // Only keep the state of stages that still exist
    for (auto const& stageEntry : allStages) {
        auto stageLayers = stageEntry._stage->GetLayerStack();
        for (const auto& stageLayer : stageLayers) {
            newState[stageLayer->GetIdentifier()] = oldState[stageLayer->GetIdentifier()];
        }
    }
    _cachedModelState->setItemsState(newState);
}

void LayerTreeView::onModelAboutToBeReset()
{
    if (!_model)
        return;

    if (_cachedModelState == nullptr) {
        LayerViewMemento memento(*this, *_model);
        if (memento.empty())
            return;

        _cachedModelState = std::make_unique<LayerViewMemento>(std::move(memento));
    } else {
        // Save the state before resetting
        _cachedModelState->preserve(*this, *_model);
    }
}

void LayerTreeView::onModelReset()
{
    if (!_model)
        return;

    if (_cachedModelState) {
        _cachedModelState->restore(*this, *_model);
    } else {
        expandAll();
    }
}

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
    DelayAbstractCommandHook delayed(*_model->sessionState()->commandHook());

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
    DelayAbstractCommandHook delayed(*_model->sessionState()->commandHook());

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

void LayerTreeView::onLockLayer(const QString& undoName) const
{
    bool includeSubLayers = false;
    onLockLayerAndSublayers(undoName, includeSubLayers);
}

void LayerTreeView::onLockLayerAndSublayers(const QString& undoName, bool includeSublayers) const
{
    DelayAbstractCommandHook delayed(*_model->sessionState()->commandHook());

    auto selection = getSelectedLayerItems();

    CallMethodParams params;
    params.selection = &selection;
    params.commandHook = _model->sessionState()->commandHook();
    params.name = undoName;

    bool isLocked = !currentLayerItem()->isLocked();

    UndoContext context(params.commandHook, params.name);
    for (auto item : *params.selection) {
        item->parentModel()->toggleLockLayer(item, includeSublayers, &isLocked);
    }
}

void LayerTreeView::callMethodOnSelection(const QString& undoName, simpleLayerMethod method)
{
    DelayAbstractCommandHook delayed(*_model->sessionState()->commandHook());

    callMethodOnSelectionNoDelay(undoName, method);
}

void LayerTreeView::callMethodOnSelectionNoDelay(const QString& undoName, simpleLayerMethod method)
{
    CallMethodParams params;
    auto             selection = getSelectedLayerItems();
    params.selection = &selection;
    params.commandHook = _model->sessionState()->commandHook();
    params.name = undoName;
    doCallMethodOnSelection(params, method, this);
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

void LayerTreeView::updateMouseCursor()
{
    // Note: special mouse cursor taken from Maya resources.
    QString pixmapName = QtUtils::getDPIPixmapName(":/rmbMenu");
    // Note: in Maya, the normal-sized pixmap name does not ends with _100,
    //       so remove that ending if it is present.
    pixmapName.remove("_100");
    QPixmap pixmap(pixmapName);

    const int hitX = MQtUtil::dpiScale(11);
    const int hitY = MQtUtil::dpiScale(9);

    setCursor(QCursor(pixmap, hitX, hitY));
}

// support for renderSetup-like action button API
void LayerTreeView::mouseMoveEvent(QMouseEvent* event)
{
    updateMouseCursor();

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
    DelayAbstractCommandHook delayed(*_model->sessionState()->commandHook());

    if (event->type() == QEvent::KeyPress) {
        if (event->key() == Qt::Key_Delete) {
            CallMethodParams params;
            auto             selection = getSelectedLayerItems();
            params.selection = &selection;
            params.commandHook = _model->sessionState()->commandHook();
            params.name = "Remove";
            doCallMethodOnSelection(params, &LayerTreeItem::removeSubLayer, this);
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
    updateMouseCursor();
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

void LayerTreeView::onLockLayerButtonPushed()
{
    auto item = currentLayerItem();
    if (item && !item->isSystemLocked()) {
        bool includeSublayers = false;
        item->parentModel()->toggleLockLayer(item, includeSublayers);
    }
    // need to force redraw of everything otherwise redraw isn't right
    update();
}
} // namespace UsdLayerEditor
