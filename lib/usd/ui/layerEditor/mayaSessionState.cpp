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

#include "mayaSessionState.h"

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
#include <saveLayersDialog.h>
#include <stringResources.h>
#else
#include "saveLayersDialog.h"
#include "stringResources.h"
#endif

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/nodes/usdPrimProvider.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilSerialization.h>

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
#include <mayaUsd/editForward/MayaUsdEditForwardHost.h>

#include <AdskUsdEditForward/Host.h>
#include <AdskUsdEditForward/StageRuleProvider.h>
#endif

#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/sceneItem.h>
#include <ufe/selection.h>

#include <maya/MDGMessage.h>
#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MNodeMessage.h>
#include <maya/MPxNode.h>
#include <maya/MSceneMessage.h>
#include <maya/MUuid.h>

#include <QtCore/QTimer>
#include <QtWidgets/QMenu>

#ifdef THIS
#undef THIS
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
MString PROXY_NODE_TYPE = "mayaUsdProxyShapeBase";
MString AUTO_HIDE_OPTION_VAR
    = UsdMayaUtil::convert(MayaUsdOptionVars->LayerEditorAutoHideSessionLayer);
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
MString ECHO_EDIT_FORWARDING_OPTION_VAR
    = UsdMayaUtil::convert(MayaUsdOptionVars->LayerEditorEchoEditForwarding);
#endif
MString DISPLAY_LAYER_CONTENTS_OPTION_VAR
    = UsdMayaUtil::convert(MayaUsdOptionVars->LayerEditorDisplayLayerContents);
MString DISPLAY_LAYER_EXPAND_ALL_VALUES_OPTION_VAR
    = UsdMayaUtil::convert(MayaUsdOptionVars->LayerEditorExpandAllValues);
} // namespace

namespace UsdLayerEditor {

MayaSessionState::MayaSessionState()
    : _mayaCommandHook(this)
{
    if (MGlobal::optionVarExists(AUTO_HIDE_OPTION_VAR)) {
        _autoHideSessionLayer = MGlobal::optionVarIntValue(AUTO_HIDE_OPTION_VAR) != 0;
    }
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
    if (MGlobal::optionVarExists(ECHO_EDIT_FORWARDING_OPTION_VAR)) {
        _echoEditForwarding = MGlobal::optionVarIntValue(ECHO_EDIT_FORWARDING_OPTION_VAR) != 0;
    }
#endif
    if (MGlobal::optionVarExists(DISPLAY_LAYER_CONTENTS_OPTION_VAR)) {
        _displayLayerContents = MGlobal::optionVarIntValue(DISPLAY_LAYER_CONTENTS_OPTION_VAR) != 0;
    }
    if (MGlobal::optionVarExists(DISPLAY_LAYER_EXPAND_ALL_VALUES_OPTION_VAR)) {
        _displayLayerExpandAllValues
            = MGlobal::optionVarIntValue(DISPLAY_LAYER_EXPAND_ALL_VALUES_OPTION_VAR) != 0;
    }

    registerNotifications();
}

MayaSessionState::~MayaSessionState()
{
    try {
        unregisterNotifications();
    } catch (const std::exception&) {
        // Ignore errors in destructor.
    }
}

void MayaSessionState::setStageEntry(StageEntry const& inEntry)
{
    PARENT_CLASS::setStageEntry(inEntry);
    if (!inEntry._stage) {
        _currentStageEntry.clear();
    }

    if (!_inLoad)
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
        MayaUsd::LayerManager::setSelectedStage(_currentStageEntry._dccObjectPath);
#else
        MayaUsd::LayerManager::setSelectedStage(_currentStageEntry._proxyShapePath);
#endif
}

bool MayaSessionState::getStageEntry(StageEntry* out_stageEntry, const MString& shapePath)
{
    UsdPrim prim;

    MObject shapeObj;
    MStatus status = UsdMayaUtil::GetMObjectByName(shapePath, shapeObj);
    if (!status)
        return false;
    MFnDagNode dagNode(shapeObj, &status);
    if (!status)
        return false;

    if (const UsdMayaUsdPrimProvider* usdPrimProvider
        = dynamic_cast<const UsdMayaUsdPrimProvider*>(dagNode.userNode())) {
        prim = usdPrimProvider->usdPrim();
    }

    if (prim) {
        auto stage = prim.GetStage();
        // debatable, but we remove the path|to|shape
        auto    tokenList = QString(shapePath.asChar()).split("|");
        QString niceName;

        if (tokenList.length() > 1) {
            niceName = tokenList[tokenList.length() - 1];
        } else {
            niceName = tokenList[0];
        }
        out_stageEntry->_id = dagNode.uuid().asString().asChar();
        out_stageEntry->_stage = stage;
        out_stageEntry->_displayName = niceName.toStdString();
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
        out_stageEntry->_dccObjectPath = shapePath.asChar();
#else
        out_stageEntry->_proxyShapePath = shapePath.asChar();
#endif
        return true;
    }
    return false;
}

std::vector<SessionState::StageEntry> MayaSessionState::allStages() const
{
    std::vector<StageEntry> stages;

    // Iterate through all shape DAG nodes to find proxy shape nodes
    MItDag dagIterator(MItDag::kDepthFirst, MFn::kPluginShape);
    for (; !dagIterator.isDone(); dagIterator.next()) {
        MObject    mobj = dagIterator.currentItem();
        MFnDagNode fnDagNode(mobj);

        const PXR_NS::MayaUsdProxyShapeBase* proxyShape
            = dynamic_cast<const PXR_NS::MayaUsdProxyShapeBase*>(fnDagNode.userNode());
        if (!proxyShape)
            continue;

        // Check if this node is a proxy shape by type name
        MDagPath dagPath;
        dagIterator.getPath(dagPath);

        // Avoid instances of the same shape by only looking at the first instance (instance number
        // 0)
        if (dagPath.instanceNumber() != 0) {
            continue;
        }

        MString    shapePath = dagPath.fullPathName();
        StageEntry entry;
        if (getStageEntry(&entry, shapePath)) {
            stages.push_back(entry);
        }
    }

    std::sort(stages.begin(), stages.end(), [](const StageEntry& a, const StageEntry& b) {
        return a._displayName < b._displayName;
    });
    return stages;
}

// API implementation
AbstractCommandHook* MayaSessionState::commandHook() { return &_mayaCommandHook; }

void MayaSessionState::registerNotifications()
{
    MCallbackId id;

    MayaUsd::MayaNodeTypeObserver& proxyObserver
        = PXR_NS::MayaUsdProxyShapeBase::getProxyShapesObserver();
    proxyObserver.addTypeListener(*this);

    id = MNodeMessage::addNameChangedCallback(
        MObject::kNullObj, MayaSessionState::nodeRenamedCB, this);
    _callbackIds.push_back(id);

    id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeOpen, MayaSessionState::sceneClosingCB, this);
    _callbackIds.push_back(id);

    id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeNew, MayaSessionState::sceneClosingCB, this);
    _callbackIds.push_back(id);

    id = MSceneMessage::addCallback(
        MSceneMessage::kAfterOpen, MayaSessionState::sceneLoadedCB, this);
    _callbackIds.push_back(id);

    id = MSceneMessage::addCallback(
        MSceneMessage::kAfterNew, MayaSessionState::sceneLoadedCB, this);
    _callbackIds.push_back(id);

    id = MSceneMessage::addNamespaceRenamedCallback(MayaSessionState::namespaceRenamedCB, this);
    _callbackIds.push_back(id);

    TfWeakPtr<MayaSessionState> me(this);
    _stageResetNoticeKey = TfNotice::Register(me, &MayaSessionState::mayaUsdStageReset);

    loadSelectedStage();
}

void MayaSessionState::unregisterNotifications()
{
    MayaUsd::MayaNodeTypeObserver& proxyObserver
        = PXR_NS::MayaUsdProxyShapeBase::getProxyShapesObserver();
    proxyObserver.removeTypeListener(*this);

    for (auto id : _callbackIds) {
        MMessage::removeCallback(id);
    }
    _callbackIds.clear();

    TfNotice::Revoke(_stageResetNoticeKey);
}

void MayaSessionState::refreshCurrentStageEntry()
{
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    refreshStageEntry(_currentStageEntry._dccObjectPath);
#else
    refreshStageEntry(_currentStageEntry._proxyShapePath);
#endif
}

void MayaSessionState::refreshStageEntry(std::string const& proxyShapePath)
{
    StageEntry entry;
    if (getStageEntry(&entry, proxyShapePath.c_str())) {
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
        if (entry._dccObjectPath == _currentStageEntry._dccObjectPath) {
#else
        if (entry._proxyShapePath == _currentStageEntry._proxyShapePath) {
#endif
            QTimer::singleShot(0, this, [this, entry]() {
                mayaUsdStageResetCBOnIdle(entry);
                setStageEntry(entry);
            });
        } else {
            QTimer::singleShot(0, this, [this, entry]() { mayaUsdStageResetCBOnIdle(entry); });
        }
    }
}

void MayaSessionState::mayaUsdStageReset(const MayaUsdProxyStageSetNotice& notice)
{
    refreshStageEntry(notice.GetShapePath());
}

void MayaSessionState::mayaUsdStageResetCBOnIdle(StageEntry const& entry)
{
    Q_EMIT stageResetSignal(entry);
}

void MayaSessionState::processNodeAdded(MObject& node)
{
    // doing it on idle give time to the Load Stage to set a file name
    QTimer::singleShot(0, [self = this, node]() { self->proxyShapeAddedCBOnIdle(node); });
}

void MayaSessionState::proxyShapeAddedCBOnIdle(const MObject& obj)
{
    if (MFileIO::isNewingFile())
        return;

    // doing it on idle give time to the Load Stage to set a file name
    // but we don't do a second idle because we could get a delete right after a Add
    MDagPath   dagPath;
    MFnDagNode dagNode;
    if (!dagNode.setObject(obj))
        return;

    dagNode.getPath(dagPath);
    auto       shapePath = dagPath.fullPathName();
    StageEntry entry;
    if (getStageEntry(&entry, shapePath)) {
        Q_EMIT stageListChangedSignal(entry);
    }
}

void MayaSessionState::processNodeRemoved(MObject& /*node*/)
{
    QTimer::singleShot(0, [self = this]() { self->stageListChangedSignal(); });
}

/* static */
void MayaSessionState::namespaceRenamedCB(
    const MString& oldName,
    const MString& newName,
    void*          clientData)
{
    if (oldName.length() != 0) {
        auto THIS = static_cast<MayaSessionState*>(clientData);
        for (StageEntry& entry : THIS->allStages()) {
            // Need to update the current Entry also
            if (THIS->_currentStageEntry._id == entry._id) {
                THIS->_currentStageEntry = entry;
            }
            Q_EMIT THIS->stageRenamedSignal(entry);
        }
    }
}

/* static */
void MayaSessionState::nodeRenamedCB(MObject& obj, const MString& oldName, void* clientData)
{
    if (oldName.length() != 0) {
        if (obj.hasFn(MFn::kShape)) {
            MDagPath dagPath;
            MFnDagNode(obj).getPath(dagPath);
            const MString shapePath = dagPath.fullPathName();

            auto THIS = static_cast<MayaSessionState*>(clientData);

            // doing it on idle give time to the Load Stage to set a file name
            QTimer::singleShot(0, [THIS, shapePath]() { THIS->nodeRenamedCBOnIdle(shapePath); });
        }
    }
}

void MayaSessionState::nodeRenamedCBOnIdle(const MString& shapePath)
{
    // this does not work:
    //        if OpenMaya.MFnDependencyNode(obj).typeName == PROXY_NODE_TYPE

    StageEntry entry;
    if (getStageEntry(&entry, shapePath)) {
        // Need to update the current Entry also
        if (_currentStageEntry._id == entry._id) {
            _currentStageEntry = entry;
        }

        Q_EMIT stageRenamedSignal(entry);
    }
}

/* static */
void MayaSessionState::sceneClosingCB(void* clientData)
{
    auto THIS = static_cast<MayaSessionState*>(clientData);
    THIS->_inLoad = true;
    Q_EMIT THIS->clearUIOnSceneResetSignal();
}

/* static */
void MayaSessionState::sceneLoadedCB(void* clientData)
{
    auto THIS = static_cast<MayaSessionState*>(clientData);
    THIS->loadSelectedStage();
    THIS->_inLoad = false;
}

void MayaSessionState::loadSelectedStage()
{
    const std::string shapePath = MayaUsd::LayerManager::getSelectedStage(nullptr);
    StageEntry        entry;
    if (!shapePath.empty() && getStageEntry(&entry, shapePath.c_str())) {
        setStageEntry(entry);
    }
}

bool MayaSessionState::saveLayerUI(
    QWidget*                      in_parent,
    std::string*                  out_filePath,
    const PXR_NS::SdfLayerRefPtr& parentLayer) const
{
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    // Shared SaveLayersDialog takes a parent-layer file path (not an SdfLayer).
    std::string parentLayerPath;
    if (parentLayer) {
        parentLayerPath = parentLayer->GetRealPath();
        if (parentLayerPath.empty())
            parentLayerPath = parentLayer->GetIdentifier();
    }
    return SaveLayersDialog::saveLayerFilePathUI(*out_filePath, parentLayerPath);
#else
    return SaveLayersDialog::saveLayerFilePathUI(*out_filePath, parentLayer);
#endif
}

std::vector<std::string>
MayaSessionState::loadLayersUI(const QString& in_title, const std::string& in_default_path) const
{
    // opens a dialog to return a list of paths to load ui that returns a list of paths to load
    QString defaultPath(in_default_path.c_str());
    defaultPath.replace("\\", "\\\\");
    MString mayaDefaultPath(defaultPath.toStdString().c_str());

    MString mayaTitle(in_title.toStdString().c_str());
    MString script;
    script.format(
        MString("UsdLayerEditor_LoadLayersFileDialog(\"^1s\", \"^2s\")"),
        mayaTitle,
        mayaDefaultPath);

    MStringArray files;
    MGlobal::executeCommand(
        script,
        files,
        /*display*/ true,
        /*undo*/ false);
    if (files.length() == 0)
        return std::vector<std::string>();
    else {
        std::vector<std::string> results;
        for (uint32_t i = 0; i < files.length(); i++) {
            const auto& file = files[i];
            results.push_back(file.asChar());
        }
        return results;
    }
}

void MayaSessionState::setupCreateMenu(QMenu* in_menu)
{
    MString menuName = "UsdLayerEditorCreateMenu";
    in_menu->setObjectName(menuName.asChar());

    MString script;
    script.format("setParent -menu ^1s;", menuName);
    script += "menuItem -runTimeCommand mayaUsdCreateStageWithNewLayer;";
    script += "menuItem -runTimeCommand mayaUsdCreateStageFromFile;";
    script += "menuItem -runTimeCommand mayaUsdCreateStageFromFileOptions -optionBox true;";
    MGlobal::executeCommand(
        script,
        /*display*/ false,
        /*undo*/ false);
}

const char* getCurrentSaveAsFolderScript = R"(
global proc string MayaSessionState_GetCurrentSaveAsFolder()
{
    string $sceneFolder = dirname(`file -q -sceneName`);
    if ("" == $sceneFolder)
    {
        string $workspaceLocation = `workspace -q -fn`;
        string $scenesFolder = `workspace -q -fileRuleEntry "scene"`;
        $sceneFolder = $workspaceLocation + "/" + $scenesFolder;
    }
    return $sceneFolder;
}
MayaSessionState_GetCurrentSaveAsFolder;
)";

// path to default load layer dialogs to
std::string MayaSessionState::defaultLoadPath() const
{
    MString sceneName;
    MGlobal::executeCommand(
        getCurrentSaveAsFolderScript,
        sceneName,
        /*display*/ false,
        /*undo*/ false);
    return sceneName.asChar();
}

// called when an anonymous root layer has been saved to a file
// in this case, the stage needs to be re-created on the new file
void MayaSessionState::rootLayerPathChanged(std::string const& in_path)
{
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    const std::string& proxyPath = _currentStageEntry._dccObjectPath;
#else
    const std::string& proxyPath = _currentStageEntry._proxyShapePath;
#endif
    if (!proxyPath.empty()) {
        MString proxyShape(proxyPath.c_str());
        MString newValue(in_path.c_str());
        MayaUsd::utils::setNewProxyPath(
            proxyShape, newValue, MayaUsd::utils::kProxyPathFollowProxyShape, nullptr, false);
    }
}

void MayaSessionState::setAutoHideSessionLayer(bool hideIt)
{
    int value = hideIt ? 1 : 0;
    MGlobal::setOptionVarValue(AUTO_HIDE_OPTION_VAR, value);
    PARENT_CLASS::setAutoHideSessionLayer(hideIt);
}

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
void MayaSessionState::setEchoEditForwarding(bool echo)
{
    MGlobal::setOptionVarValue(ECHO_EDIT_FORWARDING_OPTION_VAR, echo ? 1 : 0);
    if (auto host = std::dynamic_pointer_cast<MayaUsdEditForwardHost>(
            AdskUsdEditForward::Host::GetInstance())) {
        host->SetWantsEcho(echo);
    }
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    _echoEditForwarding = echo;
#else
    PARENT_CLASS::setEchoEditForwarding(echo);
#endif
}

bool MayaSessionState::isEditForwardMode() const
{
    const auto& stage = stageEntry()._stage;
    if (!stage)
        return false;
    auto controller = MayaUsdEditForwardController::GetForStage(stage);
    return controller && controller->isForwardingActive();
}

PXR_NS::SdfLayerRefPtr MayaSessionState::effectiveTargetLayer() const
{
    const auto& stage = stageEntry()._stage;
    if (stage) {
        auto controller = MayaUsdEditForwardController::GetForStage(stage);
        if (controller && controller->isForwardingActive()) {
            // In EF mode the stage edit target is pinned to the session layer; the meaningful
            // target is the fallback. Fall through to the stage edit target if it is not set.
            if (auto fallback = controller->fallbackTarget())
                return fallback;
            MGlobal::displayWarning("Edit forwarding is active but no fallback target is set.");
        }
    }
    return targetLayer();
}
#endif

void MayaSessionState::setDisplayLayerContents(bool showIt)
{
    int value = showIt ? 1 : 0;
    MGlobal::setOptionVarValue(DISPLAY_LAYER_CONTENTS_OPTION_VAR, value);
    PARENT_CLASS::setDisplayLayerContents(showIt);
}

void MayaSessionState::setDisplayLayerExpandAllValues(bool expand)
{
    int value = expand ? 1 : 0;
    MGlobal::setOptionVarValue(DISPLAY_LAYER_EXPAND_ALL_VALUES_OPTION_VAR, value);
    PARENT_CLASS::setDisplayLayerExpandAllValues(expand);
}

void MayaSessionState::printLayer(const PXR_NS::SdfLayerRefPtr& layer) const
{
    MString result, temp;

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    // The shared StringResources does not provide a getAsMString helper; the
    // Resource struct carries the raw format string as a std::string.
    temp.format(
        MString(StringResources::kUsdLayerIdentifier.value.c_str()),
        layer->GetIdentifier().c_str());
#else
    temp.format(
        StringResources::getAsMString(StringResources::kUsdLayerIdentifier),
        layer->GetIdentifier().c_str());
#endif
    result += temp;
    result += "\n";
    if (layer->GetRealPath() != layer->GetIdentifier()) {
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
        temp.format(
            MString(StringResources::kRealPath.value.c_str()), layer->GetRealPath().c_str());
#else
        temp.format(
            StringResources::getAsMString(StringResources::kRealPath),
            layer->GetRealPath().c_str());
#endif
        result += temp;
        result += "\n";
    }
    std::string text;
    layer->ExportToString(&text);
    result += text.c_str();
    MGlobal::displayInfo(result);
}

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
// -----------------------------------------------------------------------------
// Shared-API overrides
// -----------------------------------------------------------------------------

std::vector<SessionState::StageEntry> MayaSessionState::selectedStages() const
{
    std::vector<StageEntry> result;

    const Ufe::GlobalSelection::Ptr& ufeGlobalSelection = Ufe::GlobalSelection::get();
    if (!ufeGlobalSelection)
        return result;

    // Find the proxy shapes corresponding to UFE selected items. If a selected
    // item is not a proxy shape itself, also peek at its hierarchy children
    // for a contained proxy shape (matches legacy stageSelectorWidget behavior).
    const std::vector<StageEntry> all = allStages();
    auto                          findEntryById = [&](const std::string& id) -> const StageEntry* {
        for (const auto& e : all) {
            if (e._id == id)
                return &e;
        }
        return nullptr;
    };

    const Ufe::Selection& ufeSelection = *ufeGlobalSelection;
    const bool            rebuildCacheIfNeeded = false;
    for (const auto& item : ufeSelection) {
        PXR_NS::MayaUsdProxyShapeBase* proxyShapePtr
            = MayaUsd::ufe::getProxyShape(item->path(), rebuildCacheIfNeeded);
        if (!proxyShapePtr) {
            // Walk the immediate children to find an embedded proxy shape.
            if (auto hierarchy = Ufe::Hierarchy::hierarchy(item)) {
                for (const auto& subItem : hierarchy->children()) {
                    auto p = MayaUsd::ufe::getProxyShape(subItem->path(), rebuildCacheIfNeeded);
                    if (p) {
                        proxyShapePtr = p;
                        break;
                    }
                }
            }
        }
        if (!proxyShapePtr)
            continue;

        MFnDagNode        dagNode(proxyShapePtr->thisMObject());
        const std::string id = dagNode.uuid().asString().asChar();
        if (const StageEntry* entry = findEntryById(id)) {
            result.push_back(*entry);
        }
    }
    return result;
}

bool MayaSessionState::supportsEditForwarding() const
{
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
    return true;
#else
    return false;
#endif
}

bool MayaSessionState::hasEditForwarding() const
{
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
    auto stage = _currentStageEntry._stage;
    if (!stage)
        return false;
    AdskUsdEditForward::StageRuleProvider provider(stage);
    return !provider.GetRules().empty();
#else
    return false;
#endif
}

bool MayaSessionState::echoEditForwarding() const { return _echoEditForwarding; }

bool MayaSessionState::isStageAComponent(const std::string& dccObjectPath) const
{
    if (dccObjectPath.empty())
        return false;
    return MayaUsd::ComponentUtils::isAdskUsdComponent(dccObjectPath);
}

bool MayaSessionState::isUnsavedComponent(const PXR_NS::UsdStageRefPtr& stage) const
{
    return MayaUsd::ComponentUtils::isUnsavedAdskUsdComponent(stage);
}

bool MayaSessionState::shouldDisplayComponentInitialSaveDialog(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            dccObjectPath) const
{
    return MayaUsd::ComponentUtils::shouldDisplayComponentInitialSaveDialog(stage, dccObjectPath);
}

std::string MayaSessionState::sceneFolder() const { return MayaUsd::utils::getSceneFolder(); }

std::string MayaSessionState::moveComponent(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& dccObjectPath)
{
    return MayaUsd::ComponentUtils::moveAdskUsdComponent(
        saveLocation, componentName, dccObjectPath);
}

std::string MayaSessionState::previewComponentSave(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& dccObjectPath) const
{
    return MayaUsd::ComponentUtils::previewSaveAdskUsdComponent(
        saveLocation, componentName, dccObjectPath);
}

std::vector<std::string>
MayaSessionState::getComponentLayersToSave(const std::string& dccObjectPath) const
{
    return MayaUsd::ComponentUtils::getAdskUsdComponentLayersToSave(dccObjectPath);
}

#endif // MAYAUSD_USE_SHARED_LAYER_EDITOR

} // namespace UsdLayerEditor
