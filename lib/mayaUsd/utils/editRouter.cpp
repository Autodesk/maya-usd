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

#include <mayaUsdUtils/MergePrims.h>

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

PXR_NS::UsdStageRefPtr getMayaReferenceStage(const PXR_NS::VtDictionary& context)
{
    auto foundStage = context.find("stage");
    if (foundStage == context.end())
        return nullptr;

    return foundStage->second.Get<PXR_NS::UsdStageRefPtr>();
}

// Retrieve a value from a USD dictionary, with a default value.
template <class T> T getDictValue(const PXR_NS::VtDictionary& dict, const char* key, T defaultValue)
{
    return PXR_NS::VtDictionaryGet<T>(dict, key, PXR_NS::VtDefault = defaultValue);
}

// Retrieve a string from a USD dictionary, with a default value.
// This variation allows specifying the default with a string literal while
// still having a std::string return value.
std::string
getDictString(const PXR_NS::VtDictionary& dict, const char* key, const char* defaultValue = "")
{
    return getDictValue(dict, key, std::string(defaultValue));
}

// Copy the transform from the top Maya object that was holding the top reference
// object into the prim that represent the Maya Reference.
//
// We must pass the destination path in two form: one that is compatible with
// getPrimAtPath() and one that is compatible with SdfCopySpec(). The reason
// they are different is that when there is a variant, the destination variant
// must be specified in the path to SdfCopySpec(), but specifying a variant is
// not supported by getPrimAtPath(), it fails to find the prim.
void copyTransform(
    const PXR_NS::UsdStageRefPtr& srcStage,
    const PXR_NS::SdfLayerRefPtr& srcLayer,
    const PXR_NS::SdfPath&        srcSdfPath,
    const PXR_NS::UsdStageRefPtr& dstStage,
    const PXR_NS::SdfLayerRefPtr& dstLayer,
    const PXR_NS::SdfPath&        dstSdfPath,
    const PXR_NS::SdfPath&        dstSdfPathForMerge)
{
    // Note: necessary to compile the TF_VERIFY macro as it refers to USD types without using
    //       the namespace prefix.
    PXR_NAMESPACE_USING_DIRECTIVE;

    // Copy transform changes that came from the Maya transform node into the
    // Maya reference prim.  The Maya transform node changes have already been
    // exported into the temporary layer as a transform prim, which is our
    // source.  The destination prim in the stage is the Maya reference prim.
    auto srcPrim = srcStage->GetPrimAtPath(srcSdfPath);
    if (TF_VERIFY(UsdGeomXformable(srcPrim))) {
        auto dstPrim = dstStage->GetPrimAtPath(dstSdfPath);
        if (TF_VERIFY(UsdGeomXformable(dstPrim))) {
            // The Maya transform that corresponds to the Maya reference prim
            // only has its transform attributes unlocked.  Bring any transform
            // attribute edits over to the Maya reference prim.
            MayaUsdUtils::MergePrimsOptions options;
            options.ignoreUpperLayerOpinions = false;
            TF_VERIFY(MayaUsdUtils::mergePrims(
                srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPathForMerge, options));
        }
    }
}

// Create the prim that will hold the cache.
void createCachePrim(
    const PXR_NS::UsdStageRefPtr& stage,
    const PXR_NS::SdfLayerRefPtr& dstLayer,
    PXR_NS::SdfPath               dstPrimPath,
    const PXR_NS::SdfPath&        primPath,
    bool                          asReference,
    bool                          append)
{
    PXR_NS::UsdPrim cachePrim = stage->DefinePrim(primPath, PXR_NS::TfToken("Xform"));

    auto position = append ? PXR_NS::UsdListPositionFrontOfAppendList
                           : PXR_NS::UsdListPositionBackOfPrependList;

    if (asReference) {
        cachePrim.GetReferences().AddReference(
            dstLayer->GetIdentifier(), dstPrimPath, PXR_NS::SdfLayerOffset(), position);
    } else {
        cachePrim.GetPayloads().AddPayload(
            dstLayer->GetIdentifier(), dstPrimPath, PXR_NS::SdfLayerOffset(), position);
    }
}

void cacheMayaReference(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& routingData)
{
    // FIXME  Need to handle undo / redo.

    // Read from data provided by MayaReference updater
    PXR_NS::UsdStageRefPtr stage = getMayaReferenceStage(context);
    if (!stage)
        return;

    // Read user arguments provide in the context dictionary.
    // TODO: document all arguments for plugin users.
    auto pulledPathStr = getDictString(context, "prim");
    auto fileFormatExtension = getDictString(context, "defaultUSDFormat");
    auto dstLayerPath = getDictString(context, "rn_layer");
    auto dstPrimName = getDictString(context, "rn_primName");
    bool appendListEdit = (getDictString(context, "rn_listEditType", "Append") == "Append");
    bool asReference = (getDictString(context, "rn_payloadOrReference", "") == "Reference");
    bool dstIsVariant = (getDictValue(context, "rn_defineInVariant", 1) == 1);
    auto dstVariantSet = getDictString(context, "rn_variantSetName");
    auto dstVariant = getDictString(context, "rn_variantName");

    if (!PXR_NS::SdfPath::IsValidPathString(pulledPathStr))
        return;

    const PXR_NS::SdfPath pulledPath(pulledPathStr);
    const PXR_NS::SdfPath pulledParentPath = pulledPath.GetParentPath();

    if (dstLayerPath.empty() || dstPrimName.empty())
        return;

    // Determine the file format
    PXR_NS::SdfLayer::FileFormatArguments fileFormatArgs;
    PXR_NS::SdfFileFormatConstPtr         fileFormat;

    if (fileFormatExtension.size() > 0) {
        fileFormatArgs[PXR_NS::UsdUsdFileFormatTokens->FormatArg] = fileFormatExtension;
        auto dummyFilename = std::string("a.") + fileFormatExtension;
        fileFormat = PXR_NS::SdfFileFormat::FindByExtension(dummyFilename, fileFormatArgs);
    }

    // Prepare the layer
    PXR_NS::SdfPath dstPrimPath
        = PXR_NS::SdfPath(dstPrimName).MakeAbsolutePath(PXR_NS::SdfPath::AbsoluteRootPath());
    PXR_NS::SdfLayerRefPtr tmpLayer
        = PXR_NS::SdfLayer::CreateAnonymous("", fileFormat, fileFormatArgs);
    SdfJustCreatePrimInLayer(tmpLayer, dstPrimPath);

    tmpLayer->SetDefaultPrim(dstPrimPath.GetNameToken());

    tmpLayer->Export(dstLayerPath, "", fileFormatArgs);
    PXR_NS::SdfLayerRefPtr dstLayer = PXR_NS::SdfLayer::FindOrOpen(dstLayerPath);
    if (!dstLayer)
        return;

    // Copy the transform to the Maya reference prim under the Maya reference variant.
    {
        auto srcStage = getDictValue(context, "src_stage", PXR_NS::UsdStageRefPtr());
        auto srcLayer = getDictValue(context, "src_layer", PXR_NS::SdfLayerRefPtr());
        auto srcSdfPath = getDictValue(context, "src_path", PXR_NS::SdfPath());
        auto dstStage = getDictValue(context, "dst_stage", PXR_NS::UsdStageRefPtr());
        auto dstLayer = getDictValue(context, "dst_layer", PXR_NS::SdfLayerRefPtr());
        auto dstSdfPath = getDictValue(context, "dst_path", PXR_NS::SdfPath());

        // Prepare destination path for merge, incorporating the destination variant
        // if caching into a variant.
        auto dstSdfPathForMerge = dstSdfPath;
        if (dstIsVariant) {
            auto primWithVariant = stage->GetPrimAtPath(pulledParentPath);
            auto variantSet = primWithVariant.GetVariantSet(dstVariantSet);
            auto parent = dstSdfPath.GetParentPath();
            auto withVariant
                = parent.AppendVariantSelection(dstVariantSet, variantSet.GetVariantSelection());

            dstSdfPathForMerge = withVariant.AppendChild(dstSdfPath.GetNameToken());
        }

        copyTransform(
            srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPath, dstSdfPathForMerge);
    }

    // Prepare the composition
    const auto cachePrimPath = pulledParentPath.AppendChild(PXR_NS::TfToken(dstPrimName));

    if (dstIsVariant) {

        PXR_NS::UsdPrim       primWithVariant = stage->GetPrimAtPath(pulledParentPath);
        PXR_NS::UsdVariantSet variantSet = primWithVariant.GetVariantSet(dstVariantSet);

        // Cache the Maya reference as USD prims under the cache variant.
        if (variantSet.AddVariant(dstVariant) && variantSet.SetVariantSelection(dstVariant)) {
            PXR_NS::UsdEditTarget target = stage->GetEditTarget();

            PXR_NS::UsdEditContext switchEditContext(
                stage, variantSet.GetVariantEditTarget(target.GetLayer()));

            createCachePrim(
                stage, dstLayer, dstPrimPath, cachePrimPath, asReference, appendListEdit);
        }
    } else {
        createCachePrim(stage, dstLayer, dstPrimPath, cachePrimPath, asReference, appendListEdit);
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
    TfTokenVector defaultOperations
        = { TfToken("parent"), TfToken("duplicate"), TfToken("visibility") };
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

static void setEditTarget(const PXR_NS::UsdPrim& prim, const PXR_NS::UsdEditTarget& editTarget)
{
    prim.GetStage()->SetEditTarget(editTarget);
}

EditTargetGuard::EditTargetGuard(
    const PXR_NS::UsdPrim&       prim,
    const PXR_NS::UsdEditTarget& editTarget)
    : _prim(prim)
    , _prevEditTarget(_prim.GetStage()->GetEditTarget())
{
    // Set editTarget as the edit target
    setEditTarget(_prim, editTarget);
}

EditTargetGuard::~EditTargetGuard()
{
    // Restore editTarget to the stage's previous edit target
    setEditTarget(_prim, _prevEditTarget);
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
