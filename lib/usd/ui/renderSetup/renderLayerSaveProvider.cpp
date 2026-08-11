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

#include "renderLayerSaveProvider.h"

#if defined(ADSK_ABI) && ADSK_ABI >= 2027
#include <AdskUsdRenderSetup/RenderLayerManager.h>

#include <pxr/usd/sdf/layer.h>

#include <algorithm>
#include <set>
#endif

namespace MayaUsdRenderSetup {

namespace {
RenderLayerSaveProvider* _instance = nullptr;
}

/* static */
void RenderLayerSaveProvider::initialize()
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    if (!_instance) {
        _instance = new RenderLayerSaveProvider();
    }
    MayaUsd::utils::setRenderLayerSaveProvider(_instance);
#endif
}

/* static */
void RenderLayerSaveProvider::finalize()
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    MayaUsd::utils::setRenderLayerSaveProvider(nullptr);
    delete _instance;
    _instance = nullptr;
#endif
}

void RenderLayerSaveProvider::getRenderLayersToSave(
    const std::string&                 proxyPath,
    const PXR_NS::UsdStageRefPtr&      stage,
    MayaUsd::utils::StageLayersToSave& layersInfo)
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    if (!stage) {
        return;
    }

    std::set<PXR_NS::SdfLayerRefPtr> alreadyCollected;
    for (const auto& layer : layersInfo._dirtyFileBackedLayers) {
        alreadyCollected.insert(layer);
    }

    auto& manager = AdskUsdRenderSetup::RenderLayerManager::instance();

    for (const auto& renderLayer : manager.trackedRenderLayers(stage)) {
        // The manager retains the handle, so an identifier it reports is resolvable
        // without reopening anything from disk.
        PXR_NS::SdfLayerRefPtr layer = PXR_NS::SdfLayer::Find(renderLayer.identifier);
        if (!layer) {
            continue;
        }

        // The active render layer is composed into the layer stack, so the caller's walk
        // has already collected it. Tag that entry rather than adding a second row for it:
        // without the name, saving would re-path the sub-layer but leave the registry
        // pointing at the old anonymous identifier.
        auto collected = std::find_if(
            layersInfo._anonLayers.begin(),
            layersInfo._anonLayers.end(),
            [&layer](const MayaUsd::utils::LayerInfo& info) { return info.layer == layer; });
        if (collected != layersInfo._anonLayers.end()) {
            collected->parent._renderLayerName = renderLayer.name;
            continue;
        }

        if (alreadyCollected.count(layer) > 0) {
            continue;
        }

        if (layer->IsAnonymous()) {
            MayaUsd::utils::LayerInfo info;
            info.stage = stage;
            info.layer = layer;
            info.parent._proxyPath = proxyPath;
            info.parent._layerParent = nullptr;
            info.parent._renderLayerName = renderLayer.name;

            // Saving happens in this order, and the registry these layers live in is
            // metadata on the root layer, which getLayersToSaveFromProxy appends last.
            // Appending here too would repoint the registry only after the root had
            // already been written, losing the new identifier on disk.
            layersInfo._anonLayers.insert(layersInfo._anonLayers.begin(), info);
        } else if (layer->IsDirty()) {
            // Saved in place, so the identifier does not change and the registry entry
            // stays valid.
            layersInfo._dirtyFileBackedLayers.push_back(layer);
        }
    }
#else
    (void)proxyPath;
    (void)stage;
    (void)layersInfo;
#endif
}

void RenderLayerSaveProvider::onRenderLayerSaved(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            renderLayerName,
    const std::string&            newIdentifier)
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    // Committer-less on purpose. Saving writes a file; undoing the registry repoint
    // would leave the entry pointing at an anonymous identifier whose contents are
    // already on disk under the new path.
    AdskUsdRenderSetup::RenderLayerManager::instance().updateRenderLayerIdentifier(
        stage, renderLayerName, newIdentifier);
#else
    (void)stage;
    (void)renderLayerName;
    (void)newIdentifier;
#endif
}

} // namespace MayaUsdRenderSetup
