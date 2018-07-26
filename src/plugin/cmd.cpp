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
#include "cmd.h"

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>

#include "renderOverride.h"
#include <hdmaya/delegates/delegateRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

const MString HdMayaCmd::name("hdmaya");

namespace {

constexpr auto _listRenderers = "-lr";
constexpr auto _listRenderersLong = "-listRenderers";

constexpr auto _getRendererDisplayName = "-gn";
constexpr auto _getRendererDisplayNameLong = "-getRendererDisplayName";

constexpr auto _changeRenderer = "-cr";
constexpr auto _changeRendererLong = "-changeRenderer";

constexpr auto _listDelegates = "-ld";
constexpr auto _listDelegatesLong = "-listDelegates";

constexpr auto _getMaximumShadowMapResolution = "-gms";
constexpr auto _getMaximumShadowMapResolutionLong = "-getMaximumShadowMapResolution";

constexpr auto _setMaximumShadowMapResolution = "-sms";
constexpr auto _setMaximumShadowMapResolutionLong = "-setMaximumShadowMapResolution";

constexpr auto _getTextureMemoryPerTexture = "-gtm";
constexpr auto _getTextureMemoryPerTextureLong = "-getTextureMemoryPerTexture";

constexpr auto _setTextureMemoryPerTexture = "-stm";
constexpr auto _setTextureMemoryPerTextureLong = "-setTextureMemoryPerTexture";

constexpr auto _getWireframeSelectionHighlight = "-gwh";
constexpr auto _getWireframeSelectionHighlightLong = "-getWireframeSelectionHighlight";

constexpr auto _setWireframeSelectionHighlight = "-swh";
constexpr auto _setWireframeSelectionHighlightLong = "-setWireframeSelectionHighlight";

constexpr auto _getColorSelectionHighlight = "-gch";
constexpr auto _getColorSelectionHighlightLong = "-getColorSelectionHighlight";

constexpr auto _setColorSelectionHighlight = "-sch";
constexpr auto _setColorSelectionHighlightLong = "-setColorSelectionHighlight";

constexpr auto _getColorSelectionHighlightColor = "-gcc";
constexpr auto _getColorSelectionHighlightColorLong = "-getColorSelectionHighlightColor";

constexpr auto _setColorSelectionHighlightColor = "-scc";
constexpr auto _setColorSelectionHighlightColorLong = "-setColorSelectionHighlightColor";

}

MSyntax HdMayaCmd::createSyntax() {
    MSyntax syntax;

    syntax.addFlag(
        _listRenderers,
        _listRenderersLong);

    syntax.addFlag(
        _getRendererDisplayName,
        _getRendererDisplayNameLong,
        MSyntax::kString);

    syntax.addFlag(
        _changeRenderer,
        _changeRendererLong,
        MSyntax::kString);

    syntax.addFlag(
        _listDelegates,
        _listDelegatesLong);

    syntax.addFlag(
        _getMaximumShadowMapResolution,
        _getMaximumShadowMapResolutionLong);

    syntax.addFlag(
        _setMaximumShadowMapResolution,
        _setMaximumShadowMapResolutionLong,
        MSyntax::kLong);

    syntax.addFlag(
        _getTextureMemoryPerTexture,
        _getTextureMemoryPerTextureLong);

    syntax.addFlag(
        _setTextureMemoryPerTexture,
        _setTextureMemoryPerTextureLong,
        MSyntax::kLong);

    syntax.addFlag(
        _getWireframeSelectionHighlight,
        _getWireframeSelectionHighlightLong);

    syntax.addFlag(
        _setWireframeSelectionHighlight,
        _setWireframeSelectionHighlightLong,
        MSyntax::kBoolean);

    syntax.addFlag(
        _getColorSelectionHighlight,
        _getColorSelectionHighlightLong);

    syntax.addFlag(
        _setColorSelectionHighlight,
        _setColorSelectionHighlightLong,
        MSyntax::kBoolean);

    syntax.addFlag(
        _getColorSelectionHighlightColor,
        _getColorSelectionHighlightColorLong);

    syntax.addFlag(
        _setColorSelectionHighlightColor,
        _setColorSelectionHighlightColorLong,
        MSyntax::kDouble,
        MSyntax::kDouble,
        MSyntax::kDouble,
        MSyntax::kDouble);

    return syntax;
}

MStatus HdMayaCmd::doIt(const MArgList &args) {
    MArgDatabase db(syntax(), args);

    if (db.isFlagSet(_listRenderers)) {
        for (const auto& renderer: HdMayaRenderOverride::GetRendererPlugins()) {
            appendToResult(renderer.GetText());
        }
    } else if (db.isFlagSet(_getRendererDisplayName)) {
        MString id;
        if (db.getFlagArgument(_getRendererDisplayName, 0, id)) {
            const auto dn = HdMayaRenderOverride::GetRendererPluginDisplayName(TfToken(id.asChar()));
            setResult(MString(dn.c_str()));
        }
    } else if (db.isFlagSet(_changeRenderer)) {
        MString id;
        if (db.getFlagArgument(_changeRenderer, 0, id)) {
            HdMayaRenderOverride::ChangeRendererPlugin(TfToken(id.asChar()));
            MGlobal::executeCommandOnIdle("refresh -f");
        }
    } else if (db.isFlagSet(_listDelegates)) {
        for (const auto& delegate: HdMayaDelegateRegistry::GetDelegateNames()) {
            appendToResult(delegate.GetText());
        }
    } else if (db.isFlagSet(_getMaximumShadowMapResolution)) {
        appendToResult(HdMayaRenderOverride::GetMaximumShadowMapResolution());
    } else if (db.isFlagSet(_setMaximumShadowMapResolution)) {
        int res = 32;
        if (db.getFlagArgument(_setMaximumShadowMapResolution, 0, res)) {
            if (res < 32) { res = 32; }
            if (res > 8192) { res = 8192; }
            HdMayaRenderOverride::SetMaximumShadowMapResolution(res);
        }
    } else if (db.isFlagSet(_getTextureMemoryPerTexture)) {
        appendToResult(HdMayaRenderOverride::GetTextureMemoryPerTexture());
    } else if (db.isFlagSet(_setTextureMemoryPerTexture)) {
        int res = 4 * 1024 * 1024;
        if (db.getFlagArgument(_setTextureMemoryPerTexture, 0, res)) {
            if (res < 1024) { res = 1024; }
            if (res > 64 * 1024 * 1024) { res = 64 * 1024 * 1024; }
            HdMayaRenderOverride::SetTextureMemoryPerTexture(res);
        }
    } else if (db.isFlagSet(_getWireframeSelectionHighlight)) {
        appendToResult(HdMayaRenderOverride::GetWireframeSelectionHighlight());
    } else if (db.isFlagSet(_setWireframeSelectionHighlight)) {
        bool res = true;
        if (db.getFlagArgument(_setWireframeSelectionHighlight, 0, res)) {
            HdMayaRenderOverride::SetWireframeSelectionHighlight(res);
        }
    } else if (db.isFlagSet(_getColorSelectionHighlight)) {
        appendToResult(HdMayaRenderOverride::GetColorSelectionHighlight());
    } else if (db.isFlagSet(_setColorSelectionHighlight)) {
        bool res = true;
        if (db.getFlagArgument(_setColorSelectionHighlight, 0, res)) {
            HdMayaRenderOverride::SetColorSelectionHighlight(res);
        }
    } else if (db.isFlagSet(_getColorSelectionHighlightColor)) {
        const auto res = HdMayaRenderOverride::GetColorSelectionHighlightColor();
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
            HdMayaRenderOverride::SetColorSelectionHighlightColor(res);
        }
    }
    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
