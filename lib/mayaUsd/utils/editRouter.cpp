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

#include <pxr/base/tf/callContext.h>
#include <pxr/base/tf/diagnosticLite.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/gprim.h>

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

void cacheMayaReference(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData)
{
    // FIXME  Need to handle undo / redo.

    // Read from data provided by MayaReference updater
    auto foundStage = context.find("stage");
    if (foundStage == context.end()) {
        return;
    }

    PXR_NS::UsdStageRefPtr stage = foundStage->second.Get<PXR_NS::UsdStageRefPtr>();
    if (!stage) {
        return;
    }

    auto pulledPathStr
        = PXR_NS::VtDictionaryGet<std::string>(context, "prim", PXR_NS::VtDefault = "");

    if (!PXR_NS::SdfPath::IsValidPathString(pulledPathStr))
        return;

    const PXR_NS::SdfPath pulledPath(pulledPathStr);

    // Read user args
    auto dstLayerPath
        = PXR_NS::VtDictionaryGet<std::string>(context, "rn_layer", PXR_NS::VtDefault = "");

    auto dstPrimName
        = PXR_NS::VtDictionaryGet<std::string>(context, "rn_primName", PXR_NS::VtDefault = "");

    if (dstLayerPath.empty() || dstPrimName.empty())
        return;

    // Prepare the layer
    PXR_NS::SdfPath dstPrimPath
        = PXR_NS::SdfPath(dstPrimName).MakeAbsolutePath(PXR_NS::SdfPath::AbsoluteRootPath());
    PXR_NS::SdfLayerRefPtr tmpLayer = PXR_NS::SdfLayer::CreateAnonymous();
    SdfJustCreatePrimInLayer(tmpLayer, dstPrimPath);

    tmpLayer->Export(dstLayerPath);
    PXR_NS::SdfLayerRefPtr dstLayer = PXR_NS::SdfLayer::FindOrOpen(dstLayerPath);
    if (!dstLayer)
        return;

    auto listEditName = PXR_NS::VtDictionaryGet<std::string>(
        context, "rn_listEditType", PXR_NS::VtDefault = "Append");

    auto createCachePrimFn = [stage, dstLayer, dstPrimPath](
                                 const PXR_NS::SdfPath& primPath, bool asReference, bool append) {
        auto cachePrim = stage->DefinePrim(primPath, PXR_NS::TfToken("Xform"));

        auto position = append ? PXR_NS::UsdListPositionFrontOfAppendList
                               : PXR_NS::UsdListPositionBackOfPrependList;

        if (asReference) {
            cachePrim.GetReferences().AddReference(
                dstLayer->GetIdentifier(), dstPrimPath, PXR_NS::SdfLayerOffset(), position);
        } else {
            cachePrim.GetPayloads().AddPayload(
                dstLayer->GetIdentifier(), dstPrimPath, PXR_NS::SdfLayerOffset(), position);
        }
    };

    // Prepare the composition
    auto compositionArc = PXR_NS::VtDictionaryGet<std::string>(
        context, "rn_payloadOrReference", PXR_NS::VtDefault = "");
    bool dstIsVariant
        = (PXR_NS::VtDictionaryGet<int>(context, "rn_defineInVariant", PXR_NS::VtDefault = 1) == 1);
    const auto parentPath = pulledPath.GetParentPath();
    const auto cachePrimPath = parentPath.AppendChild(PXR_NS::TfToken(dstPrimName));

    if (dstIsVariant) {
        auto dstVariantSet = PXR_NS::VtDictionaryGet<std::string>(
            context, "rn_variantSetName", PXR_NS::VtDefault = "");
        auto dstVariant = PXR_NS::VtDictionaryGet<std::string>(
            context, "rn_variantName", PXR_NS::VtDefault = "");

        PXR_NS::UsdPrim primWithVariant = stage->GetPrimAtPath(parentPath);

        PXR_NS::UsdVariantSet variantSet = primWithVariant.GetVariantSet(dstVariantSet);
        if (variantSet.AddVariant(dstVariant) && variantSet.SetVariantSelection(dstVariant)) {
            PXR_NS::UsdEditTarget target = stage->GetEditTarget();

            PXR_NS::UsdEditContext switchEditContext(
                stage, variantSet.GetVariantEditTarget(target.GetLayer()));

            createCachePrimFn(
                cachePrimPath, (compositionArc == "Reference"), (listEditName == "Append"));
        }
    } else {
        createCachePrimFn(
            cachePrimPath, (compositionArc == "Reference"), (listEditName == "Append"));
    }

    // Fill routing info
    routingData["layer"] = PXR_NS::VtValue(dstLayerPath);
    routingData["save_layer"] = PXR_NS::VtValue("yes");
    routingData["path"] = PXR_NS::VtValue(dstPrimPath.GetString());
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

EditRouters defaultEditRouters()
{
    PXR_NAMESPACE_USING_DIRECTIVE

    EditRouters   defaultRouters;
    TfTokenVector defaultOperations = { TfToken("parent"), TfToken("duplicate") };
    for (const auto& o : defaultOperations) {
        defaultRouters[o] = std::make_shared<CxxEditRouter>(editTargetLayer);
    }

    defaultRouters[TfToken("mayaReferencePush")]
        = std::make_shared<CxxEditRouter>(cacheMayaReference);

    return defaultRouters;
}

void registerEditRouter(const PXR_NS::TfToken& operation, const EditRouter::Ptr& editRouter)
{
    editRouters[operation] = editRouter;
}

bool restoreDefaultEditRouter(const PXR_NS::TfToken& operation)
{
    auto defaults = defaultEditRouters();
    auto found = defaults.find(operation);
    if (found == defaults.end()) {
        return false;
    }
    registerEditRouter(operation, found->second);
    return true;
}

EditRouter::Ptr getEditRouter(const PXR_NS::TfToken& operation)
{
    auto foundRouter = editRouters.find(operation);
    return (foundRouter == editRouters.end()) ? nullptr : foundRouter->second;
}

PXR_NS::SdfLayerHandle
getEditRouterLayer(const PXR_NS::TfToken& operation, const PXR_NS::UsdPrim& prim)
{
    auto dstEditRouter = getEditRouter(operation);
    if (!dstEditRouter) {
        return nullptr;
    }

    PXR_NS::VtDictionary context;
    PXR_NS::VtDictionary routingData;
    context["prim"] = PXR_NS::VtValue(prim);
    (*dstEditRouter)(context, routingData);
    // Try to retrieve the layer from the routing data.
    auto found = routingData.find("layer");
    if (found != routingData.end()) {
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
        }
    }
    return prim.GetStage()->GetEditTarget().GetLayer();
}

} // namespace MAYAUSD_NS_DEF
