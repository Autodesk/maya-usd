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
#include <maya/MFnPlugin.h>

#include "cmd.h"
#include "renderOverride.h"
#include "usdPreviewSurface.h"

#include <stdlib.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/usd/ar/defaultResolver.h>

#include <hdmaya/adapters/adapter.h>

PXR_NAMESPACE_USING_DIRECTIVE

MStatus initializePlugin(MObject obj) {
    MStatus ret = MS::kSuccess;

    ret = HdMayaAdapter::Initialize();
    if (!ret) { return ret; }

    // For now this is required for the HdSt backed to use lights.
    // putenv requires char* and I'm not willing to use const cast!
    constexpr const char* envVarSet = "USDIMAGING_ENABLE_SCENE_LIGHTS=1";
    std::vector<char> envVarData;
    envVarData.resize(strlen(envVarSet) + 1);
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

    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        auto& override = HdMayaRenderOverride::GetInstance();
        renderer->registerOverride(&override);
    }

    if (!plugin.registerCommand(HdMayaCmd::name, HdMayaCmd::creator, HdMayaCmd::createSyntax)) {
        ret = MS::kFailure;
        ret.perror("Error registering hdmaya command!");
        return ret;
    }

    if (!plugin.registerNode(
            HdMayaUsdPreviewSurface::name, HdMayaUsdPreviewSurface::typeId,
            HdMayaUsdPreviewSurface::Creator, HdMayaUsdPreviewSurface::Initialize,
            MPxNode::kDependNode, &HdMayaUsdPreviewSurface::classification)) {
        ret = MS::kFailure;
        ret.perror("Error registering UsdPreviewSurface node!");
        return ret;
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

    if (!plugin.deregisterNode(HdMayaUsdPreviewSurface::typeId)) {
        ret = MS::kFailure;
        ret.perror("Error deregistering UsdPreviewSurface node!");
    }

    return ret;
}
