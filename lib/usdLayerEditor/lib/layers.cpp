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

#include "utilFileSystem.h"

#include <usdUfe/utils/layers.h>

namespace UsdLayerEditor {
namespace Layers {

const std::string getTargetLayerFolder(const PXR_NS::UsdStagePtr& stage)
{
    return FileSystem::getDir(UsdUfe::getTargetLayerFilePath(stage));
}

const std::string getTargetLayerFolder(const PXR_NS::UsdPrim& prim)
{
    return FileSystem::getDir(UsdUfe::getTargetLayerFilePath(prim));
}

std::string getLocalTargetLayerAsString(const pxr::UsdStagePtr& stage)
{
    const auto editTarget = stage->GetEditTarget();
    if (!editTarget.IsValid() || !stage->HasLocalLayer(editTarget.GetLayer())) {
        return {};
    }

    return editTarget.GetLayer()->GetIdentifier();
}

pxr::SdfLayerHandle getLocalTargetLayerFromString(
    const LayerNameMap& nameMap,
    pxr::UsdStage& stage,
    const std::string& identifier)
{
    if (identifier.empty()) {
        return {};
    }

    std::string actualId;

    // Remap the anonymous layer names when reloaded.
    auto iter = nameMap.find(identifier);
    if (iter != nameMap.end()) {
        actualId = iter->second;
    } else {
        actualId = identifier;
    }

    for (const auto& layer : stage.GetUsedLayers()) {
        if (layer->GetIdentifier() == actualId) {
            if (stage.HasLocalLayer(layer)) {
                return layer;
            }
        }
    }

    return {};
}

} // namespace Layers
} // namespace UsdLayerEditor
