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

#include "renderGlobals.h"
#include "renderOverride.h"
#include "utils.h"

#include <hdMaya/delegates/delegateRegistry.h>

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>

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

constexpr auto _rendererId = "-r";
constexpr auto _rendererIdLong = "-renderer";

constexpr auto _userDefaultsId = "-u";
constexpr auto _userDefaultsIdLong = "-userDefaults";

constexpr auto _helpText = R"HELP(
Maya to Hydra utility function.
Usage: mtoh [flags]
-listDelegates/-ld : Returns the names of available scene delegates.
-listRenderers/-lr : Returns the names of available render delegates.
-listActiveRenderers/-lar : Returns the names of render delegates that are in
    use in at least one viewport.

-renderer/-r [RENDERER]: Renderer to target for the commands below.
-getRendererDisplayName/-gn : Returns the display name for the given render delegate.
-createRenderGlobals/-crg: Creates the render globals, optionally targetting a
    specific renderer.
-userDefaults/-ud: Flag for createRenderGlobals to restore user defaults on create.
-updateRenderGlobals/-urg [ATTRIBUTE]: Forces the update of the render globals
    for the viewport, optionally targetting a specific renderer or setting.
)HELP";

constexpr auto _helpNonVerboseText = R"HELP(
Use -verbose/-v to see advanced / debugging flags

)HELP";

constexpr auto _helpVerboseText = R"HELP(
Debug flags:

-listRenderIndex/-lri -r [RENDERER]: Returns a list of all the rprims in the
    render index for the given render delegate.

-visibleOnly/-vo: Flag which affects the behavior of -listRenderIndex - if
    given, then only visible items in the render index are returned.

-sceneDelegateId/-sid [SCENE_DELEGATE] -r [RENDERER]: Returns the path id
    corresponding to the given render delegate / scene delegate pair.

)HELP";

} // namespace

MSyntax MtohViewCmd::createSyntax()
{
    MSyntax syntax;

    syntax.addFlag(_listRenderers, _listRenderersLong);

    syntax.addFlag(_listActiveRenderers, _listActiveRenderersLong);

    syntax.addFlag(_rendererId, _rendererIdLong, MSyntax::kString);

    syntax.addFlag(_getRendererDisplayName, _getRendererDisplayNameLong);

    syntax.addFlag(_listDelegates, _listDelegatesLong);

    syntax.addFlag(_createRenderGlobals, _createRenderGlobalsLong);

    syntax.addFlag(_userDefaultsId, _userDefaultsIdLong);

    syntax.addFlag(_updateRenderGlobals, _updateRenderGlobalsLong, MSyntax::kString);

    syntax.addFlag(_help, _helpLong);

    syntax.addFlag(_verbose, _verboseLong);

    // Debug / testing flags

    syntax.addFlag(_listRenderIndex, _listRenderIndexLong);

    syntax.addFlag(_visibleOnly, _visibleOnlyLong);

    syntax.addFlag(_sceneDelegateId, _sceneDelegateIdLong, MSyntax::kString, MSyntax::kString);

    return syntax;
}

MStatus MtohViewCmd::doIt(const MArgList& args)
{
    MStatus status;

    MArgDatabase db(syntax(), args, &status);
    if (!status) {
        return status;
    }

    TfToken renderDelegateName;
    if (db.isFlagSet(_rendererId)) {
        MString id;
        CHECK_MSTATUS_AND_RETURN_IT(db.getFlagArgument(_rendererId, 0, id));

        // Passing 'mtoh' as the renderer adresses all renderers
        if (id != "mtoh") {
            renderDelegateName = TfToken(id.asChar());
        }
    }

    if (db.isFlagSet(_listRenderers)) {
        for (auto& plugin : MtohGetRendererDescriptions())
            appendToResult(plugin.rendererName.GetText());

        // Want to return an empty list, not None
        if (!isCurrentResultArray()) {
            setResult(MStringArray());
        }
    } else if (db.isFlagSet(_listActiveRenderers)) {
        for (const auto& renderer : MtohRenderOverride::AllActiveRendererNames()) {
            appendToResult(renderer);
        }
        // Want to return an empty list, not None
        if (!isCurrentResultArray()) {
            setResult(MStringArray());
        }
    } else if (db.isFlagSet(_getRendererDisplayName)) {
        if (renderDelegateName.IsEmpty()) {
            return MS::kInvalidParameter;
        }

        const auto dn = MtohGetRendererPluginDisplayName(renderDelegateName);
        setResult(MString(dn.c_str()));
    } else if (db.isFlagSet(_listDelegates)) {
        for (const auto& delegate : HdMayaDelegateRegistry::GetDelegateNames()) {
            appendToResult(delegate.GetText());
        }
        // Want to return an empty list, not None
        if (!isCurrentResultArray()) {
            setResult(MStringArray());
        }
    } else if (db.isFlagSet(_help)) {
        MString helpText = _helpText;
        if (db.isFlagSet(_verbose)) {
            helpText += _helpVerboseText;
        } else {
            helpText += _helpNonVerboseText;
        }
        MGlobal::displayInfo(helpText);
    } else if (db.isFlagSet(_createRenderGlobals)) {
        bool userDefaults = db.isFlagSet(_userDefaultsId);
        MtohRenderGlobals::CreateAttributes({ renderDelegateName, true, userDefaults });
    } else if (db.isFlagSet(_updateRenderGlobals)) {
        MString    attrFlag;
        const bool storeUserSettings = true;
        if (db.getFlagArgument(_updateRenderGlobals, 0, attrFlag) == MS::kSuccess) {
            bool          userDefaults = db.isFlagSet(_userDefaultsId);
            const TfToken attrName(attrFlag.asChar());
            auto&         inst = MtohRenderGlobals::GlobalChanged(
                { attrName, false, userDefaults }, storeUserSettings);
            MtohRenderOverride::UpdateRenderGlobals(inst, attrName);
            return MS::kSuccess;
        }
        MtohRenderOverride::UpdateRenderGlobals(
            MtohRenderGlobals::GetInstance(storeUserSettings), renderDelegateName);
    } else if (db.isFlagSet(_listRenderIndex)) {
        if (renderDelegateName.IsEmpty()) {
            return MS::kInvalidParameter;
        }

        auto rprimPaths
            = MtohRenderOverride::RendererRprims(renderDelegateName, db.isFlagSet(_visibleOnly));
        for (auto& rprimPath : rprimPaths) {
            appendToResult(rprimPath.GetText());
        }
        // Want to return an empty list, not None
        if (!isCurrentResultArray()) {
            setResult(MStringArray());
        }
    } else if (db.isFlagSet(_sceneDelegateId)) {
        if (renderDelegateName.IsEmpty()) {
            return MS::kInvalidParameter;
        }

        MString sceneDelegateName;
        CHECK_MSTATUS_AND_RETURN_IT(db.getFlagArgument(_sceneDelegateId, 0, sceneDelegateName));

        SdfPath delegateId = MtohRenderOverride::RendererSceneDelegateId(
            renderDelegateName, TfToken(sceneDelegateName.asChar()));
        setResult(MString(delegateId.GetText()));
    }
    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
