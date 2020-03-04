//
// Copyright 2019 Luma Pictures
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
#include "viewCommand.h"

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>

#include "../../usd/hdMaya/delegates/delegateRegistry.h"
#include "renderOverride.h"

#include "renderGlobals.h"
#include "renderOverride.h"
#include "utils.h"

PXR_NAMESPACE_OPEN_SCOPE

const MString MtohViewCmd::name("mtoh");

namespace {

constexpr auto _listRenderers = "-lr";
constexpr auto _listRenderersLong = "-listRenderers";

constexpr auto _listActiveRenderers = "-lar";
constexpr auto _listActiveRenderersLong = "-listActiveRenderers";

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

constexpr auto _verbose = "-v";
constexpr auto _verboseLong = "-verbose";

constexpr auto _listRenderIndex = "-lri";
constexpr auto _listRenderIndexLong = "-listRenderIndex";

constexpr auto _visibleOnly = "-vo";
constexpr auto _visibleOnlyLong = "-visibleOnly";

constexpr auto _sceneDelegateId = "-sid";
constexpr auto _sceneDelegateIdLong = "-sceneDelegateId";

constexpr auto _helpText = R"HELP(
Maya to Hydra utility function.
Usage: mtoh [flags]

-getRendererDisplayName/-gn [RENDERER]: Returns the display name for the given
    render delegate.
-listDelegates/-ld : Returns the names of available scene delegates.
-listRenderers/-lr : Returns the names of available render delegates.
-listActiveRenderers/-lar : Returns the names of render delegates that are in
    use in at least one viewport.
-createRenderGlobals/-crg : Creates the render globals.
-updateRenderGlobals/-urg : Forces the update of the render globals for the
    viewport.
)HELP";

constexpr auto _helpNonVerboseText = R"HELP(
Use -verbose/-v to see advanced / debugging flags

)HELP";

constexpr auto _helpVerboseText = R"HELP(
Debug flags:

-listRenderIndex/-lri [RENDERER]: Returns a list of all the rprims in the
    render index for the given render delegate.

-visibleOnly/-vo: Flag which affects the behavior of -listRenderIndex - if
    given, then only visible items in the render index are returned.

-sceneDelegateId/-sid [RENDERER] [SCENE_DELEGATE]: Returns the path id
    corresponding to the given render delegate / scene delegate pair.

)HELP";

} // namespace

MSyntax MtohViewCmd::createSyntax() {
    MSyntax syntax;

    syntax.addFlag(_listRenderers, _listRenderersLong);

    syntax.addFlag(_listActiveRenderers, _listActiveRenderersLong);

    syntax.addFlag(
        _getRendererDisplayName, _getRendererDisplayNameLong, MSyntax::kString);

    syntax.addFlag(_listDelegates, _listDelegatesLong);

    syntax.addFlag(_createRenderGlobals, _createRenderGlobalsLong);

    syntax.addFlag(_updateRenderGlobals, _updateRenderGlobalsLong);

    syntax.addFlag(_help, _helpLong);

    syntax.addFlag(_verbose, _verboseLong);

    // Debug / testing flags

    syntax.addFlag(_listRenderIndex, _listRenderIndexLong, MSyntax::kString);

    syntax.addFlag(_visibleOnly, _visibleOnlyLong);

    syntax.addFlag(
        _sceneDelegateId, _sceneDelegateIdLong, MSyntax::kString,
        MSyntax::kString);

    return syntax;
}

MStatus MtohViewCmd::doIt(const MArgList& args) {
    MStatus status;

    MArgDatabase db(syntax(), args, &status);
    if (!status) { return status; }

    if (db.isFlagSet(_listRenderers)) {
        for (auto& plugin : MtohGetRendererDescriptions())
            appendToResult(plugin.rendererName.GetText());

        // Want to return an empty list, not None
        if (!isCurrentResultArray()) { setResult(MStringArray()); }
    } else if (db.isFlagSet(_listActiveRenderers)) {
        for (const auto& renderer :
             MtohRenderOverride::AllActiveRendererNames()) {
            appendToResult(renderer);
        }
        // Want to return an empty list, not None
        if (!isCurrentResultArray()) { setResult(MStringArray()); }
    } else if (db.isFlagSet(_getRendererDisplayName)) {
        MString id;
        CHECK_MSTATUS_AND_RETURN_IT(
            db.getFlagArgument(_getRendererDisplayName, 0, id));
        const auto dn = MtohGetRendererPluginDisplayName(TfToken(id.asChar()));
        setResult(MString(dn.c_str()));
    } else if (db.isFlagSet(_listDelegates)) {
        for (const auto& delegate :
             HdMayaDelegateRegistry::GetDelegateNames()) {
            appendToResult(delegate.GetText());
        }
        // Want to return an empty list, not None
        if (!isCurrentResultArray()) { setResult(MStringArray()); }
    } else if (db.isFlagSet(_help)) {
        MString helpText = _helpText;
        if (db.isFlagSet(_verbose)) {
            helpText += _helpVerboseText;
        } else {
            helpText += _helpNonVerboseText;
        }
        MGlobal::displayInfo(helpText);
    } else if (db.isFlagSet(_createRenderGlobals)) {
        MtohCreateRenderGlobals();
    } else if (db.isFlagSet(_updateRenderGlobals)) {
        MtohRenderOverride::UpdateRenderGlobals();
    } else if (db.isFlagSet(_listRenderIndex)) {
        MString id;
        CHECK_MSTATUS_AND_RETURN_IT(
            db.getFlagArgument(_listRenderIndex, 0, id));
        const TfToken rendererName(id.asChar());
        auto rprimPaths = MtohRenderOverride::RendererRprims(
            rendererName, db.isFlagSet(_visibleOnly));
        for (auto& rprimPath : rprimPaths) {
            appendToResult(rprimPath.GetText());
        }
        // Want to return an empty list, not None
        if (!isCurrentResultArray()) { setResult(MStringArray()); }
    } else if (db.isFlagSet(_sceneDelegateId)) {
        MString renderDelegateName;
        MString sceneDelegateName;
        CHECK_MSTATUS_AND_RETURN_IT(
            db.getFlagArgument(_sceneDelegateId, 0, renderDelegateName));
        CHECK_MSTATUS_AND_RETURN_IT(
            db.getFlagArgument(_sceneDelegateId, 1, sceneDelegateName));

        SdfPath delegateId = MtohRenderOverride::RendererSceneDelegateId(
            TfToken(renderDelegateName.asChar()),
            TfToken(sceneDelegateName.asChar()));
        setResult(MString(delegateId.GetText()));
    }
    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
