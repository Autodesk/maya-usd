//
// Copyright 2026 Autodesk
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

#include "mayaBatchRenderHandler.h"

#include "../mayaRenderSetupHost.h"
#include "mayaBatchRenderResult.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MGlobal.h>
#include <ufe/pathString.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>

#include <algorithm>
#include <sstream>

namespace {
bool isStageDirty(PXR_NS::UsdStageRefPtr stage)
{
    if (!stage)
        return false;

    for (auto layer : stage->GetUsedLayers()) {
        if (layer->IsDirty()) {
            return true;
        }
    }

    return false;
}

// Same command as the mayaBatchRender MEL procedure builds, with extra Render flags appended.
MString batchRenderCommand(const std::string& extraOptions)
{
    MString renderer;
    MGlobal::executeCommand("currentRenderer()", renderer);

    MString optionsProc;
    MGlobal::executeCommand(
        "renderer -q -batchRenderOptionsStringProcedure \"" + renderer + "\"", optionsProc);

    MString rendererOptions;
    if (optionsProc.length() > 0) {
        MGlobal::executeCommand(optionsProc, rendererOptions);
    }

    return "batchRender -renderCommandOptions \"-interactive 1 " + rendererOptions + " "
        + MString(extraOptions.c_str()) + "\"";
}
} // namespace

namespace MayaUsdRenderSetup {

std::shared_ptr<AdskUsdRenderSetup::IRenderResult> MayaBatchRenderHandler::render() const
{
    std::string sessionLayerOptions;
    if (AdskUsdRenderSetup::Host* host = AdskUsdRenderSetup::Host::instance()) {
        AdskUsdRenderSetup::RenderDescription desc = host->activeRenderDescription();
        if (isStageDirty(desc.stage)) {
            int     saveDialogResult = 0;
            MStatus status
                = MGlobal::executeCommand("saveChanges(\"file -save\")", saveDialogResult);
            if (!status || !saveDialogResult) {
                MGlobal::displayInfo(MString("MayaBatchRenderHandler::render() canceled by user."));
                return {};
            }
        }
        // Saving can give an anonymous render layer a file, so this must follow the save.
        sessionLayerOptions = sessionLayerRenderOptions(desc.stage);
    }

    MGlobal::displayInfo(MString("MayaBatchRenderHandler::render() called."));
    if (sessionLayerOptions.empty()) {
        MGlobal::executeCommand(MString("mayaBatchRender"));
    } else {
        MGlobal::executeCommand(batchRenderCommand(sessionLayerOptions));
    }
    return std::make_shared<MayaBatchRenderResult>();
}

/* static */
std::string MayaBatchRenderHandler::sessionLayerRenderOptions(const PXR_NS::UsdStageRefPtr& stage)
{
    if (!stage) {
        return {};
    }

    const auto active
        = AdskUsdRenderSetup::RenderLayerManager::instance().getActiveRenderLayer(stage);
    if (!active) {
        return {};
    }

    // An anonymous layer has no file for the batch process to open; it still reaches the render
    // through the stage session layer serialized in the scene.
    const PXR_NS::SdfLayerHandle layer = PXR_NS::SdfLayer::Find(active->identifier);
    if (!layer || layer->IsAnonymous()) {
        return {};
    }

    // MEL string escapes would consume Windows backslashes.
    std::string layerPath = layer->GetRealPath();
    std::replace(layerPath.begin(), layerPath.end(), '\\', '/');

    std::ostringstream options;
    options << "-sessionLayer \\\"" << layerPath << "\\\"";

    // Without -sessionLayerStage, MayaHydra targets the UsdDefaultRenderDescription stage, which
    // is the right target for a stage that has no proxy shape.
    const Ufe::Path proxyShapePath = MayaUsd::ufe::stagePath(stage);
    if (!proxyShapePath.empty()) {
        options << " -sessionLayerStage \\\"" << Ufe::PathString::string(proxyShapePath) << "\\\"";
    }
    return options.str();
}

} // namespace MayaUsdRenderSetup
