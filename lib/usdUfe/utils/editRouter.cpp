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
#include "editRouter.h"

#include <usdUfe/base/tokens.h>

#include <pxr/base/tf/callContext.h>
#include <pxr/base/tf/diagnosticLite.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/gprim.h>

namespace {

UsdUfe::EditRouters& getRegisterdDefaultEditRouters()
{
    static UsdUfe::EditRouters registeredDefaultEditRouters;
    return registeredDefaultEditRouters;
}

UsdUfe::EditRouters& getRegisteredEditRouters()
{
    static UsdUfe::EditRouters registeredEditRouters;
    return registeredEditRouters;
}

void editTargetLayer(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData)
{
    // We expect a prim in the context.
    auto found = context.find(UsdUfe::EditRoutingTokens->Prim);
    if (found == context.end()) {
        return;
    }
    auto primValue = found->second;
    auto prim = primValue.Get<PXR_NS::UsdPrim>();
    if (!prim) {
        return;
    }
    auto layer = prim.GetStage()->GetEditTarget().GetLayer();
    routingData[UsdUfe::EditRoutingTokens->Layer] = PXR_NS::VtValue(layer);
}

} // namespace

namespace USDUFE_NS_DEF {

EditRouter::~EditRouter() { }

CxxEditRouter::~CxxEditRouter() { }

void CxxEditRouter::operator()(
    const PXR_NS::VtDictionary& context,
    PXR_NS::VtDictionary&       routingData)
{
    _cb(context, routingData);
}

void registerDefaultEditRouter(const PXR_NS::TfToken& operation, const EditRouter::Ptr& editRouter)
{
    getRegisterdDefaultEditRouters()[operation] = editRouter;
}

EditRouters defaultEditRouters()
{
    PXR_NAMESPACE_USING_DIRECTIVE

    EditRouters   defaultRouters;
    TfTokenVector defaultOperations = { EditRoutingTokens->RouteParent,
                                        EditRoutingTokens->RouteDuplicate,
                                        EditRoutingTokens->RouteVisibility };
    for (const auto& o : defaultOperations) {
        defaultRouters[o] = std::make_shared<CxxEditRouter>(editTargetLayer);
    }

    // Then add in any registered default edit routers.
    auto regRouters = getRegisterdDefaultEditRouters();
    for (const auto& entry : regRouters) {
        defaultRouters[entry.first] = entry.second;
    }

    return defaultRouters;
}

void registerEditRouter(const PXR_NS::TfToken& operation, const EditRouter::Ptr& editRouter)
{
    getRegisteredEditRouters()[operation] = editRouter;
}

bool restoreDefaultEditRouter(const PXR_NS::TfToken& operation)
{
    // For built-in commands that have a default router, register that
    // router again.
    auto defaults = defaultEditRouters();
    auto opAndRouter = defaults.find(operation);
    if (opAndRouter != defaults.end()) {
        registerEditRouter(operation, opAndRouter->second);
        return true;
    }

    // For commands without built-in router, for example custom composite
    // commands, remove the edit router. That will make the command no longer
    // routed.
    UsdUfe::EditRouters& editRouters = getRegisteredEditRouters();

    auto pos = editRouters.find(operation);
    if (pos == editRouters.end())
        return false;

    editRouters.erase(pos);
    return true;
}

void restoreAllDefaultEditRouters()
{
    getRegisteredEditRouters().clear();

    auto defaults = defaultEditRouters();
    for (const auto& entry : defaults) {
        registerEditRouter(entry.first, entry.second);
    }
}

EditRouter::Ptr getEditRouter(const PXR_NS::TfToken& operation)
{
    UsdUfe::EditRouters& editRouters = getRegisteredEditRouters();

    auto foundRouter = editRouters.find(operation);
    return (foundRouter == editRouters.end()) ? nullptr : foundRouter->second;
}

PXR_NS::SdfLayerHandle
getEditRouterLayer(const PXR_NS::TfToken& operation, const PXR_NS::UsdPrim& prim)
{
    const auto dstEditRouter = getEditRouter(operation);
    if (!dstEditRouter)
        return nullptr;

    PXR_NS::VtDictionary context;
    PXR_NS::VtDictionary routingData;
    context[EditRoutingTokens->Prim] = PXR_NS::VtValue(prim);
    context[EditRoutingTokens->Operation] = operation;
    (*dstEditRouter)(context, routingData);

    // Try to retrieve the layer from the routing data.
    const auto found = routingData.find(EditRoutingTokens->Layer);
    if (found == routingData.end())
        return nullptr;

    const auto& value = found->second;
    if (value.IsHolding<std::string>()) {
        std::string            layerName = value.Get<std::string>();
        PXR_NS::SdfLayerRefPtr layer = prim.GetStage()->GetRootLayer()->Find(layerName);
        return layer;
        // FIXME  We should always be using a string layer identifier, for
        // Python and C++ compatibility, so the following code should be
        // removed, and client code using edit routing should be adjusted
        // accordingly.  PPT, 27-Jan-2022.
    } else if (value.IsHolding<PXR_NS::SdfLayerHandle>()) {
        return value.Get<PXR_NS::SdfLayerHandle>();
    } else {
        return nullptr;
    }
}

PXR_NS::SdfLayerHandle
getAttrEditRouterLayer(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName)
{
    static const PXR_NS::TfToken attrOp(EditRoutingTokens->RouteAttribute);

    const EditRouter::Ptr dstEditRouter = getEditRouter(attrOp);
    if (!dstEditRouter)
        return nullptr;

    PXR_NS::VtDictionary context;
    PXR_NS::VtDictionary routingData;
    context[EditRoutingTokens->Prim] = PXR_NS::VtValue(prim);
    context[EditRoutingTokens->Operation] = attrOp;
    context[attrOp] = PXR_NS::VtValue(attrName);
    (*dstEditRouter)(context, routingData);

    // Try to retrieve the layer from the routing data.
    const auto found = routingData.find(EditRoutingTokens->Layer);
    if (found == routingData.end())
        return nullptr;

    const auto& value = found->second;
    if (value.IsHolding<std::string>()) {
        std::string layerName = value.Get<std::string>();
        return prim.GetStage()->GetRootLayer()->Find(layerName);
    } else if (value.IsHolding<PXR_NS::SdfLayerHandle>()) {
        return value.Get<PXR_NS::SdfLayerHandle>();
    } else {
        return nullptr;
    }
}

} // namespace USDUFE_NS_DEF
