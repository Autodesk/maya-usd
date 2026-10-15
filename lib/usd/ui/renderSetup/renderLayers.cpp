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

#include "renderLayers.h"

#include <mayaUsd/ufe/ProxyShapeHandler.h>

#include <pxr/usd/sdf/layer.h>

#include <AdskUsdRenderSetup/RenderLayerManager.h>

#include <layerEditorDCCFunctions.h>

#include <memory>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MayaUsdRenderSetup {

namespace {
std::unique_ptr<RenderLayers> _instance;

// Tell the layer editor how to find and save render layers, which live in a registry on the root
// layer and are invisible to a layer stack walk. Not registered with the other DCC functions in
// mayaUsd: AdskUsdRenderSetup pulls in Qt widgets, and mayaUsd must stay Qt-free.
void registerRenderLayerFns()
{
    UsdLayerEditor::RenderLayerFns renderLayer;

    renderLayer.getRenderLayersToSave = [](const UsdStageRefPtr& stage) {
        std::vector<SdfLayerRefPtr> renderLayers;
        if (!stage) {
            return renderLayers;
        }

        // Tracked layers are anonymous or dirty and retained by the manager, so Find never
        // reads from disk.
        for (const auto& info :
             AdskUsdRenderSetup::RenderLayerManager::instance().trackedRenderLayers(stage)) {
            if (auto layer = SdfLayer::Find(info.identifier)) {
                renderLayers.push_back(layer);
            }
        }
        return renderLayers;
    };

    renderLayer.renderLayerSaved = [](const UsdStageRefPtr& stage,
                                      const std::string&    oldIdentifier,
                                      const std::string&    newIdentifier) {
        // No undo: saving writes a file, so undoing only the registry repoint would leave it stale.
        auto& manager = AdskUsdRenderSetup::RenderLayerManager::instance();
        for (const auto& info : manager.trackedRenderLayers(stage)) {
            if (info.identifier == oldIdentifier) {
                manager.updateRenderLayerIdentifier(stage, info.name, newIdentifier);
                return;
            }
        }
    };

    UsdLayerEditor::setRenderLayerFns(renderLayer);
}
} // namespace

RenderLayers::RenderLayers()
{
    _stagesChangedKey = TfNotice::Register(TfCreateWeakPtr(this), &RenderLayers::onStagesChanged);
    registerRenderLayerFns();

    // Stages may already exist, for instance when the plugin is loaded into a scene that
    // is already populated.
    attachStages();
}

RenderLayers::~RenderLayers() { TfNotice::Revoke(_stagesChangedKey); }

/* static */
void RenderLayers::initialize()
{
    if (!_instance) {
        _instance = std::make_unique<RenderLayers>();
    }
}

/* static */
void RenderLayers::finalize() { _instance.reset(); }

void RenderLayers::onStagesChanged(const MayaUsdStagesChangedNotice&) { attachStages(); }

// Once attached, a stage's render layers are tracked, and strong references are kept by the
// render layer manager. This way, inactive render layers are not lost.
// Attaching is idempotent, and the manager drops the stages that were destroyed.
void RenderLayers::attachStages()
{
    auto& manager = AdskUsdRenderSetup::RenderLayerManager::instance();
    for (const auto& stage : MayaUsd::ufe::ProxyShapeHandler::getAllStages()) {
        manager.attach(stage);
    }
}

} // namespace MayaUsdRenderSetup
