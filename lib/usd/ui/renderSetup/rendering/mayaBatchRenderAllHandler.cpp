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

#include "mayaBatchRenderAllHandler.h"

#include "mayaBatchRender.h"
#include "mayaBatchRenderQueue.h"
#include "mayaBatchRenderResult.h"

#include <mayaUsdUI/ui/stringResources.h>

#include <maya/MGlobal.h>

#include <AdskUsdRenderSetup/Host.h>
#include <AdskUsdRenderSetup/RenderLayerManager.h>

#include <utility>

namespace MayaUsdRenderSetup {

std::string MayaBatchRenderAllHandler::displayName() const
{
    return GET_MAYAUSDUI_STR(kBatchRenderAll);
}

std::string MayaBatchRenderAllHandler::toolTip() const
{
    return GET_MAYAUSDUI_STR(kBatchRenderAllToolTip);
}

std::shared_ptr<AdskUsdRenderSetup::IRenderResult> MayaBatchRenderAllHandler::render() const
{
    const AdskUsdRenderSetup::Host* host = AdskUsdRenderSetup::Host::instance();
    const PXR_NS::UsdStageRefPtr    stage = host ? host->activeRenderDescription().stage : nullptr;
    if (!saveForBatchRender(stage)) {
        return {};
    }

    // Saving can give an anonymous render layer a file, so this must follow the save.
    std::vector<MString> commands = batchRenderCommands(stage);
    if (commands.empty()) {
        MGlobal::displayWarning(GET_MAYAUSDUI_MSTR(kBatchRenderNothingRenderable));
        return {};
    }

    MayaBatchRenderQueue::instance().start(std::move(commands));
    return std::make_shared<MayaBatchRenderResult>();
}

/* static */
std::vector<MString>
MayaBatchRenderAllHandler::batchRenderCommands(const PXR_NS::UsdStageRefPtr& stage)
{
    std::vector<MString> commands;
    if (!stage) {
        return commands;
    }

    for (const auto& layer :
         AdskUsdRenderSetup::RenderLayerManager::instance().getIncludedRenderLayers(stage)) {
        const std::string sessionOptions = sessionLayerOptions(stage, layer);
        // Without its own pass, the layer would render whatever description the scene saved.
        const std::string activeOptions = activeRenderDescriptionOptions(stage, layer);
        if (sessionOptions.empty() || activeOptions.empty()) {
            MString warning;
            warning.format(
                sessionOptions.empty() ? GET_MAYAUSDUI_MSTR(kBatchRenderLayerNotSaved)
                                       : GET_MAYAUSDUI_MSTR(kBatchRenderLayerNoPass),
                MString(layer.name.c_str()));
            MGlobal::displayWarning(warning);
            continue;
        }
        // MayaHydra requires -activeRenderDescription before the other render settings flags.
        commands.push_back(batchRenderCommand(activeOptions + " " + sessionOptions));
    }
    return commands;
}

} // namespace MayaUsdRenderSetup
