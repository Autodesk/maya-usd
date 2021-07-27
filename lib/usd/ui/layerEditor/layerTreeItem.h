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

#ifndef LAYERTREEITEM_H
#define LAYERTREEITEM_H

#include "pxr/usd/sdf/layer.h"

#include <pxr/usd/sdf/declareHandles.h>
#include <pxr/usd/usd/stage.h>

#include <QtGui/QStandardItem>

#include <algorithm>
#include <string>
#include <vector>

namespace UsdLayerEditor {
class AbstractCommandHook;
class LayerTreeModel;
class LayerTreeItem;
class RecursionDetector;

enum class LayerType
{
    SessionLayer,
    RootLayer,
    SubLayer
};
enum class RebuildChildren
{
    Yes,
    No
};

struct LayerActionInfo
{
    QString _name;
    QString _tooltip;
    QPixmap _pixmap;
    int     extraPadding = 0;
    QColor  _borderColor;
    bool    _checked = false;
};

typedef std::vector<std::string>    recursionDetection;
typedef std::vector<LayerTreeItem*> LayerItemVector;

/**
 * @brief Implements one USD layer item in the treeview
 *
 */

class LayerTreeItem : public QStandardItem
{
public:
    LayerTreeItem(
        PXR_NS::SdfLayerRefPtr in_usdLayer,
        LayerType              in_layerType = LayerType::SubLayer,
        std::string            in_subLayerPath = "",
        std::set<std::string>* in_incomingLayers = nullptr,
        bool                   in_sharedStage = false,
        std::set<std::string>* in_sharedLayers = nullptr,
        RecursionDetector*     in_recursionDetector = nullptr);

    // refresh our data from the USD Layer
    void fetchData(RebuildChildren in_rebuild, RecursionDetector* in_recursionDetector = nullptr);

    // QStandardItem API
    int      type() const override;
    QVariant data(int role) const override;
    void     emitDataChanged() { QStandardItem::emitDataChanged(); }

    // parent(), properly typed
    LayerTreeItem* parentLayerItem() const { return dynamic_cast<LayerTreeItem*>(parent()); }
    // model, properly typed
    LayerTreeModel* parentModel() const;
    // get the display name
    const std::string& displayName() const { return _displayName; }
    // if a sublayer, get the path we were saved with in the parent
    const std::string& subLayerPath() const { return _subLayerPath; }
    // shortcut to get stage from model
    PXR_NS::UsdStageRefPtr const& stage() const;

    // is the layer muted at the stage level?
    bool isMuted() const;
    // check if this layer is muted, or any of its parent
    bool appearsMuted() const;
    // check if this layer is readonly
    bool isReadOnly() const;
    // true if dirty, but look at needsSaving for UI feedback
    bool isDirty() const { return _layer ? _layer->IsDirty() : false; }
    // need to indicate visually that layer has something to save
    bool needsSaving() const;
    // for drag and drop
    bool isMovable() const;
    // check if the layer is incoming (from a connection)
    bool isIncoming() const;

    // used by draw delegate: returns how deep in the hierarchy we are
    int depth() const;
    // is this sublayer with a path that doesn't load?
    bool isInvalidLayer() const { return !_layer; }
    // update the target layer flags -- meant to be called from invisible root
    void updateTargetLayerRecursive(const PXR_NS::SdfLayerRefPtr& newTargetLayer);

    // USD Layer type query
    bool                   isSessionLayer() const { return _layerType == LayerType::SessionLayer; }
    bool                   isSublayer() const { return _layerType == LayerType::SubLayer; }
    bool                   isTargetLayer() const { return _isTargetLayer; }
    bool                   isAnonymous() const { return _layer ? _layer->IsAnonymous() : false; }
    bool                   isRootLayer() const { return _layerType == LayerType::RootLayer; }
    PXR_NS::SdfLayerRefPtr layer() const { return _layer; }

    // allows c++ iteration of children
    LayerItemVector childrenVector() const;

    // menu callbacks
    void removeSubLayer();
    void saveEdits();
    void saveEditsNoPrompt();
    void discardEdits();

    // there are two addAnonymousSubLayer , because the menu needs all method to be void
    void                   addAnonymousSublayer();
    PXR_NS::SdfLayerRefPtr addAnonymousSublayerAndReturn();
    void                   loadSubLayers(QWidget* in_parent);
    void                   printLayer();
    void                   clearLayer();

    // delegate Action API for command buttons
    int getActionButtonCount() const { return (isSublayer() && !isInvalidLayer()) ? 1 : 0; }
    // delegate Action API for command buttons
    void getActionButton(int index, LayerActionInfo* out_info) const;
    static const std::vector<LayerActionInfo>& actionButtonsDefinition();

protected:
    PXR_NS::SdfLayerRefPtr _layer;
    std::string            _displayName;
    bool                   _isTargetLayer = false;
    LayerType              _layerType = LayerType::SubLayer;
    std::string            _subLayerPath; // name of the layer as it was found in the parent's stack
    bool                   _isIncomingLayer;
    std::set<std::string>  _incomingLayers;
    bool                   _isSharedStage;
    bool                   _isSharedLayer;
    std::set<std::string>  _sharedLayers;

    static std::vector<LayerActionInfo> _actionButtons;

protected:
    AbstractCommandHook* commandHook() const;

protected:
    void populateChildren(RecursionDetector* in_recursionDetector);
    // helper to save anon layers called by saveEdits()
    void saveAnonymousLayer();

private:
    bool sublayerOfShared() const;
};

class RecursionDetector
{
public:
    RecursionDetector() { }
    void push(const std::string& path) { _paths.push_back(path); }

    void pop() { _paths.pop_back(); }
    bool contains(const std::string& in_path) const
    {
        return !in_path.empty()
            && std::find(_paths.cbegin(), _paths.cend(), in_path) != _paths.cend();
    }

    std::vector<std::string> _paths;
};

} // namespace UsdLayerEditor

#endif // LAYERTREEITEM_H
