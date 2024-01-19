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
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerUtils.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

// delegate Action API for command buttons
std::vector<LayerActionInfo> LayerTreeItem::_actionButtons;

const std::vector<LayerActionInfo>& LayerTreeItem::actionButtonsDefinition()
{
    if (_actionButtons.size() == 0) {
        LayerActionInfo info;
        info._name = "Mute Action";
        info._tooltip = StringResources::getAsQString(StringResources::kMuteUnmuteLayer);
        info._pixmap = utils->createPNGResPixmap("RS_disable");
        _actionButtons.push_back(info);
    }
    return _actionButtons;
}

LayerTreeItem::LayerTreeItem(
    SdfLayerRefPtr         in_usdLayer,
    LayerType              in_layerType,
    std::string            in_subLayerPath,
    std::set<std::string>* in_incomingLayers,
    bool                   in_sharedStage,
    std::set<std::string>* in_sharedLayers,
    RecursionDetector*     in_recursionDetector)
    : _layer(std::move(in_usdLayer))
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
        std::string actualPath = SdfComputeAssetPathRelativeToLayer(_layer, path);
        auto        subLayer = SdfLayer::FindOrOpen(actualPath);
        if (subLayer) {
            if (recursionDetector->contains(subLayer->GetRealPath())) {
                MString msg;
                msg.format(
                    StringResources::getAsMString(StringResources::kErrorRecursionDetected),
                    subLayer->GetRealPath().c_str());
                puts(msg.asChar());
            } else {
                auto item = new LayerTreeItem(
                    subLayer,
                    LayerType::SubLayer,
                    path,
                    &_incomingLayers,
                    _isSharedStage,
                    &_sharedLayers,
                    recursionDetector);
                appendRow(item);
            }
        } else {
            MString msg;
            msg.format(
                StringResources::getAsMString(StringResources::kErrorDidNotFind),
                std::string(path).c_str());
            puts(msg.asChar());
            auto item = new LayerTreeItem(
                subLayer,
                LayerType::SubLayer,
                path,
                &_incomingLayers,
                _isSharedStage,
                &_sharedLayers);
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case Qt::ForegroundRole: return QColor(200, 200, 200);
#else
    case Qt::TextColorRole: return QColor(200, 200, 200);
#endif
    case Qt::BackgroundRole: return QColor(71, 71, 71);
    case Qt::TextAlignmentRole:
        return (static_cast<int>(Qt::AlignLeft) + static_cast<int>(Qt::AlignVCenter));
    case Qt::SizeHintRole: return QSize(0, DPIScale(30));
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

PXR_NS::UsdStageRefPtr const& LayerTreeItem::stage() const
{
    return parentModel()->sessionState()->stage();
}

bool LayerTreeItem::isMuted() const
{
    return isInvalidLayer() || !stage() ? false : stage()->IsLayerMuted(_layer->GetIdentifier());
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

bool LayerTreeItem::isReadOnly() const
{
    return (_isSharedLayer || (_layer && !_layer->PermissionToEdit()));
}

bool LayerTreeItem::isMovable() const
{
    // Dragging the root layer, session and muted layer is not allowed.
    return !isSessionLayer() && !isRootLayer() && !appearsMuted() && !sublayerOfShared();
}

bool LayerTreeItem::isIncoming() const { return _isIncomingLayer; }

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
void LayerTreeItem::getActionButton(int index, LayerActionInfo* out_info) const
{
    *out_info = actionButtonsDefinition()[0];
    out_info->_checked = isMuted();
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

        warningDialog(MQtUtil::toQString(title), MQtUtil::toQString(msg), &anonymLayerNames);
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
            MQtUtil::toQString(title),
            MQtUtil::toQString(msg),
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
        if (!MayaUsd::utils::saveLayerWithFormat(layer())) {
            MString errMsg;
            MString layerName(layer()->GetDisplayName().c_str());
            errMsg.format("Could not save layer ^1s.", layerName);
            MGlobal::displayError(errMsg);
        }
    }
}

// helper to save anon layers called by saveEdits()
void LayerTreeItem::saveAnonymousLayer()
{
    // TODO: the code below is very similar to mayaUsd::utils::saveAnonymousLayer().
    //       When fixing bug here or there, we need to fix it in the other. Refactor to have a
    //       single copy.

    auto sessionState = parentModel()->sessionState();

    std::string fileName;
    if (sessionState->saveLayerUI(nullptr, &fileName, parentLayer())) {

        MayaUsd::utils::ensureUSDFileExtension(fileName);

        // the path we have is an absolute path
        const QString dialogTitle = StringResources::getAsQString(StringResources::kSaveLayer);
        std::string   formatTag = MayaUsd::utils::usdFormatArgOption();
        if (saveSubLayer(dialogTitle, parentLayerItem(), layer(), fileName, formatTag)) {

            const std::string absoluteFileName = fileName;

            // now replace the layer in the parent
            if (isRootLayer()) {
                if (UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile()) {
                    fileName = UsdMayaUtilFileSystem::getPathRelativeToMayaSceneFile(fileName);
                }
                sessionState->rootLayerPathChanged(fileName);
            } else {
                auto parentItem = parentLayerItem();
                auto parentLayer = parentItem ? parentItem->layer() : nullptr;
                if (parentLayer) {
                    if (UsdMayaUtilFileSystem::requireUsdPathsRelativeToParentLayer()) {
                        if (!parentLayer->IsAnonymous()) {
                            fileName = UsdMayaUtilFileSystem::getPathRelativeToLayerFile(
                                fileName, parentLayer);
                        } else {
                            UsdMayaUtilFileSystem::markPathAsPostponedRelative(
                                parentLayer, fileName);
                        }
                    } else {
                        UsdMayaUtilFileSystem::unmarkPathAsPostponedRelative(parentLayer, fileName);
                    }
                }

                // Note: we need to open the layer with the absolute path. The relative path is only
                //       used by the parent layer to refer to the sub-layer relative to itself. When
                //       opening the layer in isolation, we need to use the absolute path. Failure
                //       to do so will make finding the layer by its own identifier fail! A symptom
                //       of this failure is that drag-and-drop in the Layer Manager UI fails
                //       immediately after saving a layer with a relative path.
                SdfLayerRefPtr newLayer = SdfLayer::FindOrOpen(absoluteFileName);

                // Now replace the layer in the parent, using a relative path if requested.
                if (newLayer) {
                    bool setTarget = _isTargetLayer;
                    auto model = parentModel();
                    MayaUsd::utils::updateSubLayer(parentItem->layer(), layer(), fileName);
                    if (setTarget) {
                        auto stage = sessionState->stage();
                        if (stage) {
                            stage->SetEditTarget(newLayer);
                        }
                    }
                    model->selectUsdLayerOnIdle(newLayer);
                } else {
                    QMessageBox::critical(
                        nullptr,
                        dialogTitle,
                        StringResources::getAsQString(StringResources::kErrorFailedToReloadLayer));
                }
            }
        }
    }
}
void LayerTreeItem::discardEdits()
{
    if (isAnonymous() || !isDirty()) {
        // according to MAYA-104336, we don't prompt for confirmation for anonymous layers
        // according to EMSUSD-964, we don't prompt for confirmation if the layer is not dirty
        commandHook()->discardEdits(layer());
    } else {
        MString title;
        title.format(
            StringResources::getAsMString(StringResources::kRevertToFileTitle),
            MQtUtil::toMString(text()));

        MString desc;
        desc.format(
            StringResources::getAsMString(StringResources::kRevertToFileMsg),
            MQtUtil::toMString(text()));

        if (confirmDialog(MQtUtil::toQString(title), MQtUtil::toQString(desc))) {
            commandHook()->discardEdits(layer());
        }
    }
}

void LayerTreeItem::addAnonymousSublayer()
{
    //
    addAnonymousSublayerAndReturn();
}

PXR_NS::SdfLayerRefPtr LayerTreeItem::addAnonymousSublayerAndReturn()
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
    }
}

void LayerTreeItem::printLayer()
{
    if (!isInvalidLayer()) {
        parentModel()->sessionState()->printLayer(layer());
    }
}

void LayerTreeItem::clearLayer() { commandHook()->clearLayer(layer()); }

} // namespace UsdLayerEditor
