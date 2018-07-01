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
            HdMayaRenderOverride::SetMaximumShadowMapResolution(res);
        }
    }
    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
