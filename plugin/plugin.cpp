#include <maya/MFnPlugin.h>

#include "viewportRenderer.h"
#include "renderOverride.h"
#include "renderOverride.h"
#include "cmd.h"

#include <stdlib.h>

PXR_NAMESPACE_USING_DIRECTIVE

MStatus initializePlugin(MObject obj) {
    // For now this is required for the HdSt backed to use lights.
    // putenv requires char* and I'm not willing to use const cast!
    constexpr const char* envVarSet = "USDIMAGING_ENABLE_SCENE_LIGHTS=1";
    std::vector<char> envVarData; envVarData.resize(strlen(envVarSet) + 1);
    sprintf(envVarData.data(), "%s", envVarSet);
    putenv(envVarData.data());

    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");
    MStatus ret = MS::kSuccess;

    auto* vp = HdMayaViewportRenderer::GetInstance();
    if (vp && !vp->registerRenderer()) {
        ret = MS::kFailure;
        ret.perror("Error registering hd viewport renderer!");
        HdMayaViewportRenderer::Cleanup();
    }

    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        auto& override = HdMayaRenderOverride::GetInstance();
        renderer->registerOverride(&override);
    }

    if (!plugin.registerCommand(HdMayaCmd::name, HdMayaCmd::creator, HdMayaCmd::createSyntax)) {
        ret = MS::kFailure;
        ret.perror("Error registering hdmaya command!");
    }

    return ret;
}


MStatus uninitializePlugin(MObject obj) {
    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");
    MStatus ret = MS::kSuccess;

    auto* vp = HdMayaViewportRenderer::GetInstance();
    if (vp && vp->deregisterRenderer()) {
        ret = MS::kFailure;
        ret.perror("Error deregistering hd viewport renderer!");
    }
    HdMayaViewportRenderer::Cleanup();

    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        const auto* override = renderer->findRenderOverride("hydraViewportOverride");
        if (override) {
            renderer->deregisterOverride(override);
            HdMayaRenderOverride::DeleteInstance();
        }
    }

    if (!plugin.deregisterCommand(HdMayaCmd::name)) {
        ret = MS::kFailure;
        ret.perror("Error deregistering hdmaya command!");
    }

    return ret;
}
