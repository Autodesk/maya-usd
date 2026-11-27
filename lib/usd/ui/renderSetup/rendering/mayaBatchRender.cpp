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

#include "mayaBatchRender.h"

#include <mayaUsd/ufe/Utils.h>
#include <usdLayerEditor/lib/layerEditorDCCFunctions.h>

#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/sdf/layer.h>

#include <maya/MGlobal.h>
#include <ufe/pathString.h>

#include <filesystem>

namespace {
// A Render flag and its value, quoted for the MEL string literal the flags are passed in.
std::string renderFlag(const char* flag, const std::string& value)
{
    return std::string(flag) + " \\\"" + value + "\\\"";
}

bool isStageDirty(const PXR_NS::UsdStageRefPtr& stage)
{
    if (!stage)
        return false;

    for (auto layer : stage->GetUsedLayers()) {
        if (layer->IsDirty()) {
            return true;
        }
    }
    // Inactive render layers are absent from GetUsedLayers, yet "render all" renders them.
    for (const auto& layer : UsdLayerEditor::getRenderLayersToSave(stage)) {
        if (layer && layer->IsDirty()) {
            return true;
        }
    }

    return false;
}
} // namespace

namespace MayaUsdRenderSetup {

bool saveForBatchRender(const PXR_NS::UsdStageRefPtr& stage)
{
    if (!isStageDirty(stage)) {
        return true;
    }

    int     saveDialogResult = 0;
    MStatus status = MGlobal::executeCommand("saveChanges(\"file -save\")", saveDialogResult);
    if (!status || !saveDialogResult) {
        MGlobal::displayInfo(MString("Batch render canceled by user."));
        return false;
    }
    return true;
}

// Same command the mayaBatchRender MEL procedure builds for non-mental-ray renderers, with extra
// Render flags appended.
MString batchRenderCommand(const std::string& extraOptions, const std::string& sceneFile)
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

    MString command = "batchRender ";
    if (!sceneFile.empty()) {
        command += "-filename \"" + MString(sceneFile.c_str()) + "\" ";
    }
    return command + "-renderCommandOptions \"-interactive 1 " + rendererOptions + " "
        + MString(extraOptions.c_str()) + "\"";
}

std::string activeRenderDescriptionOptions(
    const PXR_NS::UsdStageRefPtr&              stage,
    const AdskUsdRenderSetup::RenderLayerInfo& renderLayer)
{
    if (renderLayer.renderPassPath.IsEmpty()) {
        return {};
    }

    const Ufe::Path stagePath = MayaUsd::ufe::stagePath(stage);
    if (stagePath.empty()) {
        return {};
    }

    return renderFlag(
        "-activeRenderDescription",
        Ufe::PathString::string(
            stagePath + UsdUfe::usdPathToUfePathSegment(renderLayer.renderPassPath)));
}

std::string sessionLayerOptions(
    const PXR_NS::UsdStageRefPtr&              stage,
    const AdskUsdRenderSetup::RenderLayerInfo& renderLayer)
{
    // An anonymous layer has no file for the batch process to open; it still reaches the render
    // through the stage session layer serialized in the scene.
    // Opens the layer when it is not loaded, as for a render layer that is not active.
    const PXR_NS::SdfLayerRefPtr layer = PXR_NS::SdfLayer::FindOrOpen(renderLayer.identifier);
    if (!layer || layer->IsAnonymous()) {
        return {};
    }

    // MEL string escapes would consume Windows backslashes.
    const std::string layerPath = std::filesystem::path(layer->GetRealPath()).generic_string();
    // A quote would end the MEL string the path is passed in.
    if (layerPath.find('"') != std::string::npos) {
        return {};
    }

    std::string options = renderFlag("-sessionLayer", layerPath);

    // Without -sessionLayerStage, MayaHydra applies the session layer to the
    // UsdDefaultRenderDescription stage.
    const Ufe::Path stagePath = MayaUsd::ufe::stagePath(stage);
    if (!stagePath.empty()) {
        options += " " + renderFlag("-sessionLayerStage", Ufe::PathString::string(stagePath));
    }
    return options;
}

} // namespace MayaUsdRenderSetup
