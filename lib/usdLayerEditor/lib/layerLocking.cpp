//
// Copyright 2024 Autodesk
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

#include "layerLocking.h"


namespace UsdLayerEditor {

void loadLayerLockState(
    const std::vector<std::string>& locked,
    const LayerNameMap&             nameMap,
    PXR_NS::UsdStage&               stage)
{
    auto remapped = locked;

    // Remap the locked layer names in case the layers were renamed when reloaded.
    for (std::string& name : remapped) {
        auto iter = nameMap.find(name);
        if (iter != nameMap.end()) {
            name = iter->second;
        }
    }

    // Add locked layers to the retained locked layer set to avoid losing them.
    // This is necessary because USD only keeps layers in memory if at least one
    // referencing pointer holds it, but locking in the stage makes the stage no
    // longer reference the layer, so the layer would be lost otherwise.
    //
    // Use a set to accelerate lookup of locked layers.
    PXR_NS::SdfLayerHandleVector layers = stage.GetLayerStack();
    std::set<std::string>        lockedSet(remapped.begin(), remapped.end());
    for (const auto& layer : layers) {
        const auto iter = lockedSet.find(layer->GetIdentifier());
        if (iter != lockedSet.end()) {
            std::string emptyShapePath;
            lockLayer(emptyShapePath, layer, LayerLockType::LayerLock_Locked, false);
        }
    }
}

void lockLayer(
    std::string                   dccObjectPath,
    const PXR_NS::SdfLayerRefPtr& layer,
    LayerLockType                 locktype,
    bool                          updateDCCObjectAttr /*= true */)
{
    switch (locktype) {
    default:
    case LayerLock_Unlocked: {
        layer->SetPermissionToEdit(true);
        layer->SetPermissionToSave(true);
        removeLockedLayer(layer);
        removeSystemLockedLayer(layer);
        break;
    }
    case LayerLock_Locked: {
        layer->SetPermissionToEdit(false);
        layer->SetPermissionToSave(true);
        addLockedLayer(layer);
        removeSystemLockedLayer(layer);
        break;
    }
    case LayerLock_SystemLocked: {
        layer->SetPermissionToSave(false);
        layer->SetPermissionToEdit(false);
        addSystemLockedLayer(layer);
        removeLockedLayer(layer);
        break;
    }
    }
}

// The set of locked layers.
//
// Kept in a function to avoid problem with the order of construction
// of global variables in C++.

LockedLayers& getLockedLayers()
{
    // Note: C++ guarantees correct multi-thread protection for static
    //       variables initialization in functions.
    static LockedLayers layers;
    return layers;
}

std::vector<std::string> getLockedLayersIdentifiers()
{
    LockedLayers& lockedLayers = getLockedLayers();

    std::vector<std::string> identifiers;
    identifiers.reserve(lockedLayers.size());

    for (auto layer : lockedLayers) {
        identifiers.emplace_back(layer->GetIdentifier());
    }

    return identifiers;
}

LockedLayers& getSystemLockedLayers()
{
    // Note: C++ guarantees correct multi-thread protection for static
    //       variables initialization in functions.
    static LockedLayers layers;
    return layers;
}

void addLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getLockedLayers();
    layers.insert(layer);
}

void removeLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getLockedLayers();
    layers.erase(layer);
}

bool isLayerLocked(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return false;

    LockedLayers& layers = getLockedLayers();
    auto          iter = layers.find(layer);
    if (iter != layers.end()) {
        return true;
    }
    return false;
}

void forgetLockedLayers()
{
    LockedLayers& layers = getLockedLayers();
    layers.clear();
}

void addSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getSystemLockedLayers();
    layers.insert(layer);
}

void removeSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getSystemLockedLayers();
    layers.erase(layer);
}

bool isLayerSystemLocked(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return false;

    LockedLayers& layers = getSystemLockedLayers();
    auto          iter = layers.find(layer);
    if (iter != layers.end()) {
        return true;
    }
    return false;
}

void forgetSystemLockedLayers()
{
    LockedLayers& layers = getSystemLockedLayers();
    layers.clear();
}

} // namespace UsdLayerEditor
