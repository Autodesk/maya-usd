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

#include "mayaCommandHook.h"

#include "abstractCommandHook.h"
#include "mayaSessionState.h"

#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/layerLocking.h>
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/selection.h>

#include <QtCore/QStringList>

#include <cassert>
#include <string>

#define STR(x) std::string(x)

namespace {

std::string quote(const std::string& string) { return STR(" \"") + string + STR("\""); }

// maya doesn't support spaces in undo chunk names...
MString cleanChunkName(QString name) { return quote(name.replace(" ", "_").toStdString()).c_str(); }

std::string getProxyShapeName(const std::string& proxyShapePath)
{
    std::size_t found = proxyShapePath.find_last_of("|");
    if (std::string::npos != found) {
        return proxyShapePath.substr(found + 1);
    } else {
        return proxyShapePath;
    }
}

bool getBooleanAttributeOnProxyShape(
    const std::string& proxyShapePath,
    const std::string& attributeName)
{
    if (proxyShapePath.empty()) {
        return false;
    }

    MObject mobj;
    MStatus status = PXR_NS::UsdMayaUtil::GetMObjectByName(getProxyShapeName(proxyShapePath), mobj);
    if (status == MStatus::kSuccess) {
        MFnDependencyNode fn;
        fn.setObject(mobj);
        bool attribute;
        if (PXR_NS::UsdMayaUtil::getPlugValue(fn, attributeName.c_str(), &attribute)) {
            return attribute;
        }
    }

    return false;
}

} // namespace

namespace UsdLayerEditor {
std::string MayaCommandHook::proxyShapePath()
{
    return dynamic_cast<MayaSessionState*>(_sessionState)->proxyShapePath();
}

void MayaCommandHook::setEditTarget(UsdLayer usdLayer)
{
    std::string cmd;
    cmd = STR("mayaUsdEditTarget -edit -editTarget ") + quote(usdLayer->GetIdentifier());
    cmd += " " + quote(proxyShapePath());
    executeMel(cmd);
}

// starts a complex undo operation in the host app. Please use UndoContext class to safely
// open/close
void MayaCommandHook::openUndoBracket(const QString& name)
{
    MGlobal::executeCommand(
        MString("undoInfo -openChunk -chunkName ") + cleanChunkName(name), false, false);
}

// closes a complex undo operation in the host app. Please use UndoContext class to safely
// open/close
void MayaCommandHook::closeUndoBracket()
{
    MGlobal::executeCommand("undoInfo -closeChunk", false, false);
}

// insert a sub layer path at a given index
void MayaCommandHook::insertSubLayerPath(UsdLayer usdLayer, Path path, int index)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -insertSubPath ";
    cmd += std::to_string(index);
    cmd += quote(path);
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);
}

// remove a sub layer by path
void MayaCommandHook::removeSubLayerPath(UsdLayer usdLayer, Path path)
{
    size_t index = usdLayer->GetSubLayerPaths().Find(path);
    assert(index != static_cast<size_t>(-1));
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -removeSubPath ";
    cmd += std::to_string(index);
    cmd += quote(proxyShapePath());
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);
}

void MayaCommandHook::moveSubLayerPath(
    Path     path,
    UsdLayer oldParentUsdLayer,
    UsdLayer newParentUsdLayer,
    int      index)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -moveSubPath ";
    cmd += quote(path);
    cmd += quote(newParentUsdLayer->GetIdentifier());
    cmd += std::to_string(index);
    cmd += quote(oldParentUsdLayer->GetIdentifier());
    executeMel(cmd);
}

// replaces a path in the layer stack
void MayaCommandHook::replaceSubLayerPath(UsdLayer usdLayer, Path oldPath, Path newPath)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -replaceSubPath ";
    cmd += quote(oldPath);
    cmd += quote(newPath);
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);
}

// discard edit on a layer
void MayaCommandHook::discardEdits(UsdLayer usdLayer)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -discardEdits ";
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);

    refreshLayerSystemLock(usdLayer);
}

// erases everything on a layer
void MayaCommandHook::clearLayer(UsdLayer usdLayer)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -clear ";
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);
}

// add an anon layer at the top of the stack, returns it
UsdLayer MayaCommandHook::addAnonymousSubLayer(UsdLayer usdLayer, std::string newName)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -addAnonymous ";
    cmd += quote(newName);
    cmd += quote(usdLayer->GetIdentifier());
    std::string result = executeMel(cmd);
    if (result.size() > 0)
        return PXR_NS::SdfLayer::FindOrOpen(result);
    else
        return {};
}

// mute or unmute the given layer
void MayaCommandHook::muteSubLayer(UsdLayer usdLayer, bool muteIt)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -muteLayer ";
    cmd += muteIt ? "1" : "0";
    cmd += quote(proxyShapePath());
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);
}

// lock, system-lock or unlock the given layer
void MayaCommandHook::lockLayer(
    UsdLayer               usdLayer,
    MayaUsd::LayerLockType lockState,
    bool                   includeSubLayers)
{
    // Per design, we refuse to change the lock state of system-locked
    // layers through the UI.
    if (MayaUsd::isLayerSystemLocked(usdLayer))
        return;

    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -skipSystemLocked -lockLayer ";
    cmd += std::to_string(lockState);
    cmd += includeSubLayers ? " 1" : " 0";
    cmd += quote(proxyShapePath());
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);
}

void MayaCommandHook::refreshLayerSystemLock(UsdLayer usdLayer, bool refreshSubLayers /*= false*/)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -refreshSystemLock ";
    cmd += quote(proxyShapePath());
    cmd += " ";
    cmd += std::to_string(refreshSubLayers);
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd);
}

void MayaCommandHook::stitchLayers(
    const std::vector<PXR_NS::SdfLayerRefPtr>& layers,
    const std::vector<PXR_NS::SdfLayerRefPtr>& parents)
{
    // KYLE TODO
}

// Help menu callback
void MayaCommandHook::showLayerEditorHelp()
{
    MGlobal::executePythonCommand(
        "from mayaUsdUtils import showHelpMayaUSD; showHelpMayaUSD(\"UsdLayerEditor\");");
}

// this method is used to select the prims with spec in a layer
void MayaCommandHook::selectPrimsWithSpec(UsdLayer usdLayer)
{
    Ufe::Selection sn;
    for (auto prim : _sessionState->stage()->Traverse()) {
        auto primSpec = usdLayer->GetPrimAtPath(prim.GetPath());
        if (primSpec) {
            auto ufePath
                = Ufe::PathString::path(proxyShapePath() + "," + prim.GetPath().GetString());
            auto ufeSceneItem = Ufe::Hierarchy::createItem(ufePath);
            if (ufeSceneItem) {
                sn.append(ufeSceneItem);
            }
        }
    }
    if (sn.empty()) {
        return;
    }

    MayaUsd::UfeSelectionUndoItem::select("selectPrimsWithSpec", sn);
}

bool MayaCommandHook::isProxyShapeStageIncoming(const std::string& proxyShapePath)
{
    return getBooleanAttributeOnProxyShape(proxyShapePath, "stageIncoming");
}

bool MayaCommandHook::isProxyShapeSharedStage(const std::string& proxyShapePath)
{
    return getBooleanAttributeOnProxyShape(proxyShapePath, "shareStage");
}

std::string MayaCommandHook::executeMel(const std::string& commandString)
{
    if (areCommandsDelayed()) {
        _delayedCommands.push_back({ commandString, false });
    } else {
        // executes maya command with display and undo set to true so that it logs
        MStringArray result;
        MGlobal::executeCommand(
            MString(commandString.c_str()),
            result,
            /*display*/ true,
            /*undo*/ true);
        if (result.length() > 0)
            return result[0].asChar();
    }
    return "";
}

void MayaCommandHook::executePython(const std::string& commandString)
{
    if (areCommandsDelayed()) {
        _delayedCommands.push_back({ commandString, true });
    } else {
        MGlobal::executePythonCommand(commandString.c_str());
    }
}

void MayaCommandHook::executeDelayedCommands()
{
    if (areCommandsDelayed())
        return;

    // In case the execution of commands add new commands,
    // make a copy and clear the delayed commands.
    std::vector<DelayedCommand> cmds = _delayedCommands;
    _delayedCommands.clear();

    for (const auto& cmd : cmds) {
        if (cmd.isPython) {
            executePython(cmd.command);
        } else {
            executeMel(cmd.command);
        }
    }
}

} // namespace UsdLayerEditor
