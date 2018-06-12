#include "cmd.h"

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include "viewportRenderer.h"

PXR_NAMESPACE_USING_DIRECTIVE

const MString HdMayaCmd::name("hdmaya");

namespace {
    constexpr auto _listRenderers = "-ls";
    constexpr auto _listRenderersLong = "-listRenderers";

    constexpr auto _getRendererDisplayName = "-gn";
    constexpr auto _getRendererDisplayNameLong = "-getRendererDisplayName";

    constexpr auto _changeRenderer = "-cr";
    constexpr auto _changeRendererLong = "-changeRenderer";
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

    return syntax;
}

MStatus HdMayaCmd::doIt(const MArgList &args) {
    MArgDatabase db(syntax(), args);

    if (db.isFlagSet(_listRenderers)) {
        for (const auto& renderer: HdMayaViewportRenderer::getRendererPlugins()) {
            appendToResult(renderer.GetText());
        }
    } else if (db.isFlagSet(_getRendererDisplayName)) {
        MString id;
        if (db.getFlagArgument(_getRendererDisplayName, 0, id)) {
            const auto dn = HdMayaViewportRenderer::getRendererPluginDisplayName(TfToken(id.asChar()));
            setResult(MString(dn.c_str()));
        }
    } else if (db.isFlagSet(_changeRenderer)) {
        MString id;
        if (db.getFlagArgument(_getRendererDisplayName, 0, id)) {
            HdMayaViewportRenderer::changeRendererPlugin(TfToken(id.asChar()));
        }
    }
    return MS::kSuccess;
}
