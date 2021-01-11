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
#include "renderGlobals.h"
#include "renderOverride.h"
#include "viewCommand.h"

#include <hdMaya/adapters/adapter.h>
#include <mayaUsd/utils/plugRegistryHelper.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/envSetting.h>

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <vector>

using MtohRenderOverridePtr = std::unique_ptr<MtohRenderOverride>;
static std::vector<MtohRenderOverridePtr> gsRenderOverrides;

#if defined(MAYAUSD_VERSION)
#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)
#else
#error "MAYAUSD_VERSION is not defined"
#endif

PXR_NAMESPACE_USING_DIRECTIVE

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
    MStatus ret = MS::kSuccess;

    // Call one time registration of plugins compiled for same USD version as MayaUSD plugin.
    MayaUsd::registerVersionedPlugins();

    ret = HdMayaAdapter::Initialize();
    if (!ret) {
        return ret;
    }

    // For now this is required for the HdSt backed to use lights.
    // putenv requires char* and I'm not willing to use const cast!
    constexpr const char* envVarSet = "USDIMAGING_ENABLE_SCENE_LIGHTS=1";
    const auto            envVarSize = strlen(envVarSet) + 1;
    std::vector<char>     envVarData;
    envVarData.resize(envVarSize);
    snprintf(envVarData.data(), envVarSize, "%s", envVarSet);
    putenv(envVarData.data());

    MFnPlugin plugin(obj, "Autodesk", TOSTRING(MAYAUSD_VERSION), "Any");

    if (!plugin.registerCommand(
            MtohViewCmd::name, MtohViewCmd::creator, MtohViewCmd::createSyntax)) {
        ret = MS::kFailure;
        ret.perror("Error registering mtoh command!");
        return ret;
    }

    if (auto* renderer = MHWRender::MRenderer::theRenderer()) {
        for (const auto& desc : MtohGetRendererDescriptions()) {
            MtohRenderOverridePtr mtohRenderer(new MtohRenderOverride(desc));
            MStatus status = renderer->registerOverride(mtohRenderer.get());
            if (status == MS::kSuccess) {
                gsRenderOverrides.push_back(std::move(mtohRenderer));
            } 
            else mtohRenderer = nullptr; 
        }
    }

    return ret;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, "Autodesk", TOSTRING(MAYAUSD_VERSION), "Any");
    MStatus   ret = MS::kSuccess;
    if (auto* renderer = MHWRender::MRenderer::theRenderer()) {
        for (int i = 0; i < gsRenderOverrides.size(); i++) {
            renderer->deregisterOverride(gsRenderOverrides[i].get());
            gsRenderOverrides[i] = nullptr;
        }
    }
    gsRenderOverrides.clear();

    // Clear any registered callbacks
    MGlobal::executeCommand("callbacks -cc mtoh;");

    if (!plugin.deregisterCommand(MtohViewCmd::name)) {
        ret = MS::kFailure;
        ret.perror("Error deregistering mtoh command!");
    }

    return ret;
}
