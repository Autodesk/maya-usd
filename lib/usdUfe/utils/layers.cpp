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
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/ar/resolverContextBinder.h>

#include <deque>

namespace USDUFE_NS_DEF {

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

void getAllSublayers(
    const SdfLayerRefPtr&     layer,
    std::set<std::string>*    layerIds,
    std::set<SdfLayerRefPtr>* layerRefs)
{
    std::deque<SdfLayerRefPtr> processing;
    std::set<SdfLayerRefPtr>   processed;
    processing.push_back(layer);
    while (!processing.empty()) {
        auto layerToProcess = processing.front();
        processing.pop_front();
        if (processed.find(layerToProcess) != processed.end())
            continue;
        processed.insert(layerToProcess);
        SdfSubLayerProxy sublayerPaths = layerToProcess->GetSubLayerPaths();
        for (auto path : sublayerPaths) {
            SdfLayerRefPtr sublayer = SdfLayer::FindRelativeToLayer(layerToProcess, path);
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

StageDirtyState isStageDirty(const PXR_NS::UsdStage& stage)
{
    const bool               includeTopLayer = true;
    std::set<SdfLayerRefPtr> rootLayers = getAllSublayerRefs(stage.GetRootLayer(), includeTopLayer);
    std::set<SdfLayerRefPtr> sessionLayers
        = getAllSublayerRefs(stage.GetSessionLayer(), includeTopLayer);

    SdfLayerHandleVector allLayers = stage.GetUsedLayers(true);
    for (auto layer : allLayers) {
        if (!TF_VERIFY(layer) || !layer->IsDirty())
            continue;

        if (rootLayers.count(layer))
            return StageDirtyState::kDirtyRootLayers;

        if (sessionLayers.count(layer))
            return StageDirtyState::kDirtySessionLayers;
    }

    return StageDirtyState::kClean;
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

int applyToAllPrimSpecs(const UsdPrim& prim, const PrimSpecFunc& func)
{
    int count = 0;

    const SdfPrimSpecHandleVector primStack = getLocalPrimStack(prim);
    for (const SdfPrimSpecHandle& spec : primStack) {
        func(prim, spec);
        ++count;
    }

    return count;
}

int applyToAllLayersWithOpinions(const UsdPrim& prim, PrimLayerFunc& func)
{
    int count = 0;

    const SdfPrimSpecHandleVector primStack = getLocalPrimStack(prim);
    for (const SdfPrimSpecHandle& spec : primStack) {
        const auto layer = spec->GetLayer();
        func(prim, layer);
        ++count;
    }

    return count;
}

int applyToSomeLayersWithOpinions(
    const UsdPrim&                  prim,
    const std::set<SdfLayerRefPtr>& layers,
    PrimLayerFunc&                  func)
{
    int count = 0;

    const SdfPrimSpecHandleVector primStack = getLocalPrimStack(prim);
    for (const SdfPrimSpecHandle& spec : primStack) {
        const auto layer = spec->GetLayer();
        if (layers.count(layer) == 0)
            continue;
        func(prim, layer);
        ++count;
    }

    return count;
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
        SdfLayerRefPtr subLayer = SdfLayer::FindRelativeToLayer(root, path);
        if (!subLayer)
            continue;

        SdfLayerHandle stronger = getStrongerLayer(subLayer, layer1, layer2);
        if (!stronger)
            continue;

        return stronger;
    }

    return SdfLayerHandle();
}

SdfLayerHandle getStrongerLayer(
    const UsdStagePtr&    stage,
    const SdfLayerHandle& layer1,
    const SdfLayerHandle& layer2,
    bool                  compareSessionLayers)
{
    /*
     * Here, the lack of context binder creates bug cases where some sublayers
     * are not found during the recursive part of getStrongerLayer.
    */
    const ArResolverContextBinder binder(stage->GetPathResolverContext());

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

SdfPrimSpecHandleVector
getPrimStackForLayers(const UsdPrim& prim, const SdfLayerHandleVector& layers)
{
    SdfPrimSpecHandleVector primSpecs;

    for (const SdfLayerHandle& layer : layers) {
        const SdfPrimSpecHandle primSpec = layer->GetPrimAtPath(prim.GetPath());
        if (primSpec)
            primSpecs.push_back(primSpec);
    }

    return primSpecs;
}

SdfPrimSpecHandleVector getLocalPrimStack(const UsdPrim& prim)
{
    // The goal is to avoid editing non-local layers. This issue is,
    // for example, that a rename operation would fail when applied
    // to a prim that references a show asset because the rename operation
    // would be attempted on the reference and classes it inherits.
    //
    // Concrete example:
    //     - Create a test asset that inherits from one or more classes
    //     - Create a prim within a DCC Usd scene that references this asset
    //     - Attempt to rename the prim
    //     - Observe the failure due to Sdf policy

    UsdStagePtr stage = prim.GetStage();
    if (!stage)
        return {};

    return getPrimStackForLayers(prim, stage->GetLayerStack());
}

static void addSubLayers(const SdfLayerHandle& layer, std::set<SdfLayerHandle>& layers)
{
    if (!layer)
        return;

    if (layers.count(layer) > 0)
        return;

    layers.insert(layer);
    for (const std::string layerPath : layer->GetSubLayerPaths())
        addSubLayers(SdfLayer::FindOrOpen(layerPath), layers);
}

static bool hasSubLayerInSet(const SdfLayerHandle& layer, const std::set<SdfLayerHandle>& layers)
{
    if (!layer)
        return false;

    for (const std::string layerPath : layer->GetSubLayerPaths())
        if (layers.count(SdfLayer::FindOrOpen(layerPath)) > 0)
            return true;

    return false;
}

SdfPrimSpecHandleVector getDefiningPrimStack(const UsdPrim& prim)
{
    UsdStagePtr stage = prim.GetStage();
    if (!stage)
        return {};

    const SdfPrimSpecHandle defPrimSpec = getDefiningPrimSpec(prim);
    if (!defPrimSpec)
        return {};

    // Simple case: the prim is defined in the local layer stack of the stage.
    {
        const SdfLayerHandle          defLayer = defPrimSpec->GetLayer();
        const SdfPrimSpecHandleVector primSpecsInStageLayers = getLocalPrimStack(prim);
        for (const SdfPrimSpecHandle& primSpec : primSpecsInStageLayers)
            if (primSpec->GetLayer() == defLayer)
                return primSpecsInStageLayers;
    }

    // Complex case: the prim is defined within a reference or payload.
    //
    // We need to build the layer stack of that payload or reference.
    // Note that it could be a reference inside a reference, or a payload
    // in a reference, or any deeper such nesting.
    //
    // We build the defining prim stack by going outward from the defining
    // prim spec. We keep other prim spec if their layer is a parent or child
    // of the layer that defines the prim. (The code beow starts from all the
    // prim specs and removes the ones that are not in the layer hierarchy
    // above and below the defining layer.)

    // This keep tracks of layers we know are in the defining layer stack.
    // We use this to identify other layer, for example identify a parent
    // layer if one of its children in in this set.
    std::set<SdfLayerHandle> definingLayers;
    addSubLayers(defPrimSpec->GetLayer(), definingLayers);

    SdfPrimSpecHandleVector primStack = prim.GetPrimStack();

    const auto defPrimSpecPos = std::find(primStack.begin(), primStack.end(), defPrimSpec);
    if (defPrimSpecPos == primStack.end())
        return {};

    const size_t defPrimSpecIndex = defPrimSpecPos - primStack.begin();

    // Remove the sub-layers that are not in the local stack of the defining layer.
    for (size_t index = defPrimSpecIndex + 1; index < primStack.size(); index += 1) {
        const SdfPrimSpecHandle primSpec = primStack[index];

        // If the prim spec layer is a sub-layer of the defining layer, then we keep
        // it and add its children to the defining layers set.
        SdfLayerHandle layer = primSpec->GetLayer();
        if (definingLayers.count(layer) > 0) {
            addSubLayers(layer, definingLayers);
            continue;
        }

        // Otherwise, we remove the prim spec from the defining prim stack and
        // decrease the index, since we erase it from the vector and the loop
        // will increase the index.
        primStack.erase(primStack.begin() + index);
        index -= 1;
    }

    // Remove the parent layers that are not in the local stack of the defining layer.
    for (size_t index = defPrimSpecIndex; index > 0;) {
        index -= 1;
        const SdfPrimSpecHandle primSpec = primStack[index];

        // If the prim spec layer is a parent layer of the defining layer, then we keep
        // it and add its children to the defining layers set.
        SdfLayerHandle layer = primSpec->GetLayer();
        if (hasSubLayerInSet(layer, definingLayers)) {
            addSubLayers(layer, definingLayers);
            continue;
        }

        // Otherwise, we remove the prim spec from the defining prim stack.
        // We don't need to adjust the index since we are going backward.
        primStack.erase(primStack.begin() + index);
    }

    return primStack;
}

SdfPrimSpecHandle getDefiningPrimSpec(const UsdPrim& prim)
{
    SdfPrimSpecHandleVector primSpecs = prim.GetPrimStack();
    for (SdfPrimSpecHandle primSpec : primSpecs) {
        if (!primSpec)
            continue;

        const SdfSpecifier spec = primSpec->GetSpecifier();
        if (spec != SdfSpecifierDef)
            continue;

        return primSpec;
    }

    return {};
}

const SdfLayerHandle getCurrentTargetLayer(const UsdStagePtr& stage)
{
    if (!stage)
        return {};

    return stage->GetEditTarget().GetLayer();
}

const SdfLayerHandle getCurrentTargetLayer(const UsdPrim& prim)
{
    return getCurrentTargetLayer(prim.GetStage());
}

const std::string getTargetLayerFilePath(const UsdStagePtr& stage)
{
    auto layer = getCurrentTargetLayer(stage);
    if (!layer)
        return {};

    return layer->GetRealPath();
}

const std::string getTargetLayerFilePath(const UsdPrim& prim)
{
    return getTargetLayerFilePath(prim.GetStage());
}

} // namespace USDUFE_NS_DEF
