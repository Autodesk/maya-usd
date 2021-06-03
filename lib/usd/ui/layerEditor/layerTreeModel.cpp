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

#include "layerTreeModel.h"

#include "layerEditorWidget.h"
#include "layerTreeItem.h"
#include "saveLayersDialog.h"
#include "stringResources.h"
#include "warningDialogs.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/customLayerData.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/base/tf/notice.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <QtCore/QTimer>

#include <algorithm>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
// drag and drop support
// For now just use the plain text type to store the layer identifiers.
const QString LAYER_EDITOR_MIME_TYPE = QStringLiteral("text/plain");
const QString LAYED_EDITOR_MIME_SEP = QStringLiteral(";");

} // namespace

namespace UsdLayerEditor {

bool LayerTreeModel::_blockUsdNotices = false;

void LayerTreeModel::suspendUsdNotices(bool suspend)
{
    //
    _blockUsdNotices = suspend;
}

LayerTreeModel::LayerTreeModel(SessionState* in_sessionState, QObject* in_parent)
    : QStandardItemModel(in_parent)
{
    setSessionState(in_sessionState);
    registerUsdNotifications(true);
}

LayerTreeModel::~LayerTreeModel()
{
    //
    registerUsdNotifications(false);
}

void LayerTreeModel::registerUsdNotifications(bool in_register)
{
    if (in_register) {
        TfWeakPtr<LayerTreeModel> me(this);
        _noticeKeys.push_back(TfNotice::Register(me, &LayerTreeModel::usd_layerChanged));
        _noticeKeys.push_back(TfNotice::Register(me, &LayerTreeModel::usd_editTargetChanged));
        _noticeKeys.push_back(TfNotice::Register(
            me, &LayerTreeModel::usd_layerDirtinessChanged, TfWeakPtr<SdfLayer>(nullptr)));

    } else {
        TfNotice::Revoke(&_noticeKeys);
    }
}

Qt::ItemFlags LayerTreeModel::flags(const QModelIndex& index) const
{
    // set flags for drag and drop support
    auto item = layerItemFromIndex(index);
    if (!item || item->isInvalidLayer()) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    Qt::ItemFlags defaultFlags = QStandardItemModel::flags(index);
    if (index.isValid() && item->isMovable()) {
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    } else {
        defaultFlags &= ~Qt::ItemIsDragEnabled;
        return Qt::ItemIsDropEnabled | defaultFlags;
    }
}

Qt::DropActions LayerTreeModel::supportedDropActions() const
{
    // We support only moving layers around to reorder or re-parent.
    return Qt::MoveAction;
}

QStringList LayerTreeModel::mimeTypes() const
{
    // Just return our supported type (i.e. not appending it to the current)
    // list of supported types. This way our base class(es) won't perform
    // any drop actions.
    return QStringList(LAYER_EDITOR_MIME_TYPE);
}

QMimeData* LayerTreeModel::mimeData(const QModelIndexList& indexes) const
{
    // Returns an object that contains serialized items of data corresponding
    // to the list of indexes specified.

    // Prepare the entries to move. For now we just store the layer identifiers
    // as a string separated by special character.
    auto        mimeData = new QMimeData();
    QStringList identifiers;
    for (auto index : indexes) {
        auto item = layerItemFromIndex(index);
        if (item->isInvalidLayer()) {
            identifiers += item->subLayerPath().c_str();
        } else {
            identifiers += item->layer()->GetIdentifier().c_str();
        }
    }
    mimeData->setText(identifiers.join(LAYED_EDITOR_MIME_SEP));
    return mimeData;
}

bool LayerTreeModel::dropMimeData(
    const QMimeData*   in_mimeData,
    Qt::DropAction     action,
    int                row,
    int                column,
    const QModelIndex& parentIndex)
{
    // Handles the data supplied by a drag and drop operation that ended with the given action.

    // Check if the action is supported?
    if (!in_mimeData || (action != Qt::MoveAction)) {
        return false;
    }
    if (!in_mimeData->hasFormat(LAYER_EDITOR_MIME_TYPE)) {
        return false;
    }

    auto parentItem = layerItemFromIndex(parentIndex);
    if (!parentItem || parentItem->isReadOnly()) {
        return false;
    }

    // row is -1 when dropped on a parent item and not between rows.
    // In that case we want to insert at row 0 (first child).
    if (row == -1) {
        row = 0;
    }

    // Parse the mime data that was passed in to get the list of layers.
    // We parse it in reversed order since when we insert the original
    // order is maintained.
    QStringList identifiers = in_mimeData->text().split(LAYED_EDITOR_MIME_SEP);
    std::reverse(identifiers.begin(), identifiers.end());
    UndoContext context(_sessionState->commandHook(), "Drop USD Layers");
    for (QString const& ident : identifiers) {
        auto layer = SdfLayer::FindOrOpen(ident.toStdString());
        if (layer) {
            auto layerItem = findUSDLayerItem(layer);
            if (layerItem) {
                auto oldParent = layerItem->parentLayerItem()->layer();
                int  index = (int)oldParent->GetSubLayerPaths().Find(layerItem->subLayerPath());
                auto itemSubLayerPath = layerItem->subLayerPath();
                context.hook()->removeSubLayerPath(oldParent, itemSubLayerPath);
                // When we are moving an item (underneath the same parent)
                // to a new location higher up we have to adjust the row
                // (new location) to account for the remove we just did.
                if (oldParent == parentItem->layer() && (index < row)) {
                    row -= 1;
                }
                context.hook()->insertSubLayerPath(parentItem->layer(), itemSubLayerPath, row);
            }
        }
    }
    return true;
}

void LayerTreeModel::setEditTarget(LayerTreeItem* item)
{
    if (!item->appearsMuted() && !item->isReadOnly()) {
        UndoContext context(_sessionState->commandHook(), "Set USD Edit Target Layer");
        context.hook()->setEditTarget(item->layer());
    }
}

void LayerTreeModel::selectUsdLayerOnIdle(const SdfLayerRefPtr& usdLayer)
{
    QTimer::singleShot(0, this, [this, usdLayer]() {
        auto item = findUSDLayerItem(usdLayer);
        if (item != nullptr) {
            auto index = indexFromItem(item);
            Q_EMIT selectLayerSignal(index);
        }
    });
}

QModelIndex LayerTreeModel::rootLayerIndex()
{
    auto root = invisibleRootItem();
    for (int row = 0; row < root->rowCount(); row++) {
        auto child = dynamic_cast<LayerTreeItem*>(root->child(row));
        if (child->isRootLayer()) {
            return index(row, 0);
        }
    }
    return QModelIndex();
}

void LayerTreeModel::setSessionState(SessionState* in_sessionState)
{
    _sessionState = in_sessionState;
    connect(
        in_sessionState,
        &SessionState::currentStageChangedSignal,
        this,
        &LayerTreeModel::sessionStageChanged);

    connect(
        in_sessionState,
        &SessionState::autoHideSessionLayerSignal,
        this,
        &LayerTreeModel::autoHideSessionLayerChanged);
}

void LayerTreeModel::rebuildModelOnIdle()
{
    if (!_rebuildOnIdlePending) {
        _rebuildOnIdlePending = true;
        QTimer::singleShot(0, this, &LayerTreeModel::rebuildModel);
    }
}

void LayerTreeModel::rebuildModel()
{
    _rebuildOnIdlePending = false;
    _lastAskedAnonLayerNameSinceRebuild = 0;

    beginResetModel();
    clear();

    if (_sessionState->isValid()) {
        auto rootLayer = _sessionState->stage()->GetRootLayer();
        bool showSessionLayer = true;
        auto sessionLayer = _sessionState->stage()->GetSessionLayer();
        if (_sessionState->autoHideSessionLayer()) {
            showSessionLayer
                = sessionLayer->IsDirty() || sessionLayer == _sessionState->targetLayer();
        }

        std::set<std::string> sharedLayers;
        auto                  sharedStage = _sessionState->commandHook()->isProxyShapeSharedStage(
            _sessionState->stageEntry()._proxyShapePath);
        if (!sharedStage) {
            auto layers = MayaUsd::CustomLayerData::getStringArray(
                rootLayer, MayaUsd::CustomLayerData::kReferencedLayersToken);
            std::vector<std::string> layerIds;
            std::move(layers.begin(), layers.end(), inserter(layerIds, layerIds.begin()));
            sharedLayers = UsdMayaUtil::getAllSublayers(layerIds, true);
        }

        std::set<std::string> incomingLayers;
        if (_sessionState->commandHook()->isProxyShapeStageIncoming(
                _sessionState->stageEntry()._proxyShapePath)) {
            if (!sharedStage) {
                incomingLayers = sharedLayers;
            } else {
                std::vector<std::string> layerIds;
                layerIds.push_back(rootLayer->GetIdentifier());
                incomingLayers = UsdMayaUtil::getAllSublayers(layerIds, true);
            }
        }

        if (showSessionLayer) {
            appendRow(new LayerTreeItem(
                sessionLayer,
                LayerType::SessionLayer,
                "",
                &incomingLayers,
                sharedStage,
                &sharedLayers));
        }

        appendRow(new LayerTreeItem(
            rootLayer, LayerType::RootLayer, "", &incomingLayers, sharedStage, &sharedLayers));

        updateTargetLayer(InRebuildModel::Yes);
    }

    endResetModel();
}

LayerTreeItem* LayerTreeModel::findUSDLayerItem(const SdfLayerRefPtr& usdLayer) const
{
    const auto allItems = getAllItems();
    for (auto item : allItems) {
        if (item->layer() == usdLayer)
            return item;
    }
    return nullptr;
}

void LayerTreeModel::updateTargetLayer(InRebuildModel inRebuild)
{
    if (rowCount() == 0) {
        return;
    }

    auto editTarget = _sessionState->targetLayer();
    auto root = invisibleRootItem();

    // if session layer is in auto-hide handle case where it is the target
    if (inRebuild == InRebuildModel::No && _sessionState->autoHideSessionLayer()) {
        bool needToRebuild = false;
        auto firstLayerItem = dynamic_cast<LayerTreeItem*>(root->child(0));
        // if session layer is no longer the target layer, we need to rebuild to hide it
        if (firstLayerItem->isSessionLayer() && firstLayerItem->isTargetLayer()) {
            needToRebuild = firstLayerItem->layer() != editTarget;
        }
        // if the new target is the session layer
        if (editTarget == _sessionState->stage()->GetSessionLayer()) {
            needToRebuild = true;
        }
        if (needToRebuild) {
            rebuildModelOnIdle();
            return;
        }
    }

    // all other cases, just update the icon
    for (int i = 0, count = root->rowCount(); i < count; i++) {
        auto child = dynamic_cast<LayerTreeItem*>(root->child(i));
        child->updateTargetLayerRecursive(editTarget);
    }
}

// notification from USD
void LayerTreeModel::usd_layerChanged(SdfNotice::LayersDidChangeSentPerLayer const& notice)
{
    // experienced crashes in python prototype  For now, rebuild everything
    if (!_blockUsdNotices)
        rebuildModelOnIdle();
}

// notification from USD
void LayerTreeModel::usd_editTargetChanged(UsdNotice::StageEditTargetChanged const& notice)
{
    if (!_blockUsdNotices) {
        QTimer::singleShot(
            0, dynamic_cast<QObject*>(this), [this]() { updateTargetLayer(InRebuildModel::No); });
    }
}

// notification from USD
void LayerTreeModel::usd_layerDirtinessChanged(
    SdfNotice::LayerDirtinessChanged const& notice,
    const TfWeakPtr<SdfLayer>&              layer)
{
    if (!_blockUsdNotices) {
        auto layerItem = findUSDLayerItem(layer);
        if (layerItem) {
            layerItem->fetchData(RebuildChildren::No);
        }
    }
}

// called from SessionState::currentStageChangedSignal
void LayerTreeModel::sessionStageChanged() { rebuildModel(); }

// called from SessionState::autoHideSessionLayerSignal
void LayerTreeModel::autoHideSessionLayerChanged() { rebuildModelOnIdle(); }

LayerTreeItem* LayerTreeModel::layerItemFromIndex(const QModelIndex& index) const
{
    return dynamic_cast<LayerTreeItem*>(itemFromIndex(index));
}

void layerItemVectorRecurs(
    LayerTreeItem*                parent,
    LayerTreeModel::ConditionFunc filter,
    std::vector<LayerTreeItem*>&  result)
{
    if (filter(parent)) {
        result.push_back(parent);
    }
    if (parent->rowCount() > 0) {
        const auto& children = parent->childrenVector();
        for (auto const& child : children)
            layerItemVectorRecurs(child, filter, result);
    }
}

LayerItemVector
LayerTreeModel::getAllItems(ConditionFunc filter, const LayerTreeItem* item /* = nullptr*/) const
{

    LayerItemVector result;
    auto            root = item ? item : invisibleRootItem();
    for (int i = 0, count = root->rowCount(); i < count; i++) {
        auto child = dynamic_cast<LayerTreeItem*>(root->child(i));
        layerItemVectorRecurs(child, filter, result);
    }
    return result;
}

LayerItemVector LayerTreeModel::getAllNeedsSavingLayers() const
{
    auto filter = [](const LayerTreeItem* item) { return item->needsSaving(); };
    return getAllItems(filter);
}

LayerItemVector
LayerTreeModel::getAllAnonymousLayers(const LayerTreeItem* item /* = nullptr*/) const
{
    auto filter
        = [](const LayerTreeItem* item) { return item->isAnonymous() && !item->isSessionLayer(); };
    return getAllItems(filter, item);
}

void LayerTreeModel::saveStage(QWidget* in_parent)
{
    auto saveAllLayers = [this]() {
        const auto layers = getAllNeedsSavingLayers();
        for (auto layer : layers) {
            if (!layer->isAnonymous())
                layer->saveEditsNoPrompt();
        }
    };

    static const MString kConfirmExistingFileSave
        = MayaUsdOptionVars->ConfirmExistingFileSave.GetText();
    bool showConfirmDgl = MGlobal::optionVarExists(kConfirmExistingFileSave)
        && MGlobal::optionVarIntValue(kConfirmExistingFileSave) != 0;

    // if the stage contains anonymous layers, you need to show the comfirm dialog
    // so the user can choose where to save the anonymous layers.
    if (!showConfirmDgl) {
        // Get the layers to save for this stage.
        MayaUsd::utils::stageLayersToSave stageLayersToSave;
        auto&                             stageEntry = _sessionState->stageEntry();
        MayaUsd::utils::getLayersToSaveFromProxy(stageEntry._proxyShapePath, stageLayersToSave);
        showConfirmDgl = !stageLayersToSave._anonLayers.empty();
    }

    if (showConfirmDgl) {
        SaveLayersDialog dlg(_sessionState, in_parent);
        if (QDialog::Accepted == dlg.exec()) {

            if (!dlg.layersWithErrorPairs().isEmpty()) {
                const QStringList& errors = dlg.layersWithErrorPairs();
                MString            resultMsg;
                for (int i = 0; i < errors.length() - 1; i += 2) {
                    MString errorMsg;
                    errorMsg.format(
                        StringResources::getAsMString(StringResources::kSaveAnonymousLayersErrors),
                        MQtUtil::toMString(errors[i]),
                        MQtUtil::toMString(errors[i + 1]));
                    resultMsg += errorMsg + "\n";
                }

                MGlobal::displayError(resultMsg);

                warningDialog(
                    StringResources::getAsQString(StringResources::kSaveAnonymousLayersErrorsTitle),
                    StringResources::getAsQString(StringResources::kSaveAnonymousLayersErrorsMsg));
            } else {
                saveAllLayers();
            }
        }
    } else {
        saveAllLayers();
    }
}

std::string LayerTreeModel::findNameForNewAnonymousLayer() const
{
    const std::string prefix = "anonymousLayer";
    size_t            prefix_len = prefix.size();
    int               largest = _lastAskedAnonLayerNameSinceRebuild;
    int               suffix_int;
    std::string       name, suffix_string;

    // find the largest number we have
    const auto allItems = getAllItems();
    for (const auto& item : allItems) {
        name = item->displayName();
        if (name.compare(0, prefix_len, prefix) == 0) {
            suffix_string = name.substr(prefix_len);
            suffix_int = std::stoi(suffix_string);
            if (suffix_int > largest)
                largest = suffix_int;
        }
    }

    _lastAskedAnonLayerNameSinceRebuild = largest + 1;
    return prefix + std::to_string(largest + 1);
}

void LayerTreeModel::toggleMuteLayer(LayerTreeItem* item, bool* forcedState)
{
    if (item->isInvalidLayer() || !item->isSublayer())
        return;

    if (forcedState) {
        if (*forcedState == item->isMuted())
            return;
    }

    _sessionState->commandHook()->muteSubLayer(item->layer(), !item->isMuted());
}

} // namespace UsdLayerEditor
