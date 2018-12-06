//
// Copyright 2018 Luma Pictures
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

constexpr auto _getWireframeSelectionHighlight = "-gwh";
constexpr auto _getWireframeSelectionHighlightLong =
    "-getWireframeSelectionHighlight";

constexpr auto _setWireframeSelectionHighlight = "-swh";
constexpr auto _setWireframeSelectionHighlightLong =
    "-setWireframeSelectionHighlight";

constexpr auto _getColorSelectionHighlight = "-gch";
constexpr auto _getColorSelectionHighlightLong = "-getColorSelectionHighlight";

constexpr auto _setColorSelectionHighlight = "-sch";
constexpr auto _setColorSelectionHighlightLong = "-setColorSelectionHighlight";

constexpr auto _getColorSelectionHighlightColor = "-gcc";
constexpr auto _getColorSelectionHighlightColorLong =
    "-getColorSelectionHighlightColor";

constexpr auto _setColorSelectionHighlightColor = "-scc";
constexpr auto _setColorSelectionHighlightColorLong =
    "-setColorSelectionHighlightColor";

constexpr auto _createRenderGlobals = "-crg";
constexpr auto _createRenderGlobalsLong = "-createRenderGlobals";

constexpr auto _updateRenderGlobals = "-urg";
constexpr auto _updateRenderGlobalsLong = "-updateRenderGlobals";

constexpr auto _help = "-h";
constexpr auto _helpLong = "-help";

constexpr auto _helpText = R"HELP(
Maya to Hydra utility function.
Usage: mtoh [flags]

-getColorSelectionHighlightColor/-gcc : Returns the RGBA value used to
    highlight selections.
-getColorSelectionHighlight/-gch : Returns true if color selection highlight
    is enabled, false otherwise.
-getRendererDisplayName/-gn : Returns the display name for the current render
    delegate.
-getWireframeSelectionHighlight/-gwh : Returns true if wireframe selection
    highlight is enabled, false otherwise. This is only available for the
    HdStreamRendererPlugin.
-listDelegates/-ld : Returns the names of available scene delegates.
-listRenderers/-lr : Returns the names of available render delegates.
-setColorSelectionHighlightColor/-scc [float] [float] [float] [float] : Sets the
    RGBA color used to highlight selections.
-setColorSelectionHighlight/-sch [bool] : Turns color highlight of selections
    on or off.
-setMaximumShadowMapResolution/-sms [int] : Sets the maximum shadow map
    resolution in pixels for shadows in the HdStreamRendererPlugin.
-setTextureMemoryPerTexture/-stm [int] : Sets the maximum texture memory in
    bytes allowed for each texture for textures in the HdStreamRendererPlugin.
-setWireframeSelectionHighlight/-swh [bool] : Turns wireframe highlight for
    selections on or off.

)HELP";

} // namespace

MSyntax MtohViewCmd::createSyntax() {
    MSyntax syntax;

    syntax.addFlag(_listRenderers, _listRenderersLong);

    syntax.addFlag(
        _getRendererDisplayName, _getRendererDisplayNameLong, MSyntax::kString);

    syntax.addFlag(_listDelegates, _listDelegatesLong);

    syntax.addFlag(
        _getWireframeSelectionHighlight, _getWireframeSelectionHighlightLong);

    syntax.addFlag(
        _setWireframeSelectionHighlight, _setWireframeSelectionHighlightLong,
        MSyntax::kBoolean);

    syntax.addFlag(
        _getColorSelectionHighlight, _getColorSelectionHighlightLong);

    syntax.addFlag(
        _setColorSelectionHighlight, _setColorSelectionHighlightLong,
        MSyntax::kBoolean);

    syntax.addFlag(
        _getColorSelectionHighlightColor, _getColorSelectionHighlightColorLong);

    syntax.addFlag(
        _setColorSelectionHighlightColor, _setColorSelectionHighlightColorLong,
        MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);

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
    } else if (db.isFlagSet(_getWireframeSelectionHighlight)) {
        appendToResult(MtohRenderOverride::GetWireframeSelectionHighlight());
    } else if (db.isFlagSet(_setWireframeSelectionHighlight)) {
        bool res = true;
        if (db.getFlagArgument(_setWireframeSelectionHighlight, 0, res)) {
            MtohRenderOverride::SetWireframeSelectionHighlight(res);
        }
    } else if (db.isFlagSet(_getColorSelectionHighlight)) {
        appendToResult(MtohRenderOverride::GetColorSelectionHighlight());
    } else if (db.isFlagSet(_setColorSelectionHighlight)) {
        bool res = true;
        if (db.getFlagArgument(_setColorSelectionHighlight, 0, res)) {
            MtohRenderOverride::SetColorSelectionHighlight(res);
        }
    } else if (db.isFlagSet(_getColorSelectionHighlightColor)) {
        const auto res = MtohRenderOverride::GetColorSelectionHighlightColor();
        appendToResult(res[0]);
        appendToResult(res[1]);
        appendToResult(res[2]);
        appendToResult(res[3]);
    } else if (db.isFlagSet(_setColorSelectionHighlightColor)) {
        GfVec4d res(1.0, 1.0, 0.0, 0.5);
        if (db.getFlagArgument(_setColorSelectionHighlightColor, 0, res[0]) &&
            db.getFlagArgument(_setColorSelectionHighlightColor, 1, res[1]) &&
            db.getFlagArgument(_setColorSelectionHighlightColor, 2, res[2]) &&
            db.getFlagArgument(_setColorSelectionHighlightColor, 3, res[3])) {
            MtohRenderOverride::SetColorSelectionHighlightColor(res);
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
