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
#ifndef USDUFE_UTIL_LAYERS_H
#define USDUFE_UTIL_LAYERS_H

#include <usdUfe/base/api.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace USDUFE_NS_DEF {

/**
 * Returns all the sublayers recursively for a given layer
 *
 * @param layer The layer to start from and get the sublayers
 *
 * @return The list of identifiers for all the sublayers
 */
USDUFE_PUBLIC
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
USDUFE_PUBLIC
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
USDUFE_PUBLIC
std::set<PXR_NS::SdfLayerRefPtr>
getAllSublayerRefs(const PXR_NS::SdfLayerRefPtr& layer, bool includeTopLayer = false);

/**
 * Verify if the given prim has opinions on a muted layer.
 *
 * @param prim The prim to be verified.
 *
 * @return true if there is at least one muted layer.
 */

USDUFE_PUBLIC
bool hasMutedLayer(const PXR_NS::UsdPrim& prim);

/**
 * Enforce that command cannot operate if the given prim has opinions on a muted layer by throwing
 * an exception.
 *
 * @param prim The prim to be verified.
 * @param command The name of the command. Will use "modify" if null or empty.
 */

USDUFE_PUBLIC
void enforceMutedLayer(const PXR_NS::UsdPrim& prim, const char* command);

/**
 * Apply the given function to all the opinions about the given prim.
 *
 * @param prim The prim to be modified.
 * @param func The function to be applied.
 * @return the number of prim specs that were affected.
 */

using PrimSpecFunc = std::function<void(const PXR_NS::UsdPrim&, const PXR_NS::SdfPrimSpecHandle&)>;

USDUFE_PUBLIC
int applyToAllPrimSpecs(const PXR_NS::UsdPrim& prim, const PrimSpecFunc& func);

/**
 * Apply the given function to all the layers that have an opinion about the given prim.
 *
 * @param prim The prim to be modified.
 * @param func The function to be applied.
 * @return the number of layers that were affected.
 */

using PrimLayerFunc = std::function<void(const PXR_NS::UsdPrim&, const PXR_NS::SdfLayerRefPtr&)>;

USDUFE_PUBLIC
int applyToAllLayersWithOpinions(const PXR_NS::UsdPrim& prim, PrimLayerFunc& func);

/**
 * Apply the given function to some of the layers that have an opinion about the given prim.
 * Only the layers that are part of the given set will be affected.
 *
 * @param prim The prim to be modified.
 * @param layers The set of layers that can be affected if they contain an opinion.
 * @param func The function to be applied.
 * @return the number of layers that were affected.
 */

USDUFE_PUBLIC
int applyToSomeLayersWithOpinions(
    const PXR_NS::UsdPrim&                  prim,
    const std::set<PXR_NS::SdfLayerRefPtr>& layers,
    PrimLayerFunc&                          func);

/**
 * Verify if a layer is in the given stage.
 *
 * @param layer the layer to verify.
 * @param stage the stage to verify.
 */

USDUFE_PUBLIC
bool isLayerInStage(const PXR_NS::SdfLayerHandle& layer, const PXR_NS::UsdStage& stage);

/**
 * Verify if a layer is in the given set of session layers.
 *
 * @param layer the layer to verify.
 * @param sessionLayers the set of session layers.
 */

USDUFE_PUBLIC
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

USDUFE_PUBLIC
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

USDUFE_PUBLIC
PXR_NS::SdfLayerHandle getStrongerLayer(
    const PXR_NS::UsdStagePtr&    stage,
    const PXR_NS::SdfLayerHandle& layer1,
    const PXR_NS::SdfLayerHandle& layer2,
    bool                          compareSessionLayers = false);

//! Return all layers in the given layers where there are opinions about the prim.
USDUFE_PUBLIC
PXR_NS::SdfPrimSpecHandleVector
getPrimStackForLayers(const PXR_NS::UsdPrim& prim, const PXR_NS::SdfLayerHandleVector& layers);

//! Return all local layers in the stage of the prim where there are opinions about the prim.
//
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
USDUFE_PUBLIC
PXR_NS::SdfPrimSpecHandleVector getLocalPrimStack(const PXR_NS::UsdPrim& prim);

//! Return all layers and related paths in the layer stack where the prim is first defined.
//  When the prim is in a reference, those paths will not be equal to the path of the input prim.
USDUFE_PUBLIC
PXR_NS::SdfPrimSpecHandleVector getDefiningPrimStack(const PXR_NS::UsdPrim& prim);

//! Return the layer and path where the prim is defined and the path relative to that layer.
//  When the prim is in a reference, that path will not be equal to the path of the input prim.
USDUFE_PUBLIC
PXR_NS::SdfPrimSpecHandle getDefiningPrimSpec(const PXR_NS::UsdPrim& prim);

//! Return the layer of the current edit target of the stage, if any.
//  If the stage is null, the returned layer will be null.
USDUFE_PUBLIC
const PXR_NS::SdfLayerHandle getCurrentTargetLayer(const PXR_NS::UsdStagePtr& stage);

//! Return the layer of the current edit target of the prim, if any.
//  If the prim is invalid, the returned layer will be null.
USDUFE_PUBLIC
const PXR_NS::SdfLayerHandle getCurrentTargetLayer(const PXR_NS::UsdPrim& prim);

//! Return the file path of the layer of the current edit target of the stage, if any.
//  If the stage is null, the returned path will be empty.
USDUFE_PUBLIC
const std::string getTargetLayerFilePath(const PXR_NS::UsdStagePtr& stage);

//! Return the file path of the layer of the current edit target of the prim, if any.
//  If the prim is invalid, the returned path will be empty.
USDUFE_PUBLIC
const std::string getTargetLayerFilePath(const PXR_NS::UsdPrim& prim);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UTIL_LAYERS_H
