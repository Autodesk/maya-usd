//
// Copyright 2023 Autodesk
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
#include "layers.h"

#include <pxr/usd/pcp/layerStack.h>

#include <deque>

namespace MAYAUSD_NS_DEF {

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

void getAllSublayers(
    const SdfLayerRefPtr&     layer,
    std::set<std::string>*    layerIds,
    std::set<SdfLayerRefPtr>* layerRefs)
{
    std::deque<SdfLayerRefPtr> processing;
    processing.push_back(layer);
    while (!processing.empty()) {
        auto layerToProcess = processing.front();
        processing.pop_front();
        SdfSubLayerProxy sublayerPaths = layerToProcess->GetSubLayerPaths();
        for (auto path : sublayerPaths) {
            SdfLayerRefPtr sublayer = SdfLayer::FindOrOpen(path);
            if (sublayer) {
                if (layerIds)
                    layerIds->insert(path);
                if (layerRefs)
                    layerRefs->insert(sublayer);
                processing.push_back(sublayer);
            }
        }
    }
}

} // namespace

std::set<std::string> getAllSublayers(const SdfLayerRefPtr& layer)
{
    std::set<std::string> allSublayers;
    getAllSublayers(layer, &allSublayers, nullptr);
    return allSublayers;
}

std::set<SdfLayerRefPtr> getAllSublayerRefs(const SdfLayerRefPtr& layer, bool includeTopLayer)
{
    std::set<SdfLayerRefPtr> allSublayers;
    getAllSublayers(layer, nullptr, &allSublayers);
    if (includeTopLayer)
        allSublayers.insert(layer);
    return allSublayers;
}

std::set<std::string>
getAllSublayers(const std::vector<std::string>& layerPaths, bool includeParents)
{
    std::set<std::string> layers;

    for (auto layerPath : layerPaths) {
        SdfLayerRefPtr layer = SdfLayer::Find(layerPath);
        if (layer) {
            if (includeParents)
                layers.insert(layerPath);
            auto sublayerPaths = getAllSublayers(layer);
            std::move(sublayerPaths.begin(), sublayerPaths.end(), inserter(layers, layers.end()));
        }
    }

    return layers;
}

bool hasMutedLayer(const PXR_NS::UsdPrim& prim)
{
    const PXR_NS::PcpPrimIndex& primIndex = prim.GetPrimIndex();

    for (const PXR_NS::PcpNodeRef node : primIndex.GetNodeRange()) {
        if (!node)
            continue;

        const PXR_NS::PcpLayerStackRefPtr& layerStack = node.GetSite().layerStack;
        if (!layerStack)
            continue;

        const std::set<std::string>& mutedLayers = layerStack->GetMutedLayers();
        if (mutedLayers.size() > 0)
            return true;
    }
    return false;
}

void enforceMutedLayer(const PXR_NS::UsdPrim& prim, const char* command)
{
    if (hasMutedLayer(prim)) {
        const std::string error = TfStringPrintf(
            "Cannot %s prim \"%s\" because there is at least one muted layer.",
            command && command[0] ? command : "modify",
            prim.GetPath().GetText());
        TF_WARN("%s", error.c_str());
        throw std::runtime_error(error);
    }
}

void applyToAllPrimSpecs(const UsdPrim& prim, const PrimSpecFunc& func)
{
    const SdfPrimSpecHandleVector primStack = prim.GetPrimStack();
    for (const SdfPrimSpecHandle& spec : primStack)
        func(prim, spec);
}

void applyToAllLayersWithOpinions(const UsdPrim& prim, PrimLayerFunc& func)
{
    const SdfPrimSpecHandleVector primStack = prim.GetPrimStack();
    for (const SdfPrimSpecHandle& spec : primStack) {
        const auto layer = spec->GetLayer();
        func(prim, layer);
    }
}

void applyToSomeLayersWithOpinions(
    const UsdPrim&                  prim,
    const std::set<SdfLayerRefPtr>& layers,
    PrimLayerFunc&                  func)
{
    const SdfPrimSpecHandleVector primStack = prim.GetPrimStack();
    for (const SdfPrimSpecHandle& spec : primStack) {
        const auto layer = spec->GetLayer();
        if (layers.count(layer) == 0)
            continue;
        func(prim, layer);
    }
}

bool isLayerInStage(const PXR_NS::SdfLayerHandle& layer, const PXR_NS::UsdStage& stage)
{
    for (const auto& stageLayer : stage.GetLayerStack())
        if (stageLayer == layer)
            return true;
    return false;
}

bool isSessionLayer(const SdfLayerHandle& layer, const std::set<SdfLayerRefPtr>& sessionLayers)
{
    return sessionLayers.count(layer) > 0;
}

SdfLayerHandle getStrongerLayer(
    const SdfLayerHandle& root,
    const SdfLayerHandle& layer1,
    const SdfLayerHandle& layer2)
{
    if (layer1 == layer2)
        return layer1;

    if (!layer1)
        return layer2;

    if (!layer2)
        return layer1;

    if (root == layer1)
        return layer1;

    if (root == layer2)
        return layer2;

    for (auto path : root->GetSubLayerPaths()) {
        SdfLayerRefPtr subLayer = SdfLayer::FindOrOpen(path);
        if (subLayer) {
            SdfLayerHandle stronger = getStrongerLayer(subLayer, layer1, layer2);
            if (!stronger.IsInvalid()) {
                return stronger;
            }
        }
    }

    return SdfLayerHandle();
}

SdfLayerHandle getStrongerLayer(
    const UsdStagePtr&    stage,
    const SdfLayerHandle& layer1,
    const SdfLayerHandle& layer2,
    bool                  compareSessionLayers)
{
    if (compareSessionLayers) {
        // Session Layer is the strongest in the stage, so check its hierarchy first
        // when enabled.
        auto strongerLayer = getStrongerLayer(stage->GetSessionLayer(), layer1, layer2);
        if (strongerLayer == layer1) {
            return layer1;
        } else if (strongerLayer == layer2) {
            return layer2;
        }
    }

    // Only verify the stage's general layer hierarchy. Do not check the session
    // layer hierarchy because we don't want to let opinions that are owned by
    // the application interfere with the user commands.
    return getStrongerLayer(stage->GetRootLayer(), layer1, layer2);
}

} // namespace MAYAUSD_NS_DEF