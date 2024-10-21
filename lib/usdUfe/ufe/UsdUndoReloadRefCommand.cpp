//
// Copyright 2024 Autodesk
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

#include "UsdUndoReloadRefCommand.h"

#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/types.h>
#include <pxr/usd/sdf/declareHandles.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/primCompositionQuery.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(UsdUndoableCommand<Ufe::UndoableCommand>, UsdUndoReloadRefCommand);

void _getPrimLayers(const PXR_NS::UsdPrim& prim, SdfLayerHandleSet& layersSet)
{
    for (const auto node : prim.GetPrimIndex().GetNodeRange(PcpRangeTypeWeakerThanRoot)) {
        for (const auto& layer : node.GetLayerStack()->GetLayers()) {
            layersSet.insert(layer);
        }
    }
}

UsdUndoReloadRefCommand::UsdUndoReloadRefCommand(const PXR_NS::UsdPrim& prim)
    : _prim(prim)
{
}

void UsdUndoReloadRefCommand::executeImplementation()
{
    if (!_prim.IsValid())
        return;

    SdfLayerHandleSet layersSet;
    _getPrimLayers(_prim, layersSet);

    for (const UsdPrim& descendant : _prim.GetDescendants()) {
        _getPrimLayers(descendant, layersSet);
    }

    SdfLayer::ReloadLayers(layersSet);
}

} // namespace USDUFE_NS_DEF
