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
#include "editRouterContext.h"

#include <usdUfe/utils/editRouter.h>

#include <pxr/base/tf/instantiateStacked.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_STACKED(UsdUfe::StackedEditRouterContext);

PXR_NAMESPACE_CLOSE_SCOPE

namespace USDUFE_NS_DEF {

StackedEditRouterContext::StackedEditRouterContext(
    const PXR_NS::UsdStagePtr&    stage,
    const PXR_NS::SdfLayerHandle& layer)
    : _stage(stage)
    , _layer(layer)
    , _previousTarget()
{
    if (stage && layer) {
        _previousTarget = stage->GetEditTarget();
        stage->SetEditTarget(layer);
    }
}

StackedEditRouterContext::~StackedEditRouterContext()
{
    if (_stage && _layer) {
        _stage->SetEditTarget(_previousTarget);
    }
}

const PXR_NS::SdfLayerHandle& StackedEditRouterContext::getLayer() const
{
    // Use the layer of the top-most edit router context that contains a layer.
    for (const StackedEditRouterContext* ctx : GetStack())
        if (ctx->_layer)
            return ctx->_layer;

    static const PXR_NS::SdfLayerHandle empty;
    return empty;
}

PXR_NS::UsdStagePtr StackedEditRouterContext::getStage() const
{
    // Use the stage of the top-most edit router context that contains a layer.
    //
    // Note: *yes* we check the layer for the stage, it is the layer that can be
    //       null or not.
    for (const StackedEditRouterContext* ctx : GetStack())
        if (ctx->_layer)
            return ctx->_stage;

    return {};
}

bool StackedEditRouterContext::isTargetAlreadySet() const
{
    // Use the edit target of a edit router context higher-up in the call
    // stack only if it is not ourself and if it had been routed to a specific
    // layer.
    for (const StackedEditRouterContext* ctx : GetStack())
        if (ctx && ctx != this && ctx->getLayer())
            return true;

    return false;
}

PXR_NS::SdfLayerHandle OperationEditRouterContext::getOperationLayer(
    const PXR_NS::TfToken& operationName,
    const PXR_NS::UsdPrim& prim)
{
    if (isTargetAlreadySet())
        return nullptr;

    return getEditRouterLayer(operationName, prim);
}

OperationEditRouterContext::OperationEditRouterContext(
    const PXR_NS::TfToken& operationName,
    const PXR_NS::UsdPrim& prim)
    : StackedEditRouterContext(prim.GetStage(), getOperationLayer(operationName, prim))
{
}

OperationEditRouterContext::OperationEditRouterContext(
    const PXR_NS::UsdStagePtr&    stage,
    const PXR_NS::SdfLayerHandle& layer)
    : StackedEditRouterContext(stage, layer)
{
}

PXR_NS::SdfLayerHandle AttributeEditRouterContext::getAttributeLayer(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& attributeName)
{
    if (isTargetAlreadySet())
        return nullptr;

    return getAttrEditRouterLayer(prim, attributeName);
}

AttributeEditRouterContext::AttributeEditRouterContext(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& attributeName)
    : StackedEditRouterContext(prim.GetStage(), getAttributeLayer(prim, attributeName))
{
}

AttributeEditRouterContext::AttributeEditRouterContext(
    const PXR_NS::UsdStagePtr&    stage,
    const PXR_NS::SdfLayerHandle& layer)
    : StackedEditRouterContext(stage, layer)
{
}

PXR_NS::SdfLayerHandle PrimMetadataEditRouterContext::getPrimMetadataLayer(
    const PXR_NS::UsdPrim&        prim,
    const PXR_NS::TfToken&        metadataName,
    const PXR_NS::TfToken&        metadataKeyPath,
    const PXR_NS::SdfLayerHandle& fallbackLayer)
{
    if (isTargetAlreadySet())
        return nullptr;

    auto routerLayer = getPrimMetadataEditRouterLayer(prim, metadataName, metadataKeyPath);

    return routerLayer ? routerLayer : fallbackLayer;
}

PrimMetadataEditRouterContext::PrimMetadataEditRouterContext(
    const PXR_NS::UsdPrim&        prim,
    const PXR_NS::TfToken&        metadataName,
    const PXR_NS::TfToken&        metadataKeyPath,
    const PXR_NS::SdfLayerHandle& fallbackLayer)
    : StackedEditRouterContext(
        prim.GetStage(),
        getPrimMetadataLayer(prim, metadataName, metadataKeyPath, fallbackLayer))
{
}

PrimMetadataEditRouterContext::PrimMetadataEditRouterContext(
    const PXR_NS::UsdStagePtr&    stage,
    const PXR_NS::SdfLayerHandle& layer)
    : StackedEditRouterContext(stage, layer)
{
}
} // namespace USDUFE_NS_DEF
