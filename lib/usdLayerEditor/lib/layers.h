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

#ifndef USDLAYEREDITOR_LAYERS_H
#define USDLAYEREDITOR__LAYERS_H

#include "layerEditorAPI.h"

#include <usdUfe/utils/layers.h>

namespace UsdLayerEditor {

namespace Layers {

using LayerNameMap = std::map<std::string, std::string>;

inline std::set<std::string>
getAllSublayers(const std::vector<std::string>& parentLayerPaths, bool includeParents = false)
{
    return UsdUfe::getAllSublayers(parentLayerPaths, includeParents);
}

//! Return the folder of the layer of the current edit target of the stage, if any.
//  If the stage is null, the returned path will be empty.
LayerEditorAPI const std::string getTargetLayerFolder(const PXR_NS::UsdStagePtr& stage);

//! Return the folder of the layer of the current edit target of the prim, if any.
//  If the prim is invalid, the returned path will be empty.
LayerEditorAPI const std::string getTargetLayerFolder(const PXR_NS::UsdPrim& prim);

//! Returns the stage's target layer as a string that can be serialized.
// If the edit target is not a layer on the local layer stack, an empty string is returned.
LayerEditorAPI std::string getLocalTargetLayerAsString(const PXR_NS::UsdStagePtr& stage);

//! Returns a local layer target from the given stage, nameMap and layer identifier.
// Returns an invalid layer handle if the layer cannot be found (possibly no longer
// exists) or cannot be used as edit target directly (is no longer on the local layer
// stack)
LayerEditorAPI PXR_NS::SdfLayerHandle getLocalTargetLayerFromString(
    const LayerNameMap& nameMap,
    PXR_NS::UsdStage&   stage,
    const std::string&  identifier);

} // namespace Layers
} // namespace UsdLayerEditor
#endif
