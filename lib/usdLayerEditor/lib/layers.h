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
#define USDLAYEREDITOR_LAYERS_H

#include <usdLayerEditor/lib/layerEditorAPI.h>

#include <usdUfe/utils/layers.h>

#include <set>
#include <string>

namespace UsdLayerEditor {

namespace Layers {

inline std::set<std::string>
getAllSublayers(const std::vector<std::string>& parentLayerPaths, bool includeParents = false)
{
    return UsdUfe::getAllSublayers(parentLayerPaths, includeParents);
}

//! Return the identifiers of the session layer of the stage and all of its sublayers, including
//  the ones named by variable expressions. Sublayers that cannot be resolved are skipped.
//  If the stage is null, the returned set will be empty.
LAYEREDITOR_PUBLIC std::set<std::string> getSessionLayerIds(const PXR_NS::UsdStageRefPtr& stage);

//! Return the folder of the layer of the current edit target of the stage, if any.
//  If the stage is null, the returned path will be empty.
LAYEREDITOR_PUBLIC const std::string getTargetLayerFolder(const PXR_NS::UsdStagePtr& stage);

//! Return the folder of the layer of the current edit target of the prim, if any.
//  If the prim is invalid, the returned path will be empty.
LAYEREDITOR_PUBLIC const std::string getTargetLayerFolder(const PXR_NS::UsdPrim& prim);

} // namespace Layers
} // namespace UsdLayerEditor
#endif
