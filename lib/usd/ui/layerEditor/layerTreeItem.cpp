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
    case Qt::TextColorRole: return QColor(200, 200, 200);
    case Qt::BackgroundRole: return QColor(71, 71, 71);
    case Qt::TextAlignmentRole: return Qt::AlignLeft + Qt::AlignVCenter;
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
    return isInvalidLayer() ? false : stage()->IsLayerMuted(_layer->GetIdentifier());
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
    if (_layer) {
        if (!isSessionLayer() && !isReadOnly()
            && !(!_incomingLayers.empty() && parentLayerItem() == nullptr)) {
            return isDirty() || isAnonymous();
        }
    }
    return false;
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
        layer()->Save();
    }
}

// helper to save anon layers called by saveEdits()
void LayerTreeItem::saveAnonymousLayer()
{
    auto sessionState = parentModel()->sessionState();

    std::string fileName;
    if (sessionState->saveLayerUI(nullptr, &fileName)) {
        // the path we has is an absolute path
        const QString dialogTitle = StringResources::getAsQString(StringResources::kSaveLayer);
        std::string   formatTag = MayaUsd::utils::usdFormatArgOption();
        if (saveSubLayer(dialogTitle, parentLayerItem(), layer(), fileName, formatTag)) {
            printf("USD Layer written to %s\n", fileName.c_str());

            // now replace the layer in the parent
            if (isRootLayer()) {
                sessionState->rootLayerPathChanged(fileName);
            } else {
                // now replace the layer in the parent
                auto parentItem = parentLayerItem();
                auto newLayer = SdfLayer::FindOrOpen(fileName);
                if (newLayer) {
                    bool setTarget = _isTargetLayer;
                    auto model = parentModel();
                    parentItem->layer()->GetSubLayerPaths().Replace(
                        layer()->GetIdentifier(), newLayer->GetIdentifier());
                    if (setTarget) {
                        sessionState->stage()->SetEditTarget(newLayer);
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
    if (isAnonymous()) {
        // according to MAYA-104336, we don't prompt for confirmation for anonymous layers
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
        for (auto path : dlg.pathsToLoad()) {
            context.hook()->insertSubLayerPath(layer(), path, index);
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
