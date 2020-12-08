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

#ifndef LAYERTREEMODEL_H
#define LAYERTREEMODEL_H

#include "sessionState.h"

#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/sdf/declareHandles.h>
#include <pxr/usd/sdf/notice.h>
#include <pxr/usd/usd/notice.h>

#include <QtGui/QStandardItemModel>

#include <string>
#include <vector>

namespace UsdLayerEditor {

class LayerTreeItem;
class SessionState;

typedef std::vector<LayerTreeItem*> LayerItemVector;

enum class InRebuildModel
{
    Yes,
    No,
};

/**
 * @brief Implements the Qt data model for the usd layer tree view
 *
 */
class LayerTreeModel
    : public QStandardItemModel
    , public pxr::TfWeakBase
{
    Q_OBJECT
public:
    LayerTreeModel(SessionState* in_sessionState, QObject* in_parent);
    ~LayerTreeModel();
    SessionState* sessionState() { return _sessionState; }

    // API to suspend reacting to USD notifications
    static void suspendUsdNotices(bool suspend);

    // get propertly typed item
    LayerTreeItem* layerItemFromIndex(const QModelIndex& index) const;
    // gets everything recursivly as an array : used to simplify iteration
    typedef bool (*ConditionFunc)(const LayerTreeItem*);
    LayerItemVector getAllItems(ConditionFunc filter = [](const LayerTreeItem*) {
        return true;
    }) const;
    // get all the layers that need saving
    LayerItemVector getAllNeedsSavingLayers() const;
    // get all anonymous layers except the session layer
    LayerItemVector getAllAnonymousLayers() const;

    // get an appropriate name for a new anonymous layer
    std::string findNameForNewAnonymousLayer() const;

    // save stage UI
    void saveStage();

    // return the index of the root layer
    QModelIndex rootLayerIndex();

    // target button callbacks
    void setEditTarget(LayerTreeItem* item);

    // mute layer management
    void toggleMuteLayer(LayerTreeItem* item, bool* forcedState = nullptr);

    // ask to select a layer in the near future
    void selectUsdLayerOnIdle(const pxr::SdfLayerRefPtr& usdLayer);

    // drag and drop support
    Qt::ItemFlags   flags(const QModelIndex& index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList     mimeTypes() const override;
    QMimeData*      mimeData(const QModelIndexList& indexes) const override;
    bool            dropMimeData(
                   const QMimeData*   data,
                   Qt::DropAction     action,
                   int                row,
                   int                column,
                   const QModelIndex& parent) override;

    // for debugging
    void forceRefresh() { rebuildModelOnIdle(); }

Q_SIGNALS:
    void selectLayerSignal(const QModelIndex&);

protected:
    // slots
    void sessionStageChanged();
    void autoHideSessionLayerChanged();

    void          setSessionState(SessionState* in_sessionState);
    SessionState* _sessionState = nullptr;

    void registerUsdNotifications(bool in_register);
    void usd_layerChanged(pxr::SdfNotice::LayersDidChangeSentPerLayer const& notice);
    void usd_editTargetChanged(pxr::UsdNotice::StageEditTargetChanged const& notice);
    void usd_layerDirtinessChanged(
        pxr::SdfNotice::LayerDirtinessChanged const& notice,
        const pxr::TfWeakPtr<pxr::SdfLayer>&         layer);

    pxr::TfNotice::Keys _noticeKeys;
    static bool         _blockUsdNotices;
    ;

    mutable int _lastAskedAnonLayerNameSinceRebuild = 0;

    void rebuildModelOnIdle();
    bool _rebuildOnIdlePending = false;
    void rebuildModel();

    void updateTargetLayer(InRebuildModel inRebuild);

    LayerTreeItem* findUSDLayerItem(const pxr::SdfLayerRefPtr& usdLayer) const;
};

} // namespace UsdLayerEditor
#endif // LAYERTREEMODEL_H
