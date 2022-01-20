//
// Copyright 2021 Autodesk
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
#include "editRouter.h"

#include <pxr/base/tf/callContext.h>
#include <pxr/base/tf/diagnosticLite.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/gprim.h>

#include <iostream>

namespace {

MayaUsd::EditRouters editRouters;

void editTargetLayer(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData)
{
    // We expect a prim in the context.
    auto found = context.find("prim");
    if (found == context.end()) {
        return;
    }
    auto primValue = found->second;
    auto prim = primValue.Get<PXR_NS::UsdPrim>();
    if (!prim) {
        return;
    }
    auto layer = prim.GetStage()->GetEditTarget().GetLayer();
    routingData["layer"] = PXR_NS::VtValue(layer);
}

} // namespace

namespace MAYAUSD_NS_DEF {

EditRouter::~EditRouter() { }

CxxEditRouter::~CxxEditRouter() { }

void CxxEditRouter::operator()(
    const PXR_NS::VtDictionary& context,
    PXR_NS::VtDictionary&       routingData)
{
    _cb(context, routingData);
}

EditRouters editRouterDefaults()
{
    PXR_NAMESPACE_USING_DIRECTIVE

    EditRouters   defaultRouters;
    TfTokenVector defaultOperations = { TfToken("parent"), TfToken("duplicate") };
    for (const auto& o : defaultOperations) {
        defaultRouters[o] = std::make_shared<CxxEditRouter>(editTargetLayer);
    }
    return defaultRouters;
}

void registerEditRouter(const PXR_NS::TfToken& operation, const EditRouter::Ptr& editRouter)
{
    editRouters[operation] = editRouter;
}

PXR_NS::SdfLayerHandle
getEditRouterLayer(const PXR_NS::TfToken& operation, const PXR_NS::UsdPrim& prim)
{
    auto foundRouter = editRouters.find(operation);
    if (foundRouter == editRouters.end())
        return nullptr;

    auto                 dstEditRouter = foundRouter->second;
    PXR_NS::VtDictionary context;
    PXR_NS::VtDictionary routingData;
    context["prim"] = PXR_NS::VtValue(prim);
    (*dstEditRouter)(context, routingData);
    // Try to retrieve the layer from the edit control data.
    auto found = routingData.find("layer");
    if (found != routingData.end()) {
        if (found->second.IsHolding<std::string>()) {
            std::string            layerName = found->second.Get<std::string>();
            PXR_NS::SdfLayerRefPtr layer = prim.GetStage()->GetRootLayer()->Find(layerName);
            return layer;
        } else if (found->second.IsHolding<PXR_NS::SdfLayerHandle>()) {
            return found->second.Get<PXR_NS::SdfLayerHandle>();
        }
    }
    return prim.GetStage()->GetEditTarget().GetLayer();
}

} // namespace MAYAUSD_NS_DEF
