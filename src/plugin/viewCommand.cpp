//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "viewCommand.h"

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>

#include <hdmaya/delegates/delegateRegistry.h>

#include "renderGlobals.h"
#include "renderOverride.h"
#include "utils.h"

PXR_NAMESPACE_OPEN_SCOPE

const MString MtohViewCmd::name("mtoh");

namespace {

constexpr auto _listRenderers = "-lr";
constexpr auto _listRenderersLong = "-listRenderers";

constexpr auto _getRendererDisplayName = "-gn";
constexpr auto _getRendererDisplayNameLong = "-getRendererDisplayName";

constexpr auto _listDelegates = "-ld";
constexpr auto _listDelegatesLong = "-listDelegates";

constexpr auto _createRenderGlobals = "-crg";
constexpr auto _createRenderGlobalsLong = "-createRenderGlobals";

constexpr auto _updateRenderGlobals = "-urg";
constexpr auto _updateRenderGlobalsLong = "-updateRenderGlobals";

constexpr auto _help = "-h";
constexpr auto _helpLong = "-help";

constexpr auto _helpText = R"HELP(
Maya to Hydra utility function.
Usage: mtoh [flags]

-getRendererDisplayName/-gn : Returns the display name for the current render
    delegate.
-listDelegates/-ld : Returns the names of available scene delegates.
-listRenderers/-lr : Returns the names of available render delegates.
-createRenderGlobals/-crg : Creates the render globals.
-updateRenderGlobals/-urg : Forces the update of the render globals for the viewport.

)HELP";

} // namespace

MSyntax MtohViewCmd::createSyntax() {
    MSyntax syntax;

    syntax.addFlag(_listRenderers, _listRenderersLong);

    syntax.addFlag(
        _getRendererDisplayName, _getRendererDisplayNameLong, MSyntax::kString);

    syntax.addFlag(_listDelegates, _listDelegatesLong);

    syntax.addFlag(_createRenderGlobals, _createRenderGlobalsLong);

    syntax.addFlag(_updateRenderGlobals, _updateRenderGlobalsLong);

    syntax.addFlag(_help, _helpLong);

    return syntax;
}

MStatus MtohViewCmd::doIt(const MArgList& args) {
    MArgDatabase db(syntax(), args);

    if (db.isFlagSet(_listRenderers)) {
        for (const auto& renderer : MtohGetRendererPlugins()) {
            appendToResult(renderer.GetText());
        }
    } else if (db.isFlagSet(_getRendererDisplayName)) {
        MString id;
        if (db.getFlagArgument(_getRendererDisplayName, 0, id)) {
            const auto dn =
                MtohGetRendererPluginDisplayName(TfToken(id.asChar()));
            setResult(MString(dn.c_str()));
        }
    } else if (db.isFlagSet(_listDelegates)) {
        for (const auto& delegate :
             HdMayaDelegateRegistry::GetDelegateNames()) {
            appendToResult(delegate.GetText());
        }
    } else if (db.isFlagSet(_help)) {
        MGlobal::displayInfo(MString(_helpText));
    } else if (db.isFlagSet(_createRenderGlobals)) {
        MtohCreateRenderGlobals();
    } else if (db.isFlagSet(_updateRenderGlobals)) {
        MtohRenderOverride::UpdateRenderGlobals();
    }
    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
