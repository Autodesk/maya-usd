//
// Copyright 2020 Autodesk
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

#include "util.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MayaUsdUtils {

SdfLayerHandle
defPrimSpecLayer(const UsdPrim& prim)
{
    // Iterate over the layer stack, starting at the highest-priority layer.
    // The source layer is the one in which there exists a def primSpec, not
    // an over.

    SdfLayerHandle defLayer;
    auto layerStack = prim.GetStage()->GetLayerStack();

    for (auto layer : layerStack) {
        auto primSpec = layer->GetPrimAtPath(prim.GetPath());
        if (primSpec && (primSpec->GetSpecifier() == SdfSpecifierDef)) {
            defLayer = layer;
            break;
        }
    }
    return defLayer;
}

std::vector<SdfLayerHandle>
layersWithPrimSpec(const UsdPrim& prim)
{
    // get the list of PrimSpecs that provide opinions for this prim
    // ordered from strongest to weakest.
    const auto& primStack = prim.GetPrimStack();

    std::vector<SdfLayerHandle> layersWithPrimSpec;
    for (auto primSpec : primStack) {
        layersWithPrimSpec.emplace_back(primSpec->GetLayer());
    }

    return layersWithPrimSpec;
}

bool
doesEditTargetLayerHavePrimSpec(const UsdPrim& prim)
{
    auto editTarget = prim.GetStage()->GetEditTarget();
    auto layer = editTarget.GetLayer();
    auto primSpec = layer->GetPrimAtPath(prim.GetPath());

    // to know whether the target layer contains any opinions that 
    // affect a particular prim, there must be a primSpec for that prim
    if (!primSpec) {
        return false;
    }

    return true;
}

SdfLayerHandle
strongestLayerWithPrimSpec(const UsdPrim& prim)
{
    SdfLayerHandle targetLayer;
    auto layerStack = prim.GetStage()->GetLayerStack();
    for (auto layer : layerStack)
    {
        // to know whether the target layer contains any opinions that 
        // affect a particular prim, there must be a primSpec for that prim
        auto primSpec = layer->GetPrimAtPath(prim.GetPath());
        if (primSpec) {
            targetLayer = layer;
            break;
        }
    }
    return targetLayer;
}

SdfPrimSpecHandle 
getPrimSpecAtEditTarget(UsdStageWeakPtr stage, const UsdPrim& prim)
{
    return stage->GetEditTarget().GetPrimSpecForScenePath(prim.GetPath());
}

} // MayaUsdUtils
