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

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#if PXR_VERSION >= 2308
#include <pxr/usd/pcp/expressionVariables.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/primIndex.h>
#include <pxr/usd/sdf/variableExpression.h>
#endif

#include <vector>

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

std::set<std::string> getSessionLayerIds(const PXR_NS::UsdStageRefPtr& stage)
{
    std::set<std::string> sessionLayerIds;
    if (!stage)
        return sessionLayerIds;

#if PXR_VERSION >= 2308
    // The stage's expression variables, composed from the root and session layers.
    PXR_NS::VtDictionary expressionVariables;
    if (const auto layerStack
        = stage->GetPseudoRoot().GetPrimIndex().GetRootNode().GetLayerStack()) {
        expressionVariables = layerStack->GetExpressionVariables().GetVariables();
    }
#endif

    std::vector<PXR_NS::SdfLayerRefPtr> toProcess { stage->GetSessionLayer() };
    while (!toProcess.empty()) {
        const PXR_NS::SdfLayerRefPtr layer = toProcess.back();
        toProcess.pop_back();
        if (!sessionLayerIds.insert(layer->GetIdentifier()).second)
            continue;

        for (const std::string& subLayerPath : layer->GetSubLayerPaths()) {
            std::string resolvedPath = subLayerPath;
#if PXR_VERSION >= 2308
            if (PXR_NS::SdfVariableExpression::IsExpression(subLayerPath)) {
                const auto result = PXR_NS::SdfVariableExpression(subLayerPath)
                                        .EvaluateTyped<std::string>(expressionVariables);
                if (!result.errors.empty() || result.value.IsEmpty())
                    continue;
                resolvedPath = result.value.UncheckedGet<std::string>();
            }
#endif
            if (const auto subLayer
                = PXR_NS::SdfLayer::FindRelativeToLayer(layer, resolvedPath)) {
                toProcess.push_back(subLayer);
            }
        }
    }
    return sessionLayerIds;
}

} // namespace Layers
} // namespace UsdLayerEditor
