#include "layerTreeItem.h"

#include "abstractCommandHook.h"
#include "layerTreeModel.h"
#include "loadLayersDialog.h"
#include "pathChecker.h"
#include "qtUtils.h"
#include "sessionState.h"
#include "stringResources.h"
#include "warningDialogs.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/undo/MayaUsdUndoBlock.h>
#include <mayaUsd/utils/layerLocking.h>
#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/flattenUtils.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#if PXR_VERSION >= 2308
#include <pxr/usd/sdf/variableExpression.h>
#endif

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

// delegate Action API for command buttons
LayerActionDefinitions LayerTreeItem::_actionButtons;

static void createPixmapPair(const QString& name, QPixmap& normal, QPixmap& hover)
{
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
            ":/UsdLayerEditor/mute_off",
            muteActionInfo._pixmap_off,
            muteActionInfo._pixmap_off_hover);
        createPixmapPair(
            ":/UsdLayerEditor/mute_on", muteActionInfo._pixmap_on, muteActionInfo._pixmap_on_hover);

        _actionButtons.insert(std::make_pair(muteActionInfo._actionType, muteActionInfo));

        LayerActionInfo lockActionInfo;
        lockActionInfo._name = "Lock Action";
        lockActionInfo._order = 1;
        lockActionInfo._actionType = LayerActionType::Lock;
        lockActionInfo._layerMask = static_cast<LayerMasks>(
            LayerMasks::LayerMasks_SubLayer | LayerMasks::LayerMasks_Root);
        lockActionInfo._tooltip = StringResources::getAsQString(StringResources::kLockUnlockLayer);

        createPixmapPair(
            ":/UsdLayerEditor/lock_off",
            lockActionInfo._pixmap_off,
            lockActionInfo._pixmap_off_hover);
        createPixmapPair(
            ":/UsdLayerEditor/lock_on", lockActionInfo._pixmap_on, lockActionInfo._pixmap_on_hover);

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
        // Resolve any variable expressions in the path using the stage's expression variables
        std::string resolvedPath = path;
        if (_stage && SdfVariableExpression::IsExpression(path)) {
            auto resolveExprVarsFromLayer =
                [](SdfVariableExpression& varExpr, SdfLayerRefPtr fromLayer, std::string& outPath) {
                    if (fromLayer && fromLayer->HasExpressionVariables()) {
                        auto expressionVars = fromLayer->GetExpressionVariables();
                        auto result = varExpr.Evaluate(expressionVars);
                        if (result.errors.empty() && !result.value.IsEmpty()) {
                            outPath = result.value.UncheckedGet<std::string>();
                        }
                    }
                };

            SdfVariableExpression varExpr(path);
            // Get the root layer's expression variables for resolution context
            auto rootLayer = _stage->GetRootLayer();
            resolveExprVarsFromLayer(varExpr, rootLayer, resolvedPath);

            // Expression variables are composed across session layer and root
            // layer of a stage. So we do another pass with the session layer
            // to override/set the resolvedPath in case it is present in the
            // session layer
            auto sessionLayer = _stage->GetSessionLayer();
            resolveExprVarsFromLayer(varExpr, sessionLayer, resolvedPath);
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
    case Qt::ForegroundRole: return QColor(200, 200, 200);
#else
    case Qt::TextColorRole: return QColor(200, 200, 200);
#endif
    case Qt::BackgroundRole: return QColor(71, 71, 71);
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
    return MayaUsd::isLayerSystemLocked(_layer) || isReadOnly();
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

bool LayerTreeItem::isAnonymous() const
{
    if (SessionState* sessionState = parentModel()->sessionState()) {
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(
                sessionState->stageEntry()._proxyShapePath)) {
            return MayaUsd::ComponentUtils::isUnsavedAdskUsdComponent(sessionState->stage());
        }
    }

    return _layer ? _layer->IsAnonymous() : false;
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

void LayerTreeItem::removeSubLayer(QWidget* /*in_parent*/)
{
    if (isSublayer()) { // can't remove session or root layer
        commandHook()->removeSubLayerPath(parentLayerItem()->layer(), subLayerPath());
    }
}

void LayerTreeItem::saveEdits(QWidget* in_parent)
{
    // Special case for components created by the component creator. Only the component creator
    // knows how to save a component properly.
    if (SessionState* sessionState = parentModel()->sessionState()) {
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(
                sessionState->stageEntry()._proxyShapePath)) {
            parentModel()->saveStage(in_parent);
            return;
        }
    }

    bool shouldSaveEdits = true;

    // if the current layer contains anonymous layer(s),
    // display a warning and abort the saving operation.
    LayerItemVector anonymLayerItems = parentModel()->getAllAnonymousLayers(this);
    if (!anonymLayerItems.empty()) {
        const MString titleFormat
            = StringResources::getAsMString(StringResources::kSaveLayerWarnTitle);
        const MString msgFormat
            = StringResources::getAsMString(StringResources::kSaveLayerSaveNestedAnonymLayer);

        MString title;
        title.format(titleFormat, displayName().c_str());

        MString nbAnonymLayer;
        nbAnonymLayer += static_cast<unsigned int>(anonymLayerItems.size());

        MString msg;
        msg.format(msgFormat, displayName().c_str(), nbAnonymLayer);

        QStringList anonymLayerNames;
        for (auto item : anonymLayerItems) {
            anonymLayerNames.append(item->displayName().c_str());
        }

        warningDialog(
            in_parent, MQtUtil::toQString(title), MQtUtil::toQString(msg), &anonymLayerNames);
        return;
    }

    // the layer is already saved on disk.
    // ask the user a confirmation before overwrite it.
    static const MString kConfirmExistingFileSave
        = MayaUsdOptionVars->ConfirmExistingFileSave.GetText();
    const bool showConfirmDgl = MGlobal::optionVarExists(kConfirmExistingFileSave)
        && MGlobal::optionVarIntValue(kConfirmExistingFileSave) != 0;
    if (showConfirmDgl && !isAnonymous()) {
        const MString titleFormat
            = StringResources::getAsMString(StringResources::kSaveLayerWarnTitle);
        const MString msgFormat = StringResources::getAsMString(StringResources::kSaveLayerWarnMsg);

        MString title;
        title.format(titleFormat, displayName().c_str());

        MString msg;
        msg.format(msgFormat, layer()->GetRealPath().c_str());

        QString okButtonText = StringResources::getAsQString(StringResources::kSave);
        shouldSaveEdits = confirmDialog(
            in_parent,
            MQtUtil::toQString(title),
            MQtUtil::toQString(msg),
            nullptr /*bulletList*/,
            &okButtonText);
    }

    if (shouldSaveEdits) {
        saveEditsNoPrompt(in_parent);
    }
}

void LayerTreeItem::saveEditsNoPrompt(QWidget* in_parent)
{
    // Special case for components created by the component creator. Only the component creator
    // knows how to save a component properly.
    if (SessionState* sessionState = parentModel()->sessionState()) {
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(
                sessionState->stageEntry()._proxyShapePath)) {
            parentModel()->saveStage(in_parent);
            return;
        }
    }

    if (isAnonymous()) {
        if (!isSessionLayer())
            saveAnonymousLayer(in_parent);
    } else {
        if (!MayaUsd::utils::saveLayerWithFormat(layer())) {
            MString errMsg;
            MString layerName(layer()->GetDisplayName().c_str());
            errMsg.format("Could not save layer ^1s.", layerName);
            MGlobal::displayError(errMsg);
        }
    }
}

// helper to save anon layers called by saveEdits()
void LayerTreeItem::saveAnonymousLayer(QWidget* in_parent)
{
    // Special case for components created by the component creator. Only the component creator
    // knows how to save a component properly.
    if (SessionState* sessionState = parentModel()->sessionState()) {
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(
                sessionState->stageEntry()._proxyShapePath)) {
            parentModel()->saveStage(in_parent);
            return;
        }
    }

    SessionState* sessionState = parentModel()->sessionState();

    // the path we have is an absolute path
    std::string fileName;
    if (!sessionState->saveLayerUI(in_parent, &fileName, parentLayer()))
        return;

    MayaUsd::utils::ensureUSDFileExtension(fileName);

    const QString dialogTitle = StringResources::getAsQString(StringResources::kSaveLayer);

    if (!checkIfPathIsSafeToAdd(in_parent, dialogTitle, parentLayerItem(), fileName))
        return;

    MayaUsd::utils::PathInfo pathInfo;
    pathInfo.absolutePath = fileName;
    pathInfo.savePathAsRelative = isRootLayer()
        ? UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile()
        : UsdMayaUtilFileSystem::requireUsdPathsRelativeToParentLayer();
    pathInfo.customRelativeAnchor = ""; // TODO, see calculateParentLayerDir()

    MayaUsd::utils::LayerParent layerParent;
    layerParent._layerParent = parentLayer();
    layerParent._proxyPath = sessionState->stageEntry()._proxyShapePath;

    std::string    errMsg;
    std::string    formatTag = MayaUsd::utils::usdFormatArgOption();
    SdfLayerRefPtr newLayer = MayaUsd::utils::saveAnonymousLayer(
        sessionState->stage(), layer(), pathInfo, layerParent, formatTag, &errMsg);
    if (!newLayer) {
        warningDialog(in_parent, dialogTitle, errMsg.c_str());
        return;
    }

    const std::string absoluteFileName = fileName;

    // now replace the layer in the parent
    if (isRootLayer())
        sessionState->rootLayerPathChanged(fileName);

    if (auto model = parentModel())
        model->selectUsdLayerOnIdle(newLayer);
}

void LayerTreeItem::discardEdits(QWidget* in_parent)
{
    bool confirmed = false;

    if (isAnonymous() || !isDirty()) {
        // according to MAYA-104336, we don't prompt for confirmation for anonymous layers
        // according to EMSUSD-964, we don't prompt for confirmation if the layer is not dirty
        confirmed = true;
    } else {
        MString title;
        title.format(
            StringResources::getAsMString(StringResources::kRevertToFileTitle),
            MQtUtil::toMString(text()));

        MString desc;
        desc.format(
            StringResources::getAsMString(StringResources::kRevertToFileMsg),
            MQtUtil::toMString(text()));

        confirmed = confirmDialog(in_parent, MQtUtil::toQString(title), MQtUtil::toQString(desc));
    }

    if (!confirmed)
        return;

    // Special case for components created by the component creator. Only the component creator
    // knows how to save a component properly.
    if (SessionState* sessionState = parentModel()->sessionState()) {
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(
                sessionState->stageEntry()._proxyShapePath)) {
            parentModel()->reloadComponent(in_parent);
            return;
        }
    }

    commandHook()->discardEdits(layer());
}

void LayerTreeItem::addAnonymousSublayer(QWidget* in_parent)
{
    //
    addAnonymousSublayerAndReturn(in_parent);
}

PXR_NS::SdfLayerRefPtr LayerTreeItem::addAnonymousSublayerAndReturn(QWidget* /*in_parent*/)
{
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

            if (UsdMayaUtilFileSystem::requireUsdPathsRelativeToParentLayer()) {
                if (layer()->IsAnonymous()) {
                    UsdMayaUtilFileSystem::markPathAsPostponedRelative(layer(), path);
                }
            } else {
                UsdMayaUtilFileSystem::unmarkPathAsPostponedRelative(layer(), path);
            }
        }
        context.hook()->refreshLayerSystemLock(layer(), true);
    }
}

void LayerTreeItem::printLayer(QWidget* /*in_parent*/)
{
    if (!isInvalidLayer()) {
        parentModel()->sessionState()->printLayer(layer());
    }
}

void LayerTreeItem::clearLayer(QWidget* /*in_parent*/) { commandHook()->clearLayer(layer()); }

void LayerTreeItem::mergeWithSublayers(QWidget* /*in_parent*/)
{
    if (!_layer || isInvalidLayer())
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
