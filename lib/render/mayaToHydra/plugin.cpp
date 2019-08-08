//
// Copyright 2019 Luma Pictures
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

#include "renderGlobals.h"
#include "renderOverride.h"
#include "usdPreviewSurface.h"
#include "viewCommand.h"

#include <stdlib.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/envSetting.h>

#include "../../usd/hdmaya/adapters/adapter.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(
    MTOH_ENABLE_USD_PREVIEW_SURFACE_NODE, true,
    "Enables the registration of the UsdPreviewSurface node."
    "This is not required with newer version of usdMaya.");

namespace {
bool _enableUsdPreviewSurface = true;
}

PLUGIN_EXPORT MStatus initializePlugin(MObject obj) {
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

    MFnPlugin plugin(obj, "Luma Pictures", "2018", "Any");

    auto* renderer = MHWRender::MRenderer::theRenderer();
    if (renderer) {
        for (const auto& desc : MtohGetRendererDescriptions()) {
            auto* override = new MtohRenderOverride(desc);
            renderer->registerOverride(override);
        }
    }

    if (!plugin.registerCommand(
            MtohViewCmd::name, MtohViewCmd::creator,
            MtohViewCmd::createSyntax)) {
        ret = MS::kFailure;
        ret.perror("Error registering mtoh command!");
        return ret;
    }

    _enableUsdPreviewSurface =
        TfGetEnvSetting(MTOH_ENABLE_USD_PREVIEW_SURFACE_NODE);

    if (_enableUsdPreviewSurface) {
        if (!plugin.registerNode(
                MtohUsdPreviewSurface::name, MtohUsdPreviewSurface::typeId,
                MtohUsdPreviewSurface::Creator,
                MtohUsdPreviewSurface::Initialize, MPxNode::kDependNode,
                &MtohUsdPreviewSurface::classification)) {
            ret = MS::kFailure;
            ret.perror("Error registering UsdPreviewSurface node!");
            return ret;
        }
    }

    MtohInitializeRenderGlobals();

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

    if (!plugin.deregisterCommand(MtohViewCmd::name)) {
        ret = MS::kFailure;
        ret.perror("Error deregistering mtoh command!");
    }

    if (_enableUsdPreviewSurface) {
        if (!plugin.deregisterNode(MtohUsdPreviewSurface::typeId)) {
            ret = MS::kFailure;
            ret.perror("Error deregistering UsdPreviewSurface node!");
        }
    }

    return ret;
}
