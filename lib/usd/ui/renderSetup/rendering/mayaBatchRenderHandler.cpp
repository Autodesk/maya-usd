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

#include "mayaBatchRender.h"
#include "mayaBatchRenderResult.h"

#include <mayaUsdUI/ui/stringResources.h>

#include <maya/MGlobal.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>

namespace MayaUsdRenderSetup {

std::string MayaBatchRenderHandler::displayName() const
{
    const AdskUsdRenderSetup::Host* host = AdskUsdRenderSetup::Host::instance();
    return displayNameFor(
        host ? host->activeRenderDescription() : AdskUsdRenderSetup::RenderDescription {});
}

std::string MayaBatchRenderHandler::toolTip() const
{
    return GET_MAYAUSDUI_STR(kBatchRenderTargetToolTip);
}

std::shared_ptr<AdskUsdRenderSetup::IRenderResult> MayaBatchRenderHandler::render() const
{
    std::string options;
    if (AdskUsdRenderSetup::Host* host = AdskUsdRenderSetup::Host::instance()) {
        AdskUsdRenderSetup::RenderDescription desc = host->activeRenderDescription();
        if (!saveForBatchRender(desc.stage)) {
            return {};
        }
        // Saving can give an anonymous render layer a file, so this must follow the save.
        options = sessionLayerRenderOptions(desc.stage);
    }

    MGlobal::displayInfo(MString("MayaBatchRenderHandler::render() called."));
    MGlobal::executeCommand(batchRenderCommand(options));
    return std::make_shared<MayaBatchRenderResult>();
}

/* static */
std::string
MayaBatchRenderHandler::displayNameFor(const AdskUsdRenderSetup::RenderDescription& desc)
{
    std::string target;
    if (desc.stage) {
        if (const auto active
            = AdskUsdRenderSetup::RenderLayerManager::instance().getActiveRenderLayer(desc.stage)) {
            target = active->name;
        }
    }
    if (target.empty()) {
        target = desc.path.GetName();
    }
    if (target.empty()) {
        return GET_MAYAUSDUI_STR(kBatchRender);
    }

    MString label;
    label.format(GET_MAYAUSDUI_MSTR(kBatchRenderTarget), MString(target.c_str()));
    return label.asChar();
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
    const std::string layerPath = sessionLayerPath(*active);
    // Without the flags, the active layer still reaches the render through the stage session
    // layer serialized in the scene.
    if (layerPath.empty() || !canPassInMelString(layerPath)) {
        return {};
    }
    return sessionLayerOptions(stage, layerPath);
}

} // namespace MayaUsdRenderSetup
