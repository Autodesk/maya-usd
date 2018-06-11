#include "cmd.h"

#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include "viewportRenderer.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    constexpr auto _listRenderers = "-ls";
    constexpr auto _listRenderersLong = "-listRenderers";
}


MSyntax HdMayaCmd::createSyntax() {
    MSyntax syntax;

    syntax.addFlag(
        _listRenderers,
        _listRenderersLong);

    return syntax;
}

MStatus HdMayaCmd::doIt(const MArgList &args) {
    MArgDatabase db(syntax(), args);

    if (db.isFlagSet(_listRenderers)) {
        for (const auto& renderer: HdMayaViewportRenderer::getRendererPlugins()) {
            appendToResult(renderer.GetText());
        }
    }
    return MS::kSuccess;
}