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

#include "layerContextMenu.h"
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

AbstractLayerEditorCreator* AbstractLayerEditorCreator::instance() { return _instance; }

AbstractLayerEditorCreator::AbstractLayerEditorCreator() { _instance = this; }

AbstractLayerEditorCreator::~AbstractLayerEditorCreator() { _instance = nullptr; }

AbstractLayerEditorWindow::AbstractLayerEditorWindow(const char* panelName)
{
    // this empty implementation is necessary for linking
}

AbstractLayerEditorWindow::~AbstractLayerEditorWindow()
{
    // this empty implementation is necessary for linking
}

LayerEditorWindow::LayerEditorWindow(const char* panelName)
    : AbstractLayerEditorWindow(panelName)
    , _panelName(panelName)
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

#define CALL_CURRENT_ITEM(method)               \
    auto item = treeView()->currentLayerItem(); \
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
bool LayerEditorWindow::layerIsActiveRenderLayer() { CALL_CURRENT_ITEM(isActiveRenderLayer); }
bool LayerEditorWindow::stageHasActiveRenderLayer()
{
    CALL_CURRENT_ITEM(stageHasActiveRenderLayer);
}

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
    QMenu menu;
    menu.setSeparatorsCollapsible(false);
    populateLayerContextMenu(
        menu, *this, *treeView(), [this](QMenu& dccMenu) { addDCCContextMenuItems(&dccMenu); });
    menu.exec(_layerEditor->layerTree()->viewport()->mapToGlobal(pos));
}

} // namespace UsdLayerEditor
