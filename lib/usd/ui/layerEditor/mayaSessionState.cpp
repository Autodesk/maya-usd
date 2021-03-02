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

#include "saveLayersDialog.h"
#include "stringResources.h"

#include <mayaUsd/utils/query.h>
#include <mayaUsd/utils/util.h>

#include <maya/MDGMessage.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MSceneMessage.h>

#include <QtCore/QTimer>
#include <QtWidgets/QMenu>


#ifdef THIS
#undef THIS
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
MString PROXY_NODE_TYPE = "mayaUsdProxyShapeBase";
MString AUTO_HIDE_OPTION_VAR = "MayaUSDLayerEditor_AutoHideSessionLayer";
} // namespace

namespace UsdLayerEditor {
using namespace MAYAUSD_NS_DEF;

MayaSessionState::MayaSessionState()
    : _mayaCommandHook(this)
{
    if (MGlobal::optionVarExists(AUTO_HIDE_OPTION_VAR)) {
        _autoHideSessionLayer = MGlobal::optionVarIntValue(AUTO_HIDE_OPTION_VAR) != 0;
    }
}

MayaSessionState::~MayaSessionState()
{
    //
}

void MayaSessionState::setStageEntry(StageEntry const& inEntry)
{
    PARENT_CLASS::setStageEntry(inEntry);
    if (!inEntry._stage) {
        _currentStageEntry.clear();
    }
}

bool MayaSessionState::getStageEntry(StageEntry* out_stageEntry, const MString& shapePath)
{
    auto prim = UsdMayaQuery::GetPrim(shapePath.asChar());
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
        out_stageEntry->_stage = stage;
        out_stageEntry->_displayName = niceName.toStdString();
        out_stageEntry->_proxyShapePath = shapePath.asChar();
        return true;
    }
    return false;
}

std::vector<SessionState::StageEntry> MayaSessionState::allStages() const
{
    std::vector<StageEntry> stages;
    MStringArray            shapes;
    MGlobal::executeCommand(
        MString("ls -long -type ") + PROXY_NODE_TYPE, shapes, /*display*/ false, /*undo*/ false);
    StageEntry entry;
    for (unsigned i = 0; i < shapes.length(); ++i) {
        if (getStageEntry(&entry, shapes[i])) {
            stages.push_back(entry);
        }
    }
    return stages;
}

// API implementation
AbstractCommandHook* MayaSessionState::commandHook() { return &_mayaCommandHook; }

void MayaSessionState::registerNotifications()
{
    MCallbackId id;

    id = MDGMessage::addNodeAddedCallback(
        MayaSessionState::proxyShapeAddedCB, PROXY_NODE_TYPE, this);
    _callbackIds.push_back(id);

    id = MDGMessage::addNodeRemovedCallback(
        MayaSessionState::proxyShapeRemovedCB, PROXY_NODE_TYPE, this);
    _callbackIds.push_back(id);

    id = MNodeMessage::addNameChangedCallback(
        MObject::kNullObj, MayaSessionState::nodeRenamedCB, this);
    _callbackIds.push_back(id);

    id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeOpen, MayaSessionState::sceneClosingCB, this);
    _callbackIds.push_back(id);

    id = MSceneMessage::addCallback(
        MSceneMessage::kBeforeNew, MayaSessionState::sceneClosingCB, this);
    _callbackIds.push_back(id);

    TfWeakPtr<MayaSessionState> me(this);
    _stageResetNoticeKey = TfNotice::Register(me, &MayaSessionState::mayaUsdStageReset);
}

void MayaSessionState::unregisterNotifications()
{
    for (auto id : _callbackIds) {
        MMessage::removeCallback(id);
    }
    _callbackIds.clear();

    TfNotice::Revoke(_stageResetNoticeKey);
}

void MayaSessionState::mayaUsdStageReset(const MayaUsdProxyStageSetNotice& notice)
{
    auto       shapePath = notice.GetShapePath();
    StageEntry entry;
    if (getStageEntry(&entry, shapePath.c_str())) {
        if (entry._proxyShapePath == _currentStageEntry._proxyShapePath) {
            QTimer::singleShot(0, this, [this, entry]() {
                mayaUsdStageResetCBOnIdle(entry);
                setStageEntry(entry);
            });
        } else {
            QTimer::singleShot(0, this, [this, entry]() { mayaUsdStageResetCBOnIdle(entry); });
        }
    }
}

void MayaSessionState::mayaUsdStageResetCBOnIdle(StageEntry const& entry)
{
    Q_EMIT stageResetSignal(entry);
}

/* static */
void MayaSessionState::proxyShapeAddedCB(MObject& node, void* clientData)
{
    auto THIS = static_cast<MayaSessionState*>(clientData);

    // doing it on idle give time to the Load Stage to set a file name
    QTimer::singleShot(0, [THIS, node]() { THIS->proxyShapeAddedCBOnIdle(node); });
}

/* static */
void MayaSessionState::proxyShapeAddedCBOnIdle(const MObject& obj)
{
    // doing it on idle give time to the Load Stage to set a file name
    // but we don't do a second idle because we could get a delete right after a Add
    MDagPath dagPath;
    MFnDagNode(obj).getPath(dagPath);
    auto       shapePath = dagPath.fullPathName();
    StageEntry entry;
    if (getStageEntry(&entry, shapePath)) {
        Q_EMIT stageListChangedSignal(entry);
    }
}

/* static */
void MayaSessionState::proxyShapeRemovedCB(MObject& node, void* clientData)
{
    auto THIS = static_cast<MayaSessionState*>(clientData);
    QTimer::singleShot(0, [THIS]() { THIS->stageListChangedSignal(); });
}

/* static */
void MayaSessionState::nodeRenamedCB(MObject& obj, const MString& oldName, void* clientData)
{
    if (oldName.length() != 0) {
        auto THIS = static_cast<MayaSessionState*>(clientData);

        // doing it on idle give time to the Load Stage to set a file name
        QTimer::singleShot(
            0, [THIS, obj, oldName]() { THIS->nodeRenamedCBOnIdle(oldName.asChar(), obj); });
    }
}

void MayaSessionState::nodeRenamedCBOnIdle(std::string const& oldName, const MObject& obj)
{
    // this does not work:
    //        if OpenMaya.MFnDependencyNode(obj).typeName == PROXY_NODE_TYPE
    if (obj.hasFn(MFn::kShape)) {
        MDagPath dagPath;
        MFnDagNode(obj).getPath(dagPath);
        auto shapePath = dagPath.fullPathName();

        StageEntry entry;
        if (getStageEntry(&entry, shapePath)) {
            // Need to update the current Entry also
            if (_currentStageEntry._displayName == oldName
                && _currentStageEntry._stage == entry._stage) {
                _currentStageEntry = entry;
            }

            Q_EMIT stageRenamedSignal(oldName, entry);
        }
    }
}

/* static */
void MayaSessionState::sceneClosingCB(void* clientData)
{
    auto   THIS = static_cast<MayaSessionState*>(clientData);
    Q_EMIT THIS->clearUIOnSceneResetSignal();
}

bool MayaSessionState::saveLayerUI(QWidget* in_parent, std::string* out_filePath) const
{
    return SaveLayersDialog::saveLayerFilePathUI(*out_filePath);
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
    if (!_currentStageEntry._proxyShapePath.empty()) {

        MString script;
        MString proxyShape(_currentStageEntry._proxyShapePath.c_str());
        MString newValue(in_path.c_str());
        script.format("setAttr -type \"string\" ^1s.filePath \"^2s\"", proxyShape, newValue);
        MGlobal::executeCommand(
            script,
            /*display*/ true,
            /*undo*/ false);
    }
}

void MayaSessionState::setAutoHideSessionLayer(bool hideIt)
{
    int value = hideIt ? 1 : 0;
    MGlobal::setOptionVarValue(AUTO_HIDE_OPTION_VAR, value);
    PARENT_CLASS::setAutoHideSessionLayer(hideIt);
}

void MayaSessionState::printLayer(const PXR_NS::SdfLayerRefPtr& layer) const
{
    MString result, temp;

    temp.format(
        StringResources::getAsMString(StringResources::kUsdLayerIdentifier),
        layer->GetIdentifier().c_str());
    result += temp;
    result += "\n";
    if (layer->GetRealPath() != layer->GetIdentifier()) {
        temp.format(
            StringResources::getAsMString(StringResources::kRealPath),
            layer->GetRealPath().c_str());
        result += temp;
        result += "\n";
    }
    std::string text;
    layer->ExportToString(&text);
    result += text.c_str();
    MGlobal::displayInfo(result);
}

} // namespace UsdLayerEditor
