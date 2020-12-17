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

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <QtCore/QStringList>

#include <cassert>
#include <string>

#define STR(x) std::string(x)

namespace {
std::string quote(const std::string& string) { return STR(" \"") + string + STR("\""); }

MString executeMel(const std::string& commandString)
{
    // executes maya command with display and undo set to true so that it logs
    MStringArray result;
    MGlobal::executeCommand(
        MString(commandString.c_str()),
        result,
        /*display*/ true,
        /*undo*/ true);
    if (result.length() > 0)
        return result[0];
    else
        return "";
}

// maya doesn't support spaces in undo chunk names...
MString cleanChunkName(QString name) { return quote(name.replace(" ", "_").toStdString()).c_str(); }

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
    std::string result = executeMel(cmd).asChar();
    return PXR_NS::SdfLayer::FindOrOpen(result);
}

// mute or unmute the given layer
void MayaCommandHook::muteSubLayer(UsdLayer usdLayer, bool muteIt)
{
    std::string cmd;
    cmd = "mayaUsdLayerEditor -edit -muteLayer ";
    cmd += muteIt ? "1" : "0";
    cmd += quote(proxyShapePath());
    cmd += quote(usdLayer->GetIdentifier());
    executeMel(cmd).asChar();
}

// Help menu callback
void MayaCommandHook::showLayerEditorHelp() { executeMel("showHelp UsdLayerEditor"); }

// this method is used to select the prims with spec in a layer
void MayaCommandHook::selectPrimsWithSpec(UsdLayer usdLayer)
{
    std::string script;
    script = "proxyShapePath = '" + proxyShapePath() + "'\n";
    script += "primPathList = [";

    QStringList array;
    for (auto prim : _sessionState->stage()->Traverse()) {
        auto primSpec = usdLayer->GetPrimAtPath(prim.GetPath());
        if (primSpec) {
            array.append(prim.GetPath().GetString().c_str());
        }
    }
    if (array.size() == 0)
        return;
    script += ("'");
    script += array.join("','").toStdString();
    script += ("']\n");

    script += R"PYTHON(
# Ufe
import ufe
try:
    from maya.internal.ufeSupport import ufeSelectCmd
except ImportError:
    # Maya 2019 and 2020 don't have ufeSupport plugin, so use fallback.
    from ufeScripts import ufeSelectCmd

# create a selection list
sn = ufe.Selection()

for primPath in primPathList:
    ufePath = ufe.PathString.path(proxyShapePath + ',' + primPath)
    ufeSceneItem = ufe.Hierarchy.createItem(ufePath)
    sn.append(ufeSceneItem)

ufeSelectCmd.replaceWith(sn)
)PYTHON";

    MGlobal::executePythonCommand(script.c_str(), true, false);
}

} // namespace UsdLayerEditor
