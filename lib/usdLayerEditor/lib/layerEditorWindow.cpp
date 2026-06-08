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

#include "layerEditorWindow.h"
#include "layerEditorWidget.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"
#include "sessionState.h"
#include "stringResources.h"

#include <QtCore/QPointer>
#include <QtWidgets/QMenu>
#include <vector>

namespace UsdLayerEditor {

AbstractLayerEditorCreator* AbstractLayerEditorCreator::_instance = nullptr;

AbstractLayerEditorCreator* AbstractLayerEditorCreator::instance()
{
    return _instance;
}

AbstractLayerEditorCreator::AbstractLayerEditorCreator()
{
    _instance = this;
}

AbstractLayerEditorCreator::~AbstractLayerEditorCreator()
{
    _instance = nullptr;
}

AbstractLayerEditorWindow::AbstractLayerEditorWindow(const char* panelName)
{
    // this empty implementation is necessary for linking
}

AbstractLayerEditorWindow::~AbstractLayerEditorWindow()
{
    // this empty implementation is necessary for linking
}

LayerEditorWindow::LayerEditorWindow(const char* panelName) :
    AbstractLayerEditorWindow(panelName), _panelName(panelName)
{
    if (!UsdLayerEditor::getQtUtils()) {
        UsdLayerEditor::setQtUtils(new QtUtils());
    }
}

LayerEditorWindow::~LayerEditorWindow() { }


LayerTreeView* LayerEditorWindow::treeView() { return _layerEditor->layerTree(); }

int LayerEditorWindow::selectionLength()
{
    auto selection = treeView()->getSelectedLayerItems();
    return static_cast<int>(selection.size());
}

#define CALL_CURRENT_ITEM(method)                 \
    auto item = treeView() -> currentLayerItem(); \
    return (item == nullptr) ? false : item->method()

bool LayerEditorWindow::isInvalidLayer() { CALL_CURRENT_ITEM(isInvalidLayer); }
bool LayerEditorWindow::isSessionLayer() { CALL_CURRENT_ITEM(isSessionLayer); }
bool LayerEditorWindow::isLayerDirty() { CALL_CURRENT_ITEM(isDirty); }
bool LayerEditorWindow::isSubLayer() { CALL_CURRENT_ITEM(isSublayer); }
bool LayerEditorWindow::isAnonymousLayer() { CALL_CURRENT_ITEM(isAnonymous); }
bool LayerEditorWindow::isIncomingLayer() { CALL_CURRENT_ITEM(isIncoming); }
bool LayerEditorWindow::layerNeedsSaving() { CALL_CURRENT_ITEM(needsSaving); }
bool LayerEditorWindow::layerAppearsMuted() { CALL_CURRENT_ITEM(appearsMuted); }
bool LayerEditorWindow::layerIsMuted() { CALL_CURRENT_ITEM(isMuted); }
bool LayerEditorWindow::layerIsReadOnly() { CALL_CURRENT_ITEM(isReadOnly); }
bool LayerEditorWindow::layerAppearsLocked() { CALL_CURRENT_ITEM(appearsLocked); }
bool LayerEditorWindow::layerIsLocked() { CALL_CURRENT_ITEM(isLocked); }
bool LayerEditorWindow::layerAppearsSystemLocked() { CALL_CURRENT_ITEM(appearsSystemLocked); }
bool LayerEditorWindow::layerIsSystemLocked() { CALL_CURRENT_ITEM(isSystemLocked); }
bool LayerEditorWindow::layerHasSubLayers() { CALL_CURRENT_ITEM(hasSubLayers); }

void LayerEditorWindow::removeSubLayer()
{
    QString name = "Remove";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::removeSubLayer);
}

void LayerEditorWindow::saveEdits()
{
    auto item = treeView()->currentLayerItem();
    if (item) {
        QString name = item->isAnonymous() ? "Save As..." : "Save Edits";
        treeView()->callMethodOnSelection(name, &LayerTreeItem::saveEdits);
    }
}

void LayerEditorWindow::discardEdits()
{
    QString name = "Discard Edits";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::discardEdits);
}

void LayerEditorWindow::addAnonymousSublayer()
{
    QString name = "Add Sublayer";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::addAnonymousSublayer);
}

void LayerEditorWindow::updateLayerModel() { getSessionState()->refreshCurrentStageEntry(); }

void LayerEditorWindow::lockLayer()
{
    auto item = treeView()->currentLayerItem();
    if (item != nullptr) {
        QString name = item->isLocked() ? "Unlock" : "Lock";
        treeView()->onLockLayer(name);
    }
}

void LayerEditorWindow::lockLayerAndSubLayers()
{
    auto item = treeView()->currentLayerItem();
    if (item != nullptr) {
        QString name = item->isLocked() ? "Unlock Layer and Sublayers" : "Lock Layer and Sublayers";
        bool    includeSubLayers = true;
        treeView()->onLockLayerAndSublayers(name, includeSubLayers);
    }
}

void LayerEditorWindow::stitchLayers()
{
    const auto selectedItems = treeView()->getSelectedLayerItems();

    if (selectedItems.size() < 2)
        return;

    std::vector<PXR_NS::SdfLayerRefPtr> layers;
    layers.reserve(selectedItems.size());

    for (const auto& item : selectedItems) {
        if (!item)
            continue;

        const auto layer = item->layer();
        if (!layer)
            continue;

        layers.push_back(layer);
    }

    if (layers.size() < 2)
        return;

    getSessionState()->commandHook()->stitchLayers(layers);
}

void LayerEditorWindow::addParentLayer()
{
    QString name = "Add Parent Layer";
    treeView()->onAddParentLayer(name);
}

void LayerEditorWindow::loadSubLayers()
{
    auto item = treeView()->currentLayerItem();
    if (item) {
        item->loadSubLayers(getMainWindow());
    }
}

void LayerEditorWindow::muteLayer()
{
    auto item = treeView()->currentLayerItem();
    if (item != nullptr) {
        QString name = item->isMuted() ? "Unmute" : "Mute";
        treeView()->onMuteLayer(name);
    }
}

void LayerEditorWindow::printLayer()
{
    QString name = "Print to Listener";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::printLayer);
}

void LayerEditorWindow::clearLayer()
{
    // Suspend usd notification while clearing and force a refresh after
    // all layers are cleared. This is required because callMethodOnSelection()
    // will loop on all selected layers and clear them one by one, If a refresh
    // happen before callMethodOnSelection() finish to loop over the selected item,
    // maya will crash because of a dangling pointer (All layer item are deleted
    // during the refresh).
    LayerTreeModel::suspendUsdNotices(true);

    QString name = "Clear";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::clearLayer);

    LayerTreeModel::suspendUsdNotices(false);

    LayerTreeModel* model = treeView()->layerTreeModel();
    model->forceRefresh();
}

void LayerEditorWindow::mergeWithSublayers()
{
    QString name = "Merge with Sublayers";
    treeView()->callMethodOnSelection(name, &LayerTreeItem::mergeWithSublayers);
}

void LayerEditorWindow::selectPrimsWithSpec()
{
    auto item = treeView()->currentLayerItem();
    if (item != nullptr) {
        getSessionState()->commandHook()->selectPrimsWithSpec(item->layer());
    }
}

void LayerEditorWindow::buildContextMenu(const QPoint& pos)
{
    const bool singleSelect = treeView()->selectionModel()->selectedRows().size() == 1;
    const bool multiSelect  = treeView()->selectionModel()->selectedRows().size() > 1;
    const auto isInvalid    = isInvalidLayer();
    const auto hasSublayers = layerHasSubLayers();

    const auto menu = new QMenu();

    const auto needsSaving        = layerNeedsSaving();
    const auto isReadOnly         = layerIsReadOnly();
    const auto isLocked           = layerIsLocked();
    const auto isSystemLocked     = layerIsSystemLocked();
    const auto isSublayer         = isSubLayer();
    const auto isSessionLyr       = isSessionLayer();
    const auto isAnonymousLyr     = isAnonymousLayer();
    const auto appearsLocked      = layerAppearsLocked();
    const auto appearsSystemLocked = layerAppearsSystemLocked();
    const auto appearsMuted       = layerAppearsMuted();

    auto addRemoveLayerAction = [this, &menu, &isReadOnly, &appearsLocked, &appearsSystemLocked] {
        QString label  = QObject::tr("Remove");
        auto    action = menu->addAction(label);
        action->setEnabled(!isReadOnly && !appearsLocked && !appearsSystemLocked);
        QObject::connect(action, &QAction::triggered, [this]() { removeSubLayer(); });
    };

    if (isInvalid) {
        addRemoveLayerAction();
        menu->exec(_layerEditor->layerTree()->viewport()->mapToGlobal(pos));
        return;
    }

    // Save / Reload — not shown for session layer
    if (!isSessionLyr) {
        QString label  = isAnonymousLyr ? QObject::tr("Save As...") : QObject::tr("Save Edits");
        bool    enable = singleSelect && needsSaving && !isSystemLocked;
        if (isAnonymousLyr)
            enable = enable && !appearsLocked && !appearsSystemLocked;
        auto action = menu->addAction(label);
        action->setEnabled(enable);
        QObject::connect(action, &QAction::triggered, [this]() { saveEdits(); });
    }

    if (!isAnonymousLyr) {
        auto action = menu->addAction(QObject::tr("Reload"));
        QObject::connect(action, &QAction::triggered, [this]() { discardEdits(); });
    }

    menu->addSeparator();

    // Sublayer management
    {
        auto       action  = menu->addAction(QObject::tr("Add Sublayer"));
        const bool enabled = !appearsMuted && !isReadOnly && !isLocked && !isSystemLocked;
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, [this]() { addAnonymousSublayer(); });
    }

    {
        auto       action  = menu->addAction(QObject::tr("Add Parent Layer"));
        const bool enabled = isSublayer && !appearsMuted && !isReadOnly && !appearsLocked && !appearsSystemLocked;
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, [this]() { addParentLayer(); });
    }

    {
        auto       action  = menu->addAction(QObject::tr("Load Sublayers..."));
        const bool enabled = singleSelect && !appearsMuted && !isReadOnly && !isLocked && !isSystemLocked;
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, [this]() { loadSubLayers(); });
    }

    if (multiSelect && !singleSelect) {
        QString label   = StringResources::getAsQString(StringResources::kMenuStitchLayers);
        auto    action  = menu->addAction(label);
        bool    enabled = !isReadOnly && !isLocked && !appearsSystemLocked && !appearsMuted;
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, [this]() { stitchLayers(); });
    }

    if (hasSublayers) {
        auto       action  = menu->addAction(QObject::tr("Merge with Sublayers"));
        const bool enabled = !appearsMuted && !isReadOnly && !isLocked && !isSystemLocked;
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, [this]() { mergeWithSublayers(); });
    }

    menu->addSeparator();

    // Mute / Lock
    if (isSublayer) {
        QString label  = layerIsMuted() ? QObject::tr("Unmute") : QObject::tr("Mute");
        auto    action = menu->addAction(label);
        QObject::connect(action, &QAction::triggered, [this]() { muteLayer(); });
    }

    if (!isSessionLyr) {
        {
            QString label  = isLocked ? QObject::tr("Unlock") : QObject::tr("Lock");
            auto    action = menu->addAction(label);
            action->setEnabled(!isSystemLocked);
            QObject::connect(action, &QAction::triggered, [this]() { lockLayer(); });
        }

        if (hasSublayers) {
            QString label  = isLocked ? QObject::tr("Unlock Layer and Sublayers")
                                      : QObject::tr("Lock Layer and Sublayers");
            auto    action = menu->addAction(label);
            action->setEnabled(!isSystemLocked);
            QObject::connect(action, &QAction::triggered, [this]() { lockLayerAndSubLayers(); });
        }
    }

    {
        auto action = menu->addAction(QObject::tr("Print to Listener"));
        QObject::connect(action, &QAction::triggered, [this]() { printLayer(); });
    }

    menu->addSeparator();

    {
        auto action = menu->addAction(QObject::tr("Select Prims with Spec"));
        QObject::connect(action, &QAction::triggered, [this]() { selectPrimsWithSpec(); });
    }

    // DCC-specific extensions (e.g. "Select Incoming Node" in Maya)
    addDCCContextMenuItems(menu);

    if (isSublayer || !isAnonymousLyr) {
        menu->addSeparator();
    }

    if (isSublayer) {
        addRemoveLayerAction();
    }

    {
        auto    action  = menu->addAction(QObject::tr("Clear"));
        bool    enabled = !isReadOnly && !isLocked && !isSystemLocked;
        action->setEnabled(enabled);
        QObject::connect(action, &QAction::triggered, [this]() { clearLayer(); });
    }

    menu->exec(_layerEditor->layerTree()->viewport()->mapToGlobal(pos));
}

} // namespace UsdLayerEditor
