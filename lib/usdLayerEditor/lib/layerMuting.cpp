//
// Copyright 2022 Autodesk
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

#include "layerMuting.h"

#include <pxr/base/tf/weakBase.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

namespace {
// The set of muted layers.
//
// Kept in a function to avoid problem with the order of construction
// of global variables in C++.
using MutedLayers = std::set<PXR_NS::SdfLayerRefPtr>;
MutedLayers& getMutedLayers()
{
    // Note: C++ guarantees correct multi-thread protection for static
    //       variables initialization in functions.
    static MutedLayers layers;
    return layers;
}
} // namespace

void loadLayerMuteState(
    const std::vector<std::string>& muted,
    const LayerNameMap&             nameMap,
    PXR_NS::UsdStage&               stage)
{
    auto remapped = muted;

    // Remap the muted layer names in case the layer were renamed when reloaded.
    for (std::string& name : remapped) {
        auto iter = nameMap.find(name);
        if (iter != nameMap.end()) {
            name = iter->second;
        }
    }

    // Add muted layers to the retained muted layer set to avoid losing them.
    // This is necessary because USD only keeps layers in memory if at least one
    // referencing pointer holds it, but muting in the stage makes the stage no
    // longer reference the layer, so the layer would be lost otherwise.
    //
    // Use a set to accelerate lookup of muted layers.
    PXR_NS::SdfLayerHandleVector layers = stage.GetLayerStack();
    std::set<std::string>        mutedSet(remapped.begin(), remapped.end());
    for (const auto& layer : layers) {
        const auto iter = mutedSet.find(layer->GetIdentifier());
        if (iter != mutedSet.end()) {
            addMutedLayer(layer);
        }
    }

    const std::vector<std::string> unmuted;
    stage.MuteAndUnmuteLayers(remapped, unmuted);
}

void addMutedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    // Non-dirty, non-anonymous layers can be reloaded, so we
    // won't hold onto them.
    const bool needHolding = (layer->IsDirty() || layer->IsAnonymous());
    if (!needHolding)
        return;

    MutedLayers& layers = getMutedLayers();
    layers.insert(layer);

    // Hold onto sub-layers as well, in case they are dirty or anonymous.
    // Note: the GetSubLayerPaths function returns proxies, so we have to
    //       hold the std::string by value, not reference.
    for (const std::string subLayerPath : layer->GetSubLayerPaths()) {
        auto subLayer = SdfLayer::FindRelativeToLayer(layer, subLayerPath);
        addMutedLayer(subLayer);
    }
}

void removeMutedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    MutedLayers& layers = getMutedLayers();
    layers.erase(layer);

    // Stop holding onto sub-layers as well, in case they were previously
    // dirty or anonymous.
    //
    // Note: we don't check the dirty or anonymous status while removing
    //       in case the status changed. Trying to remove a layer that
    //       was not held has no consequences.
    //
    // Note: the GetSubLayerPaths function returns proxies, so we have to
    //       hold the std::string by value, not reference.
    for (const std::string subLayerPath : layer->GetSubLayerPaths()) {
        auto subLayer = SdfLayer::FindRelativeToLayer(layer, subLayerPath);
        removeMutedLayer(subLayer);
    }
}

void forgetMutedLayers()
{
    MutedLayers& layers = getMutedLayers();
    layers.clear();
}

} // namespace UsdLayerEditor
