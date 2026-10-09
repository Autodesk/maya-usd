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
#ifndef USDLAYEREDITOR_LAYERMUTING_H
#define USDLAYEREDITOR_LAYERMUTING_H

#include "layerEditorAPI.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <set>
#include <string>

namespace UsdLayerEditor {

using LayerNameMap = std::map<std::string, std::string>;

/**
 * Loads a layer mute state.
 * @param muted The layer identifiers of layers to be muted.
 * @param nameMap Layer name map. When Anon layers are saved to a DCC scene, and reloaded,
 * their names change, this map allows to handle the remapping that needs to be done to mute the
 * layers.
 * @param stage The USD Stage.
 */
LAYEREDITOR_PUBLIC void loadLayerMuteState(
    const std::vector<std::string>& muted,
    const LayerNameMap&             nameMap,
    PXR_NS::UsdStage&               stage);

// OpenUSD forget everything about muted layers. The OpenUSD documentation for
// the MuteLayer function says:
//
//    Note that muting a layer will cause this stage to release all references
//    to that layer. If no other client is holding on to references to that
//    layer, it will be unloaded.In this case, if there are unsaved edits to
//    the muted layer, those edits are lost.
//
//    Since anonymous layers are not serialized, muting an anonymous layer will
//    cause that layer and its contents to be lost in this case.
//
// So we need to hold on to muted layers. We do this in a private global list
// of muted layers. That list gets cleared when a new Maya scene is created.

LAYEREDITOR_PUBLIC bool addMutedLayer(const PXR_NS::SdfLayerRefPtr& layer);

LAYEREDITOR_PUBLIC bool removeMutedLayer(const PXR_NS::SdfLayerRefPtr& layer);

LAYEREDITOR_PUBLIC void forgetMutedLayers();

/*! Set of layer reference pointers.
 */
using LayerRefSet = std::set<PXR_NS::SdfLayerRefPtr>;

/*! Returns layers held due to muting layer \p mutedIdentifier in a USD stage,
 * includes the muted root (if dirty/anonymous) and all recorded descendants
 * in its sublayer hierarchy.
 */
LAYEREDITOR_PUBLIC const LayerRefSet& getMutedLayers(const std::string& mutedIdentifier);

} // namespace UsdLayerEditor

#endif
