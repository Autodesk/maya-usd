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

// Needs to come first when used with VS2017 and Qt5.
#include "pxr/usd/sdf/layer.h"

#include <pxr/usd/sdf/declareHandles.h>
#include <pxr/usd/usd/stage.h>

#include <QtGui/QStandardItem>

#include <algorithm>
#include <map>
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

enum LayerActionType
{
    None,
    Mute,
    Lock
};

enum LayerMasks
{
    LayerMasks_None = (1u << 0),     // 00000000
    LayerMasks_Session = (1u << 1),  // 00000001
    LayerMasks_Root = (1u << 2),     // 00000010
    LayerMasks_SubLayer = (1u << 3), // 00000100
};

LayerMasks operator|(LayerMasks lhs, LayerMasks rhs);

LayerMasks CreateLayerMask(bool isRootLayer, bool isSubLayer, bool isSessionLayer);

struct LayerActionInfo
{
    QString         _name;
    QString         _tooltip;
    QPixmap         _pixmap_off;
    QPixmap         _pixmap_off_hover;
    QPixmap         _pixmap_on;
    QPixmap         _pixmap_on_hover;
    int             _extraPadding = 0;
    QColor          _borderColor;
    bool            _checked = false;
    LayerMasks      _layerMask = LayerMasks::LayerMasks_None;
    LayerActionType _actionType;
    int             _order = 0;
};

bool IsLayerActionAllowed(const LayerActionInfo& actionInfo, LayerMasks layerMaskFlag);

using recursionDetection = std::vector<std::string>;
using LayerItemVector = std::vector<LayerTreeItem*>;
using LayerActionDefinitions = std::map<LayerActionType, LayerActionInfo>;
/**
 * @brief Implements one USD layer item in the treeview
 *
 */

class LayerTreeItem : public QStandardItem
{
public:
    LayerTreeItem(
        PXR_NS::SdfLayerRefPtr in_usdLayer,
        PXR_NS::UsdStageRefPtr in_stage,
        LayerType              in_layerType = LayerType::SubLayer,
        std::string            in_subLayerPath = "",
        std::set<std::string>* in_incomingLayers = nullptr,
        bool                   in_sharedStage = false,
        std::set<std::string>* in_sharedLayers = nullptr,
        RecursionDetector*     in_recursionDetector = nullptr);

    // refresh our data from the USD Layer
    void fetchData(RebuildChildren in_rebuild, RecursionDetector* in_recursionDetector = nullptr);

    enum Roles
    {
        // Data role containing a boolean indicating if the mouse is over an action button.
        kHoverActionRole = Qt::UserRole + 1
    };

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

    // is the layer muted at the stage level?
    bool isMuted() const;
    // check if this layer is muted, or any of its parent
    bool appearsMuted() const;
    // check if this layer is readonly (whether it is a shared layer or a sublayer of a shared
    // stage)
    bool isReadOnly() const;
    // true if dirty, but look at needsSaving for UI feedback
    bool isDirty() const { return _layer ? _layer->IsDirty() : false; }
    // need to indicate visually that layer has something to save
    bool needsSaving() const;
    // for drag and drop
    bool isMovable() const;
    // check if the layer is incoming (from a connection)
    bool isIncoming() const;
    // is the layer locked?
    bool isLocked() const;
    // check if this layer appears locked. This means that the layer item itself
    // may not be locked but by inference some of the action items of the layer
    // can appear as locked if the parent is locked.
    bool appearsLocked() const;
    // is the layer system locked?
    bool isSystemLocked() const;
    // check if this layer appears system locked. This means that the layer item itself
    // may not be system locked but by inference some of the action items of the layer
    // can appear as locked if the parent is system locked.
    bool appearsSystemLocked() const;
    // Checks if this layer has any sub layers.
    bool hasSubLayers() const;

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
    PXR_NS::SdfLayerRefPtr parentLayer() const
    {
        auto parentItem = parentLayerItem();
        return parentItem ? parentItem->layer() : nullptr;
    }

    // allows c++ iteration of children
    LayerItemVector childrenVector() const;

    // menu callbacks
    void removeSubLayer(QWidget* in_parent);
    void saveEdits(QWidget* in_parent);
    void saveEditsNoPrompt(QWidget* in_parent);
    void discardEdits(QWidget* in_parent);

    // there are two addAnonymousSubLayer , because the menu needs all method to be void
    void                   addAnonymousSublayer(QWidget* in_parent);
    PXR_NS::SdfLayerRefPtr addAnonymousSublayerAndReturn(QWidget* in_parent);
    void                   loadSubLayers(QWidget* in_parent);
    void                   printLayer(QWidget* in_parent);
    void                   clearLayer(QWidget* in_parent);

    // delegate Action API for command buttons
    void getActionButton(LayerActionType actionType, LayerActionInfo& out_info) const;
    static const LayerActionDefinitions& actionButtonsDefinition();

protected:
    PXR_NS::SdfLayerRefPtr _layer;
    PXR_NS::UsdStageRefPtr _stage;
    std::string            _displayName;
    bool                   _isTargetLayer = false;
    LayerType              _layerType = LayerType::SubLayer;
    std::string            _subLayerPath; // name of the layer as it was found in the parent's stack
    bool                   _isIncomingLayer;
    std::set<std::string>  _incomingLayers;
    bool                   _isSharedStage;
    bool                   _isSharedLayer;
    std::set<std::string>  _sharedLayers;

    static LayerActionDefinitions _actionButtons;

protected:
    AbstractCommandHook* commandHook() const;

protected:
    void populateChildren(RecursionDetector* in_recursionDetector);
    // helper to save anon layers called by saveEdits()
    void saveAnonymousLayer(QWidget* in_parent);

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
