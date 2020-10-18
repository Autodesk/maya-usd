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
#include <stdio.h>
#include <stdlib.h>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/envSetting.h>

#include <mayaUsd/utils/plugRegistryHelper.h>

#include <hdMaya/adapters/adapter.h>

#include "renderGlobals.h"
#include "renderOverride.h"
#include "viewCommand.h"

PXR_NAMESPACE_USING_DIRECTIVE

PLUGIN_EXPORT MStatus initializePlugin(MObject obj) {
    MStatus ret = MS::kSuccess;

    // Call one time registration of plugins compiled for same USD version as MayaUSD plugin.
    MAYAUSD_NS::registerVersionedPlugins();
    
    ret = HdMayaAdapter::Initialize();
    if (!ret) { return ret; }

    // For now this is required for the HdSt backed to use lights.
    // putenv requires char* and I'm not willing to use const cast!
    constexpr const char* envVarSet = "USDIMAGING_ENABLE_SCENE_LIGHTS=1";
    const auto envVarSize = strlen(envVarSet) + 1;
    std::vector<char> envVarData;
    envVarData.resize(envVarSize);
    snprintf(envVarData.data(), envVarSize, "%s", envVarSet);
    putenv(envVarData.data());

    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");

    if (!plugin.registerCommand(
            MtohViewCmd::name, MtohViewCmd::creator,
            MtohViewCmd::createSyntax)) {
        ret = MS::kFailure;
        ret.perror("Error registering mtoh command!");
        return ret;
    }

    if (auto* renderer = MHWRender::MRenderer::theRenderer()) {
        for (const auto& desc : MtohGetRendererDescriptions()) {
            std::unique_ptr<MtohRenderOverride> mtohRenderer(new MtohRenderOverride(desc));
            renderer->registerOverride(mtohRenderer.get());
            // registerOverride took the pointer, so release ownership
            mtohRenderer.release();
        }
    }

    return ret;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj) {
    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");
    MStatus ret = MS::kSuccess;

    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        for (const auto& desc : MtohGetRendererDescriptions()) {
            const auto* override =
                renderer->findRenderOverride(desc.overrideName.GetText());
            if (override) {
                renderer->deregisterOverride(override);
                delete override;
            }
        }
    }

    // Clear any registered callbacks
    MGlobal::executeCommand("callbacks -cc mtoh;");

    if (!plugin.deregisterCommand(MtohViewCmd::name)) {
        ret = MS::kFailure;
        ret.perror("Error deregistering mtoh command!");
    }

    return ret;
}
