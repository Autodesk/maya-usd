//
// Copyright 2020 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//ı
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "layerEditorWindowCommand.h"

#include "abstractLayerEditorWindow.h"

#include <maya/MGlobal.h>
#include <maya/MSyntax.h>

namespace {
#define DEF_FLAG(short, long)             \
    const char k_##long##Flag[] = #short; \
    const char k_##long##FlagLong[] = #long;

#define FLAG(long) k_##long##FlagLong

DEF_FLAG(rl, reload)
DEF_FLAG(ps, proxyShape)

// query flags
DEF_FLAG(se, selectionLength)
DEF_FLAG(il, isInvalidLayer)
DEF_FLAG(sl, isSessionLayer)
DEF_FLAG(dl, isLayerDirty)
DEF_FLAG(su, isSubLayer)
DEF_FLAG(al, isAnonymousLayer)
DEF_FLAG(ns, layerNeedsSaving)
DEF_FLAG(am, layerAppearsMuted)
DEF_FLAG(mu, layerIsMuted)

// edit flags
DEF_FLAG(rs, removeSubLayer)
DEF_FLAG(sv, saveEdits)
DEF_FLAG(de, discardEdits)
DEF_FLAG(aa, addAnonymousSublayer)
DEF_FLAG(ap, addParentLayer)
DEF_FLAG(lo, loadSubLayers)
DEF_FLAG(ml, muteLayer)
DEF_FLAG(pl, printLayer)
DEF_FLAG(cl, clearLayer)
DEF_FLAG(sp, selectPrimsWithSpec)

const MString WORKSPACE_CONTROL_NAME = "mayaUsdLayerEditor";
} // namespace

namespace MAYAUSD_NS_DEF {

//  AbstractLayerEditorCreator implememtation

AbstractLayerEditorCreator* AbstractLayerEditorCreator::_instance = nullptr;

AbstractLayerEditorCreator* AbstractLayerEditorCreator::instance()
{
    return AbstractLayerEditorCreator::_instance;
}

AbstractLayerEditorCreator::AbstractLayerEditorCreator()
{
    AbstractLayerEditorCreator::_instance = this;
}

AbstractLayerEditorCreator::~AbstractLayerEditorCreator()
{
    AbstractLayerEditorCreator::_instance = nullptr;
}

//  AbstractLayerEditorWindow implementation

AbstractLayerEditorWindow::AbstractLayerEditorWindow(const char* panelName)
{
    // this empty implementation is necessary for linking
}

AbstractLayerEditorWindow::~AbstractLayerEditorWindow()
{
    // this empty implementation is necessary for linking
}

//  LayerEditorWindowCommand implementation

const char LayerEditorWindowCommand::commandName[] = "mayaUsdLayerEditorWindow";

// plug-in callback to create the command object
void* LayerEditorWindowCommand::creator()
{
    return static_cast<MPxCommand*>(new LayerEditorWindowCommand());
}

// plug-in callback to register the command syntax
MSyntax LayerEditorWindowCommand::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(true);
    syntax.enableEdit(true);

#define ADD_FLAG(name) syntax.addFlag(k_##name##Flag, k_##name##FlagLong)

    ADD_FLAG(reload);
    syntax.addFlag(k_proxyShapeFlag, k_proxyShapeFlagLong, MSyntax::kString);

    ADD_FLAG(selectionLength);
    ADD_FLAG(isInvalidLayer);
    ADD_FLAG(isSessionLayer);
    ADD_FLAG(isLayerDirty);
    ADD_FLAG(isSubLayer);
    ADD_FLAG(isAnonymousLayer);
    ADD_FLAG(layerNeedsSaving);
    ADD_FLAG(layerAppearsMuted);
    ADD_FLAG(layerIsMuted);

    ADD_FLAG(removeSubLayer);
    ADD_FLAG(saveEdits);
    ADD_FLAG(discardEdits);
    ADD_FLAG(addAnonymousSublayer);
    ADD_FLAG(addParentLayer);
    ADD_FLAG(loadSubLayers);
    ADD_FLAG(muteLayer);
    ADD_FLAG(printLayer);
    ADD_FLAG(clearLayer);
    ADD_FLAG(selectPrimsWithSpec);

    // editor name
    syntax.addArg(MSyntax::kString);

    return syntax;
}

LayerEditorWindowCommand::LayerEditorWindowCommand()
{
    //
}
bool LayerEditorWindowCommand::isUndoable() const
{
    //
    return false;
}

MStatus LayerEditorWindowCommand::doIt(const MArgList& argList)
{
    const MString WINDOW_TITLE_NAME = "USD Layer Editor";
    const MString DEAULT_EDITOR_NAME = "mayaUsdLayerEditor";

    auto handler = AbstractLayerEditorCreator::instance();
    if (!handler) {
        return MS::kNotFound;
    }

    // get the name of the layer editor use
    MArgParser argParser(syntax(), argList);
    MString    editorName = argParser.commandArgumentString(0);
    if (editorName.length() == 0) {
        editorName = DEAULT_EDITOR_NAME;
    }

    // get the pointer to that control
    auto layerEditorWindow = handler->getWindow(editorName.asChar());
    if (argParser.isQuery() || argParser.isEdit()) {
        if (!layerEditorWindow) {
            MString errorMsg;
            errorMsg.format("layer editor named ^1s not found", editorName);
            displayError(errorMsg);
            return MS::kNotFound;
        }
    }

    bool createOrShowWindow = false;
    if (!argParser.isQuery()) {
        if (argParser.isEdit()) {
            if (argParser.isFlagSet(k_reloadFlag)) {
                createOrShowWindow = true;
            }
        } else {
            // create mode
            createOrShowWindow = true;
        }
    }

    MStatus st;

    // call this always, to check for flags used in wrong mode
    st = handleQueries(argParser, layerEditorWindow);
    if (st != MS::kSuccess) {
        return st;
    }

    // call this always, to check for -e mode missing
    st = handleEdits(argParser, layerEditorWindow);
    if (st != MS::kSuccess)
        return st;

    if (createOrShowWindow) {
        if (layerEditorWindow) {
            // Call -restore on existing workspace control to make it visible
            // from whatever previous state it was in
            MString restoreCommand;
            restoreCommand.format("workspaceControl -e -restore ^1s", WORKSPACE_CONTROL_NAME);
            MGlobal::executeCommand(restoreCommand);
        } else {
            bool doReload = argParser.isFlagSet(k_reloadFlag);
            // If we're not reloading a workspace, we need to create a workspace control
            if (!doReload) {

                MString cmdString;
                cmdString.format(
                    "workspaceControl"
                    " -label \"^1s\""
                    " -retain false"
                    " -deleteLater false"
                    " -loadImmediately true"
                    " -floating true"
                    " -initialWidth 400"
                    " -initialHeight 600"
                    " -requiredPlugin \"^2s\""
                    " \"^3s\"",
                    WINDOW_TITLE_NAME,
                    "mayaUsdPlugin",
                    WORKSPACE_CONTROL_NAME);
                MGlobal::executeCommand(cmdString);
            }
            layerEditorWindow = handler->createWindow(editorName.asChar());
            // Set the -uiScript (used to rebuild UI when reloading workspace)
            // after creation so that it doesn't get executed
            if (!doReload) {
                const MString mCommandName = LayerEditorWindowCommand::commandName;
                MString       uiScriptCommand;
                uiScriptCommand.format(
                    R"(workspaceControl -e -uiScript "^1s -reload" "^2s")",
                    mCommandName,
                    WORKSPACE_CONTROL_NAME);
                MGlobal::executeCommand(uiScriptCommand);
            }
        }

        if (argParser.isFlagSet(k_proxyShapeFlag)) {
            MString proxyShapeName;
            argParser.getFlagArgument(k_proxyShapeFlag, 0, proxyShapeName);
            if (proxyShapeName.length() > 0) {
                layerEditorWindow->selectProxyShape(proxyShapeName.asChar());
            }
        }
    }

    return MS::kSuccess;
}

void LayerEditorWindowCommand::cleanupOnPluginUnload()
{
    //  Close the workspace control, if it still exists.
    auto handler = AbstractLayerEditorCreator::instance();
    if (handler) {
        MString closeCommand;
        auto    panels = handler->getAllPanelNames();
        for (auto entry : panels) {
            closeCommand.format("workspaceControl -e -close \"^1s\"", MString(entry.c_str()));
            MGlobal::executeCommand(closeCommand);
        }
    }
}

MStatus LayerEditorWindowCommand::undoIt()
{
    //
    return MS::kSuccess;
}

MStatus LayerEditorWindowCommand::redoIt()
{
    //
    return MS::kSuccess;
}

MStatus LayerEditorWindowCommand::handleQueries(
    const MArgParser&          argParser,
    AbstractLayerEditorWindow* layerEditor)
{
    // this methis is written so that it'll return an error
    // if a query flag is used in non-query mode.
    bool    notQuery = !argParser.isQuery();
    MString errorMsg("Need -query mode for parameter ");

#define HANDLE_Q_FLAG(name)                \
    if (argParser.isFlagSet(FLAG(name))) { \
        if (notQuery) {                    \
            errorMsg += FLAG(name);        \
            displayError(errorMsg);        \
            return MS::kInvalidParameter;  \
        }                                  \
        setResult(layerEditor->name());    \
    }

    HANDLE_Q_FLAG(selectionLength)
    HANDLE_Q_FLAG(isInvalidLayer)
    HANDLE_Q_FLAG(isSessionLayer)
    HANDLE_Q_FLAG(isLayerDirty)
    HANDLE_Q_FLAG(isSubLayer)
    HANDLE_Q_FLAG(isAnonymousLayer)
    HANDLE_Q_FLAG(layerNeedsSaving)
    HANDLE_Q_FLAG(layerAppearsMuted)
    HANDLE_Q_FLAG(layerIsMuted)

    return MS::kSuccess;
}

MStatus LayerEditorWindowCommand::handleEdits(
    const MArgParser&          argParser,
    AbstractLayerEditorWindow* layerEditor)
{
    //
    // this method is written so that it'll return an error
    // if a query flag is used in non-query mode.
    bool    notEdit = !argParser.isEdit();
    MString errorMsg("Need -edit mode for parameter ");

#define HANDLE_E_FLAG(name)                \
    if (argParser.isFlagSet(FLAG(name))) { \
        if (notEdit) {                     \
            errorMsg += FLAG(name);        \
            displayError(errorMsg);        \
            return MS::kInvalidParameter;  \
        }                                  \
        layerEditor->name();               \
    }

    HANDLE_E_FLAG(removeSubLayer)
    HANDLE_E_FLAG(saveEdits)
    HANDLE_E_FLAG(discardEdits)
    HANDLE_E_FLAG(addAnonymousSublayer)
    HANDLE_E_FLAG(addParentLayer)
    HANDLE_E_FLAG(loadSubLayers)
    HANDLE_E_FLAG(muteLayer)
    HANDLE_E_FLAG(printLayer)
    HANDLE_E_FLAG(clearLayer)
    HANDLE_E_FLAG(selectPrimsWithSpec)

    return MS::kSuccess;
}

} // namespace MAYAUSD_NS_DEF
