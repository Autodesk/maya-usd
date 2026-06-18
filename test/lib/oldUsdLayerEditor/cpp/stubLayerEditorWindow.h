// Copyright 2026 Autodesk
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

#include "layerEditorWidget.h"
#include "layerTreeItem.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"
#include "stubSessionState.h"

#include <mayaUsd/commands/abstractLayerEditorWindow.h>

#include <QtWidgets/QMainWindow>

#include <string>
#include <vector>

namespace UsdLayerEditor {

class OldEditorStubLayerEditorWindow : public MayaUsd::AbstractLayerEditorWindow
{
public:
    OldEditorStubLayerEditorWindow(
        OldEditorStubSessionState& sessionState, QMainWindow* parent)
        : MayaUsd::AbstractLayerEditorWindow("stub_panel")
        , _sessionState(sessionState)
    {
        _layerEditor = new LayerEditorWidget(sessionState, parent);
    }

    LayerEditorWidget* widget() const { return _layerEditor; }

    // --- query methods ---

    int selectionLength() override
    {
        return static_cast<int>(treeView()->getSelectedLayerItems().size());
    }

    bool hasCurrentLayerItem() override { return treeView()->currentLayerItem() != nullptr; }

#define ITEM_QUERY(method)                      \
    auto item = treeView()->currentLayerItem(); \
    return item ? item->method() : false

    bool isInvalidLayer()       override { ITEM_QUERY(isInvalidLayer); }
    bool isSessionLayer()       override { ITEM_QUERY(isSessionLayer); }
    bool isLayerDirty()         override { ITEM_QUERY(isDirty); }
    bool isSubLayer()           override { ITEM_QUERY(isSublayer); }
    bool isAnonymousLayer()     override { ITEM_QUERY(isAnonymous); }
    bool isIncomingLayer()      override { ITEM_QUERY(isIncoming); }
    bool layerNeedsSaving()     override { ITEM_QUERY(needsSaving); }
    bool layerAppearsMuted()    override { ITEM_QUERY(appearsMuted); }
    bool layerIsMuted()         override { ITEM_QUERY(isMuted); }
    bool layerIsReadOnly()      override { ITEM_QUERY(isReadOnly); }
    bool layerAppearsLocked()   override { ITEM_QUERY(appearsLocked); }
    bool layerIsLocked()        override { ITEM_QUERY(isLocked); }
    bool layerAppearsSystemLocked() override { ITEM_QUERY(appearsSystemLocked); }
    bool layerIsSystemLocked()  override { ITEM_QUERY(isSystemLocked); }
    bool layerHasSubLayers()    override { ITEM_QUERY(hasSubLayers); }

#undef ITEM_QUERY

    std::string proxyShapeName(const bool /*fullPath*/ = false) const override
    {
        return "stub_panel";
    }

    // --- action methods ---

    void removeSubLayer() override
    {
        treeView()->callMethodOnSelectionNoDelay("Remove", &LayerTreeItem::removeSubLayer);
    }

    void saveEdits() override
    {
        auto item = treeView()->currentLayerItem();
        if (item) {
            QString name = item->isAnonymous() ? "Save As..." : "Save Edits";
            treeView()->callMethodOnSelection(name, &LayerTreeItem::saveEdits);
        }
    }

    void discardEdits() override
    {
        treeView()->callMethodOnSelection("Discard Edits", &LayerTreeItem::discardEdits);
    }

    void addAnonymousSublayer() override
    {
        treeView()->callMethodOnSelection("Add Sublayer", &LayerTreeItem::addAnonymousSublayer);
    }

    void addParentLayer() override { }

    void loadSubLayers() override
    {
        auto item = treeView()->currentLayerItem();
        if (item)
            item->loadSubLayers(_layerEditor);
    }

    void muteLayer() override
    {
        auto item = treeView()->currentLayerItem();
        if (item) {
            QString name = item->isMuted() ? "Unmute" : "Mute";
            treeView()->onMuteLayer(name);
        }
    }

    void printLayer() override
    {
        treeView()->callMethodOnSelection("Print to Script Editor", &LayerTreeItem::printLayer);
    }

    void clearLayer() override
    {
        // Suspend notices during the loop to avoid dangling-pointer crash on refresh mid-loop.
        LayerTreeModel::suspendUsdNotices(true);
        treeView()->callMethodOnSelection("Clear", &LayerTreeItem::clearLayer);
        LayerTreeModel::suspendUsdNotices(false);
        treeView()->layerTreeModel()->forceRefresh();
    }

    void mergeWithSublayers() override
    {
        treeView()->callMethodOnSelection(
            "Merge with Sublayers", &LayerTreeItem::mergeWithSublayers);
    }

    void selectPrimsWithSpec() override
    {
        auto item = treeView()->currentLayerItem();
        if (item)
            _sessionState.commandHook()->selectPrimsWithSpec(item->layer());
    }

    void updateLayerModel() override { }

    void lockLayer() override
    {
        auto item = treeView()->currentLayerItem();
        if (item) {
            QString name = item->isLocked() ? "Unlock" : "Lock";
            treeView()->onLockLayer(name);
        }
    }

    void lockLayerAndSubLayers() override
    {
        auto item = treeView()->currentLayerItem();
        if (item) {
            QString name
                = item->isLocked() ? "Unlock Layer and Sublayers" : "Lock Layer and Sublayers";
            treeView()->onLockLayerAndSublayers(name, /*includeSublayers=*/true);
        }
    }

    void stitchLayers() override
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

        _sessionState.commandHook()->stitchLayers(layers);
    }

    void selectProxyShape(const char* /*shapePath*/) override { }

    std::vector<std::string> getSelectedLayers() override { return {}; }

    void selectLayers(std::vector<std::string> /*layerIds*/) override { }

private:
    LayerTreeView* treeView() { return _layerEditor->layerTree(); }

    OldEditorStubSessionState& _sessionState;
    LayerEditorWidget*         _layerEditor { nullptr };
};

} // namespace UsdLayerEditor
