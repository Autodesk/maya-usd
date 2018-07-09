#include <maya/MFnPlugin.h>

#include "renderOverride.h"
#include "cmd.h"

#include <stdlib.h>

#include <pxr/base/plug/registry.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/usd/ar/defaultResolver.h>

PXR_NAMESPACE_USING_DIRECTIVE

MStatus initializePlugin(MObject obj) {
    // For now this is required for the HdSt backed to use lights.
    // putenv requires char* and I'm not willing to use const cast!
    constexpr const char* envVarSet = "USDIMAGING_ENABLE_SCENE_LIGHTS=1";
    std::vector<char> envVarData; envVarData.resize(strlen(envVarSet) + 1);
    sprintf(envVarData.data(), "%s", envVarSet);
    putenv(envVarData.data());

    std::vector<std::string> defaultSearchPaths;
    // We are trying to setup the default search paths for shaders
    // to simplify finding shader paths, like the preview surface.
    for (auto plug : PlugRegistry::GetInstance().GetAllPlugins()) {
        const auto metadata = plug->GetMetadata();
        if (metadata.find("ShaderResources") != metadata.end()) {
            defaultSearchPaths.push_back(plug->GetResourcePath());
        }
    }
    std::sort(defaultSearchPaths.begin(), defaultSearchPaths.end());
    ArDefaultResolver::SetDefaultSearchPath(defaultSearchPaths);

    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");
    MStatus ret = MS::kSuccess;

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
