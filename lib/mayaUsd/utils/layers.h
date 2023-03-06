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
#ifndef PXRUSDMAYA_UTIL_LAYERS_H
#define PXRUSDMAYA_UTIL_LAYERS_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

/**
 * Returns all the sublayers recursively for a given layer
 *
 * @param layer The layer to start from and get the sublayers
 *
 * @return The list of identifiers for all the sublayers
 */
MAYAUSD_CORE_PUBLIC
std::set<std::string> getAllSublayers(const PXR_NS::SdfLayerRefPtr& layer);

/**
 * Returns all the sublayers recursively for a list of layers
 *
 * @param parentLayerPaths The list of the parent layer paths to traverse recursively
 * @param includeParents   This will add the parents passed in to the output
 *
 * @return The list of identifiers for all the sublayers for each parent (plus the parents
 * potentially)
 */
MAYAUSD_CORE_PUBLIC
std::set<std::string>
getAllSublayers(const std::vector<std::string>& parentLayerPaths, bool includeParents = false);

/**
 * Returns all the sublayers reference pointers recursively for a given layer
 *
 * @param layer The layer to start from and get the sublayers
 * @param includeTopLayer also add the layer that was passed in
 *
 * @return The list of layers for all the sublayers
 */
MAYAUSD_CORE_PUBLIC
std::set<PXR_NS::SdfLayerRefPtr>
getAllSublayerRefs(const PXR_NS::SdfLayerRefPtr& layer, bool includeTopLayer = false);

/**
 * Apply the given function to all the opinions about the given prim.
 *
 * @param prim The prim to be modified.
 * @param func The function to be applied.
 */

using PrimSpecFunc = std::function<void(const PXR_NS::UsdPrim&, const PXR_NS::SdfPrimSpecHandle&)>;

MAYAUSD_CORE_PUBLIC
void applyToAllPrimSpecs(const PXR_NS::UsdPrim& prim, const PrimSpecFunc& func);

/**
 * Apply the given function to all the layers that have an opinion about the given prim.
 *
 * @param prim The prim to be modified.
 * @param func The function to be applied.
 */

using PrimLayerFunc = std::function<void(const PXR_NS::UsdPrim&, const PXR_NS::SdfLayerRefPtr&)>;

MAYAUSD_CORE_PUBLIC
void applyToAllLayersWithOpinions(const PXR_NS::UsdPrim& prim, PrimLayerFunc& func);

/**
 * Apply the given function to some of the layers that have an opinion about the given prim.
 * Only the layers that are part of the given set will be affected.
 *
 * @param prim The prim to be modified.
 * @param layers The set of layers that can be affected if they contain an opinion.
 * @param func The function to be applied.
 */

MAYAUSD_CORE_PUBLIC
void applyToSomeLayersWithOpinions(
    const PXR_NS::UsdPrim&                  prim,
    const std::set<PXR_NS::SdfLayerRefPtr>& layers,
    PrimLayerFunc&                          func);

/**
 * Verify if a layer is in the given set of session layers.
 *
 * @param layer the layer to verify.
 * @param sessionLayers the set of session layers.
 */

MAYAUSD_CORE_PUBLIC
bool isSessionLayer(
    const PXR_NS::SdfLayerHandle&           layer,
    const std::set<PXR_NS::SdfLayerRefPtr>& sessionLayers);

/**
 * Get which of the two given layers is the strongest under the given root layer hierarchy.
 *
 * @param root The root layer that will determine which is stronger.
 * @param layer1 one of the layers to be compared.
 * @param layer2 one of the layers to be compared.
 */

MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerHandle getStrongerLayer(
    const PXR_NS::SdfLayerHandle& root,
    const PXR_NS::SdfLayerHandle& layer1,
    const PXR_NS::SdfLayerHandle& layer2);

/**
 * Get which of the two given layers is the strongest under the given stage root layer hierarchy.
 *
 * @param stage The stage with the root layer that will determine which is stronger.
 * @param layer1 one of the layers to be compared.
 * @param layer2 one of the layers to be compared.
 * @param compareSessionLayers if true, also search the session layer. False by default.
 */

MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerHandle getStrongerLayer(
    const PXR_NS::UsdStagePtr&    stage,
    const PXR_NS::SdfLayerHandle& layer1,
    const PXR_NS::SdfLayerHandle& layer2,
    bool                          compareSessionLayers = false);

} // namespace MAYAUSD_NS_DEF

#endif
