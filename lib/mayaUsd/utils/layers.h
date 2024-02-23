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

#include <usdUfe/utils/layers.h>

namespace MAYAUSD_NS_DEF {

inline std::set<std::string>
getAllSublayers(const std::vector<std::string>& parentLayerPaths, bool includeParents = false)
{
    return UsdUfe::getAllSublayers(parentLayerPaths, includeParents);
}

inline std::set<PXR_NS::SdfLayerRefPtr>
getAllSublayerRefs(const PXR_NS::SdfLayerRefPtr& layer, bool includeTopLayer = false)
{
    return UsdUfe::getAllSublayerRefs(layer, includeTopLayer);
}

//! Return the folder of the layer of the current edit target of the stage, if any.
//  If the stage is null, the returned path will be empty.
MAYAUSD_CORE_PUBLIC
const std::string getTargetLayerFolder(const PXR_NS::UsdStagePtr& stage);

//! Return the folder of the layer of the current edit target of the prim, if any.
//  If the prim is invalid, the returned path will be empty.
MAYAUSD_CORE_PUBLIC
const std::string getTargetLayerFolder(const PXR_NS::UsdPrim& prim);

} // namespace MAYAUSD_NS_DEF

#endif
