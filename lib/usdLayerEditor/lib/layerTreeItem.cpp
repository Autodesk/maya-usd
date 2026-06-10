//
// Copyright 2023 Autodesk
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

#include "layerTreeItem.h"

#include "abstractCommandHook.h"
#include "layerEditorDCCFunctions.h"
#include "layerLocking.h"
#include "layerTreeModel.h"
#include "loadLayersDialog.h"
#include "pathChecker.h"
#include "sessionState.h"
#include "stringResources.h"
#include "utilString.h"
#include "tokens.h"
#include "utilUI.h"
#include "utilFileSystem.h"
#include "utilQT.h"
#include "utilSerialization.h"
#include "warningDialogs.h"

#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerUtils.h>

#if PXR_VERSION >= 2308
#include <pxr/usd/pcp/expressionVariables.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/primIndex.h>
#include <pxr/usd/sdf/variableExpression.h>
#include <pxr/usd/usd/prim.h>
#endif

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

// delegate Action API for command buttons
LayerActionDefinitions LayerTreeItem::_actionButtons;

static void createPixmapPair(const QString& name, QPixmap& normal, QPixmap& hover)
{
    auto utils = getQtUtils();
    normal = utils->createPNGResPixmap(name);
    hover = utils->createPNGResPixmap(name + QString("_hover"));
}

const LayerActionDefinitions& LayerTreeItem::actionButtonsDefinition()
{
    if (_actionButtons.size() == 0) {
        LayerActionInfo muteActionInfo;
        muteActionInfo._name = "Mute Action";
        muteActionInfo._order = 0;
        muteActionInfo._actionType = LayerActionType::Mute;
        muteActionInfo._layerMask = LayerMasks::LayerMasks_SubLayer;
        muteActionInfo._tooltip = StringResources::getAsQString(StringResources::kMuteUnmuteLayer);

        createPixmapPair(
            ":/UsdLayerEditor/LE_mute_off",
            muteActionInfo._pixmap_off,
            muteActionInfo._pixmap_off_hover);
        createPixmapPair(
            ":/UsdLayerEditor/LE_mute_on",
            muteActionInfo._pixmap_on,
            muteActionInfo._pixmap_on_hover);

        _actionButtons.insert(std::make_pair(muteActionInfo._actionType, muteActionInfo));

        LayerActionInfo lockActionInfo;
        lockActionInfo._name = "Lock Action";
        lockActionInfo._order = 1;
        lockActionInfo._actionType = LayerActionType::Lock;
        lockActionInfo._layerMask = static_cast<LayerMasks>(
            LayerMasks::LayerMasks_SubLayer | LayerMasks::LayerMasks_Root);
        lockActionInfo._tooltip = StringResources::getAsQString(StringResources::kLockUnlockLayer);

        createPixmapPair(
            ":/UsdLayerEditor/LE_lock_off",
            lockActionInfo._pixmap_off,
            lockActionInfo._pixmap_off_hover);
        createPixmapPair(
            ":/UsdLayerEditor/LE_lock_on",
            lockActionInfo._pixmap_on,
            lockActionInfo._pixmap_on_hover);

        _actionButtons.insert(std::make_pair(lockActionInfo._actionType, lockActionInfo));
    }
    return _actionButtons;
}

LayerTreeItem::LayerTreeItem(
    SdfLayerRefPtr         in_usdLayer,
    UsdStageRefPtr         in_stage,
    LayerType              in_layerType,
    std::string            in_subLayerPath,
    std::set<std::string>* in_incomingLayers,
    bool                   in_sharedStage,
    std::set<std::string>* in_sharedLayers,
    RecursionDetector*     in_recursionDetector)
    : _layer(std::move(in_usdLayer))
    , _stage(std::move(in_stage))
    , _isTargetLayer(false)
    , _layerType(in_layerType)
    , _subLayerPath(in_subLayerPath)
    , _isIncomingLayer(false)
    , _incomingLayers()
    , _isSharedStage(in_sharedStage)
    , _isSharedLayer(false)
    , _sharedLayers()
{
    if (in_incomingLayers != nullptr) {
        _incomingLayers = *in_incomingLayers;
        if (_layer && _incomingLayers.find(_layer->GetIdentifier()) != _incomingLayers.end()) {
            _isIncomingLayer = true;
        }
    }
    if (in_sharedLayers != nullptr) {
        _sharedLayers = *in_sharedLayers;
        if (_layer && _sharedLayers.find(_layer->GetIdentifier()) != _sharedLayers.end()) {
            _isSharedLayer = true;
        }
    }
    fetchData(RebuildChildren::Yes, in_recursionDetector);
}

// QStandardItem API
int LayerTreeItem::type() const { return QStandardItem::UserType; }

// used by draw delegate: returns how deep in the hierarchy we are
int LayerTreeItem::depth() const
{
    auto parent = parentLayerItem();
    return (parent == nullptr) ? 0 : 1 + parent->depth();
}

// this algorithm works with muted layers
void LayerTreeItem::populateChildren(RecursionDetector* recursionDetector)
{
    removeRows(0, rowCount());
    if (isInvalidLayer())
        return;

    auto subPaths = _layer->GetSubLayerPaths();

    RecursionDetector defaultDetector;
    if (!recursionDetector) {
        recursionDetector = &defaultDetector;
    }
    recursionDetector->push(_layer->GetRealPath());

    for (auto const path : subPaths) {
#if PXR_VERSION >= 2308
        // Resolve any variable expressions in the path using the stage's expression variables,
        // composed from root and session layer variables.
        std::string resolvedPath = path;
        if (_stage && SdfVariableExpression::IsExpression(path)) {
            const auto stageRootLayerStack
                = _stage->GetPseudoRoot().GetPrimIndex().GetRootNode().GetLayerStack();

            if (stageRootLayerStack) {
                const auto& expressionVars
                    = stageRootLayerStack->GetExpressionVariables().GetVariables();

                const auto result
                    = SdfVariableExpression(path).EvaluateTyped<std::string>(expressionVars);

                if (result.errors.empty() && !result.value.IsEmpty()) {
                    resolvedPath = result.value.UncheckedGet<std::string>();
                }
            }
        }

        std::string actualPath = SdfComputeAssetPathRelativeToLayer(_layer, resolvedPath);
        auto        subLayer = SdfLayer::FindOrOpen(actualPath);
#else
        std::string actualPath = SdfComputeAssetPathRelativeToLayer(_layer, path);
        auto        subLayer = SdfLayer::FindOrOpen(actualPath);
#endif
        if (!subLayer || !recursionDetector->contains(subLayer->GetRealPath())) {
            auto item = new LayerTreeItem(
                subLayer,
                _stage,
                LayerType::SubLayer,
                path,
                &_incomingLayers,
                _isSharedStage,
                &_sharedLayers,
                recursionDetector);
            appendRow(item);
        }
    }

    recursionDetector->pop();
}

LayerItemVector LayerTreeItem::childrenVector() const
{
    LayerItemVector result;
    result.reserve(rowCount());
    for (int i = 0; i < rowCount(); i++) {
        result.push_back(dynamic_cast<LayerTreeItem*>(child(i, 0)));
    }
    return result;
}

bool LayerTreeItem::isIdenticalItem(const LayerTreeItem* other) const
{
    if (!other) {
        return false;
    }

    if (this == other) {
        return true;
    }

    if (layer() != other->layer()) {
        return false;
    }

    if (_isSharedStage != other->_isSharedStage) {
        return false;
    }

    auto myChildren = childrenVector();
    auto otherChildren = other->childrenVector();
    if (myChildren.size() != otherChildren.size()) {
        return false;
    }

    for (size_t i = 0; i < myChildren.size(); i++) {
        if (!myChildren[i]->isIdenticalItem(otherChildren[i])) {
            return false;
        }
    }

    return true;
}

// recursively update the target layer data member. Meant to be called from invisibleRoot
void LayerTreeItem::updateTargetLayerRecursive(const PXR_NS::SdfLayerRefPtr& newTargetLayer)
{
    if (!_layer)
        return;
    bool thisLayerIsNowTarget = (_layer == newTargetLayer);
    if (thisLayerIsNowTarget != _isTargetLayer) {
        _isTargetLayer = thisLayerIsNowTarget;
        emitDataChanged();
    }
    for (auto child : childrenVector()) {
        child->updateTargetLayerRecursive(newTargetLayer);
    }
}

void LayerTreeItem::fetchData(RebuildChildren in_rebuild, RecursionDetector* in_recursionDetector)
{
    std::string name;
    if (isSessionLayer()) {
        name = "sessionLayer";
    } else {
        if (isInvalidLayer()) {
            name = _subLayerPath;
        } else {
            name = _layer->GetDisplayName();
            if (name.empty()) {
                name = _layer->GetIdentifier();
            }
        }
    }
    _displayName = name;
    setText(name.c_str());
    if (in_rebuild == RebuildChildren::Yes) {
        populateChildren(in_recursionDetector);
    }
    emitDataChanged();
}

QVariant LayerTreeItem::data(int role) const
{
    switch (role) {
#if QT_DISABLE_DEPRECATED_BEFORE || QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case Qt::ForegroundRole: return QApplication::palette().color(QPalette::ButtonText);
#else
    case Qt::TextColorRole: return QApplication::palette().color(QPalette::ButtonText);
#endif
    case Qt::BackgroundRole: return QApplication::palette().color(QPalette::Window);
    case Qt::TextAlignmentRole:
        return (static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter));
    case Qt::SizeHintRole: return QSize(0, DPIScale(24));
    default: return QStandardItem::data(role);
    }
}

LayerTreeModel* LayerTreeItem::parentModel() const
{
    return dynamic_cast<LayerTreeModel*>(model());
}

AbstractCommandHook* LayerTreeItem::commandHook() const
{
    return parentModel()->sessionState()->commandHook();
}

bool LayerTreeItem::isMuted() const
{
    return isInvalidLayer() || !_stage ? false : _stage->IsLayerMuted(_layer->GetIdentifier());
}

bool LayerTreeItem::appearsMuted() const
{
    if (isMuted()) {
        return true;
    }
    auto item = parentLayerItem();
    while (item != nullptr) {
        if (item->isMuted()) {
            return true;
        }
        item = item->parentLayerItem();
    }
    return false;
}

bool LayerTreeItem::sublayerOfShared() const
{
    auto item = parentLayerItem();
    while (item != nullptr) {
        if (item->_isSharedLayer) {
            return true;
        }
        item = item->parentLayerItem();
    }

    return false;
}

bool LayerTreeItem::isReadOnly() const { return (_isSharedLayer) || sublayerOfShared(); }

bool LayerTreeItem::isMovable() const
{
    // Dragging the root layer, session and muted layer is not allowed.
    return !isSessionLayer() && !isRootLayer() && !appearsMuted() && !sublayerOfShared()
        && !isLocked() && !appearsLocked() && !isSystemLocked() && !appearsSystemLocked();
}

bool LayerTreeItem::isIncoming() const { return _isIncomingLayer; }

bool LayerTreeItem::isLocked() const { return _layer && _layer->PermissionToEdit() == false; }

bool LayerTreeItem::appearsLocked() const
{
    // Note: This is used to indicate that some of the actions
    // cannot be performed on a layer whose parent is locked.
    auto item = parentLayerItem();
    if (item != nullptr) {
        return item->isLocked();
    }

    return false;
}

bool LayerTreeItem::isSystemLocked() const
{
    // When a layer is being externally driven, it should appear as system-locked.
    return isLayerSystemLocked(_layer) || isReadOnly();
}

bool LayerTreeItem::appearsSystemLocked() const
{
    // Note: This is used to indicate that some of the actions cannot
    // be performed on a layer whose parent is system-locked.
    auto item = parentLayerItem();
    if (item != nullptr) {
        return item->isSystemLocked();
    }
    return false;
}

bool LayerTreeItem::hasSubLayers() const
{
    if (!_layer)
        return false;
    return _layer->GetNumSubLayerPaths() > 0;
}

bool LayerTreeItem::needsSaving() const
{
    // If for any reason we don't hold a layer, then we cannot save it.
    if (!_layer)
        return false;

    // Session layers are managed by Maya, not the Layer Editor,
    // so their dirty state does not count.
    if (isSessionLayer())
        return false;

    // The stage is not shared, layers are assumed to be managed
    // somewhere else and do not get saved here.
    if (!_isSharedStage)
        return false;

    return isDirty() || isAnonymous();
}

// delegate Action API for command buttons
void LayerTreeItem::getActionButton(LayerActionType actionType, LayerActionInfo& out_info) const
{
    auto actionButtons = actionButtonsDefinition();
    auto iter = actionButtons.find(actionType);
    if (iter != actionButtons.end()) {
        out_info = iter->second;
        if (actionType == Lock)
            out_info._checked = isLocked();
        else if (actionType == Mute)
            out_info._checked = isMuted();
    }
}

void LayerTreeItem::removeSubLayer()
{
    if (isSublayer()) { // can't remove session or root layer
        commandHook()->removeSubLayerPath(parentLayerItem()->layer(), subLayerPath());
    }
}

void LayerTreeItem::saveEdits()
{
    bool shouldSaveEdits = true;

    // if the current layer contains anonymous layer(s),
    // display a warning and abort the saving operation.
    LayerItemVector anonymLayerItems = parentModel()->getAllAnonymousLayers(this);
    if (!anonymLayerItems.empty()) {
        const std::string titleFormat = StringResources::kSaveLayerWarnTitle.value;
        const std::string msgFormat = StringResources::kSaveLayerSaveNestedAnonymLayer.value;

        std::string title = String::format(titleFormat, displayName().c_str());

        std::string nbAnonymLayer = std::to_string(anonymLayerItems.size());

        std::string msg = String::format(msgFormat, displayName().c_str(), nbAnonymLayer);

        QStringList anonymLayerNames;
        for (auto item : anonymLayerItems) {
            anonymLayerNames.append(item->displayName().c_str());
        }

        warningDialog(
            QString::fromStdString(title), QString::fromStdString(msg), &anonymLayerNames);

        return;
    }

    // the layer is already saved on disk.
    // ask the user a confirmation before overwrite it.
    const bool showConfirmDgl = confirmExistingFileSave();
    if (showConfirmDgl && !isAnonymous()) {
        const std::string titleFormat = StringResources::kSaveLayerWarnTitle.value;
        const std::string msgFormat = StringResources::kSaveLayerWarnMsg.value;

        std::string title = String::format(titleFormat, displayName().c_str());

        std::string msg = String::format(msgFormat, layer()->GetRealPath().c_str());

        QString okButtonText = StringResources::getAsQString(StringResources::kSave);
        shouldSaveEdits = confirmDialog(
            QString::fromStdString(title),
            QString::fromStdString(msg),
            nullptr /*bulletList*/,
            &okButtonText);
    }

    if (shouldSaveEdits) {
        saveEditsNoPrompt();
    }
}

void LayerTreeItem::saveEditsNoPrompt()
{
    if (isAnonymous()) {
        if (!isSessionLayer())
            saveAnonymousLayer();
    } else {
        if (!Serialization::saveLayerWithFormat(layer())) {
            std::string layerName(layer()->GetDisplayName().c_str());
            std::string errMsg = String::format("Could not save layer ^1s.", layerName);
            UIUtils::displayError(errMsg);
        }
    }
}

// helper to save anon layers called by saveEdits()
void LayerTreeItem::saveAnonymousLayer()
{
    // Special case for components created by the component creator. Only the
    // component creator knows how to save a component properly; delegate to the
    // stage save. The predicate is false for DCCs without component support.
    if (SessionState* ss = parentModel()->sessionState()) {
        if (UsdLayerEditor::isStageAComponent(ss->stageEntry()._dccObjectPath)) {
            parentModel()->saveStage(nullptr);
            return;
        }
    }

     SessionState* sessionState = parentModel()->sessionState();

    // the path we have is an absolute path
     std::string fileName;
     if (!sessionState->saveLayerUI(nullptr, &fileName, parentLayer()))
         return;

     Serialization::ensureUSDFileExtension(fileName);

     const QString dialogTitle = StringResources::getAsQString(StringResources::kSaveLayer);

     if (!checkIfPathIsSafeToAdd(dialogTitle, parentLayerItem(), fileName))
         return;

     Serialization::PathInfo pathInfo;
     pathInfo.absolutePath = fileName;
     pathInfo.savePathAsRelative = isRootLayer()
         ? FileSystem::requireUsdPathsRelativeToDCCSceneFile()
         : FileSystem::requireUsdPathsRelativeToParentLayer();
     pathInfo.customRelativeAnchor = ""; // TODO, see calculateParentLayerDir()

     Serialization::LayerParent layerParent;
     layerParent._layerParent = parentLayer();
     layerParent._objectPath = sessionState->stageEntry()._dccObjectPath;

     std::string    errMsg;
     std::string    formatTag = Serialization::usdFormatArgOption();
     SdfLayerRefPtr newLayer = Serialization::saveAnonymousLayer(
         sessionState->stage(), layer(), pathInfo, layerParent, formatTag, &errMsg);
     if (!newLayer) {
         warningDialog(dialogTitle, errMsg.c_str());
         return;
     }

     const std::string absoluteFileName = fileName;

    // now replace the layer in the parent
     if (isRootLayer())
         sessionState->rootLayerPathChanged(fileName);

     if (auto model = parentModel())
         model->selectUsdLayerOnIdle(newLayer);
}

void LayerTreeItem::discardEdits()
{
    if (isAnonymous() || !isDirty()) {
        // according to MAYA-104336, we don't prompt for confirmation for anonymous layers
        // according to EMSUSD-964, we don't prompt for confirmation if the layer is not dirty
        commandHook()->discardEdits(layer());
    } else {
        std::string title
            = String::format(StringResources::kReloadTitle.value, text().toStdString());

        std::string desc
            = String::format(StringResources::kReloadMsg.value, text().toStdString());

        const QString buttonText = QString::fromStdString(StringResources::kReloadButtonText.value);

        if (confirmDialog(
                QString::fromStdString(title),
                QString::fromStdString(desc),
                nullptr,
                &buttonText
                )) {
            commandHook()->discardEdits(layer());
        }
    }
}

void LayerTreeItem::addAnonymousSublayer()
{
    addAnonymousSublayerAndReturn();
}

PXR_NS::SdfLayerRefPtr LayerTreeItem::addAnonymousSublayerAndReturn()
{
    UndoContext context(commandHook(), "Add Anonymous Layer");
    auto model = parentModel();
    auto newLayer
        = commandHook()->addAnonymousSubLayer(layer(), model->findNameForNewAnonymousLayer());
    model->selectUsdLayerOnIdle(newLayer);
    return newLayer;
}

void LayerTreeItem::loadSubLayers(QWidget* in_parent)
{
     LoadLayersDialog dlg(this, in_parent);
     dlg.exec();
     if (dlg.pathsToLoad().size() > 0) {
         const int   index = 0;
         UndoContext context(commandHook(), "Load Layers");
         for (const auto& path : dlg.pathsToLoad()) {
             context.hook()->insertSubLayerPath(layer(), path, index);

            if (FileSystem::requireUsdPathsRelativeToParentLayer()) {
                if (layer()->IsAnonymous()) {
                    FileSystem::markPathAsPostponedRelative(layer(), path);
                }
            } else {
                FileSystem::unmarkPathAsPostponedRelative(layer(), path);
            }
        }
        context.hook()->refreshLayerSystemLock(layer(), true);
    }
}

void LayerTreeItem::printLayer()
{
    if (!isInvalidLayer()) {
        parentModel()->sessionState()->printLayer(layer());
    }
}

void LayerTreeItem::clearLayer() { commandHook()->clearLayer(layer()); }

void LayerTreeItem::mergeWithSublayers()
{
    if (!_layer || isInvalidLayer() || !hasSubLayers() || isLocked())
        return;

    commandHook()->flattenLayer(_layer);
}

UsdLayerEditor::LayerMasks CreateLayerMask(bool isRootLayer, bool isSubLayer, bool isSessionLayer)
{
    LayerMasks mask = LayerMasks::LayerMasks_None;
    if (isRootLayer)
        mask = mask | LayerMasks::LayerMasks_Root;
    if (isSubLayer)
        mask = mask | LayerMasks::LayerMasks_SubLayer;
    if (isSessionLayer)
        mask = mask | LayerMasks::LayerMasks_Session;
    return mask;
}

LayerMasks operator|(LayerMasks lhs, LayerMasks rhs)
{
    return static_cast<LayerMasks>(unsigned(lhs) | unsigned(rhs));
}

bool IsLayerActionAllowed(const LayerActionInfo& actionInfo, LayerMasks layerMaskFlag)
{
    return (actionInfo._layerMask & layerMaskFlag) != 0;
}

} // namespace UsdLayerEditor
