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
#include "mayaEditRouter.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/layers.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <usdUfe/base/tokens.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/mergePrims.h>

#include <pxr/base/tf/callContext.h>
#include <pxr/base/tf/diagnosticLite.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/references.h>
#include <pxr/usd/usd/stage.h>

#if PXR_VERSION < 2508
#include <pxr/usd/usd/usdFileFormat.h>
#else
#include <pxr/usd/sdf/usdFileFormat.h>
#endif

#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/gprim.h>

namespace {

PXR_NS::UsdStageRefPtr getMayaReferenceStage(const PXR_NS::VtDictionary& context)
{
    auto foundStage = context.find(UsdUfe::EditRoutingTokens->Stage);
    if (foundStage == context.end())
        return nullptr;

    return foundStage->second.Get<PXR_NS::UsdStageRefPtr>();
}

// Retrieve a value from a USD dictionary, with a default value.
template <class T>
T getDictValue(const PXR_NS::VtDictionary& dict, const PXR_NS::TfToken& key, T defaultValue)
{
    return PXR_NS::VtDictionaryGet<T>(dict, key, PXR_NS::VtDefault = defaultValue);
}

// Retrieve a string from a USD dictionary, with a default value.
// This variation allows specifying the default with a string literal while
// still having a std::string return value.
std::string getDictString(
    const PXR_NS::VtDictionary& dict,
    const PXR_NS::TfToken&      key,
    const char*                 defaultValue = "")
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
    PXR_NAMESPACE_USING_DIRECTIVE

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
            UsdUfe::MergePrimsOptions options;
            options.ignoreUpperLayerOpinions = false;
            options.ignoreVariants = true;
            TF_VERIFY(UsdUfe::mergePrims(
                srcStage, srcLayer, srcSdfPath, dstStage, dstLayer, dstSdfPathForMerge, options));
        }
    }
}

// Create the prim that will hold the cache.
void createCachePrim(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            dstLayerPath,
    PXR_NS::SdfPath               dstPrimPath,
    const PXR_NS::SdfPath&        primPath,
    bool                          asReference,
    bool                          append)
{
    PXR_NS::UsdPrim cachePrim
        = stage->DefinePrim(primPath, PXR_NS::MayaUsdEditRoutingTokens->Xform);

    auto position = append ? PXR_NS::UsdListPositionFrontOfAppendList
                           : PXR_NS::UsdListPositionBackOfPrependList;

    if (asReference) {
        PXR_NS::SdfReference ref(dstLayerPath, dstPrimPath);
        cachePrim.GetReferences().AddReference(ref, position);
    } else {
        PXR_NS::SdfPayload payload(dstLayerPath, dstPrimPath);
        cachePrim.GetPayloads().AddPayload(payload, position);
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
    auto pulledPathStr = getDictString(context, UsdUfe::EditRoutingTokens->Prim);
    auto fileFormatExtension
        = getDictString(context, PXR_NS::MayaUsdEditRoutingTokens->DefaultUSDFormat);
    auto dstLayerPath
        = getDictString(context, PXR_NS::MayaUsdEditRoutingTokens->DestinationLayerPath);
    const bool makePathRelative
        = (getDictValue(context, PXR_NS::MayaUsdEditRoutingTokens->RelativePath, 1) != 0);
    auto dstPrimName
        = getDictString(context, PXR_NS::MayaUsdEditRoutingTokens->DestinationPrimName);
    bool appendListEdit
        = (getDictString(context, PXR_NS::MayaUsdEditRoutingTokens->ListEditType, "Append")
           == "Append");
    bool asReference
        = (getDictString(context, PXR_NS::MayaUsdEditRoutingTokens->PayloadOrReference, "")
           == "Reference");
    bool dstIsVariant
        = (getDictValue(context, PXR_NS::MayaUsdEditRoutingTokens->DefineInVariant, 1) == 1);
    auto dstVariantSet = getDictString(context, PXR_NS::MayaUsdEditRoutingTokens->VariantSetName);
    auto dstVariant = getDictString(context, PXR_NS::MayaUsdEditRoutingTokens->VariantName);

    if (!PXR_NS::SdfPath::IsValidPathString(pulledPathStr))
        return;

    const PXR_NS::SdfPath pulledPath(pulledPathStr);
    const PXR_NS::SdfPath pulledParentPath = pulledPath.GetParentPath();

    std::string relDistLayerPath = dstLayerPath;
    if (makePathRelative) {
        const std::string layerDirPath = MayaUsd::getTargetLayerFolder(stage);
        if (layerDirPath.empty()) {
            // Making the path relative is postponed until the containing layer is saved.
            auto layer = UsdUfe::getCurrentTargetLayer(stage);
            PXR_NS::UsdMayaUtilFileSystem::markPathAsPostponedRelative(layer, dstLayerPath);
        } else {
            const auto relativePathAndSuccess
                = PXR_NS::UsdMayaUtilFileSystem::makePathRelativeTo(dstLayerPath, layerDirPath);
            if (relativePathAndSuccess.second) {
                relDistLayerPath = relativePathAndSuccess.first;
            } else {
                PXR_NAMESPACE_USING_DIRECTIVE
                TF_WARN(
                    "File name (%s) cannot be resolved as relative to the current edit target "
                    "layer, "
                    "using the absolute path.",
                    dstLayerPath.c_str());
            }
        }
    }

    if (relDistLayerPath.empty() || dstPrimName.empty())
        return;

    // Determine the file format
    PXR_NS::SdfLayer::FileFormatArguments fileFormatArgs;
    PXR_NS::SdfFileFormatConstPtr         fileFormat;

    if (fileFormatExtension.size() > 0) {

#if PXR_VERSION < 2508
        fileFormatArgs[PXR_NS::UsdUsdFileFormatTokens->FormatArg] = fileFormatExtension;
#else
        fileFormatArgs[PXR_NS::SdfUsdFileFormatTokens->FormatArg] = fileFormatExtension;
#endif
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
        auto srcStage = getDictValue(
            context, PXR_NS::MayaUsdEditRoutingTokens->SrcStage, PXR_NS::UsdStageRefPtr());
        auto srcLayer = getDictValue(
            context, PXR_NS::MayaUsdEditRoutingTokens->SrcLayer, PXR_NS::SdfLayerRefPtr());
        auto srcSdfPath
            = getDictValue(context, PXR_NS::MayaUsdEditRoutingTokens->SrcPath, PXR_NS::SdfPath());
        auto dstStage = getDictValue(
            context, PXR_NS::MayaUsdEditRoutingTokens->DstStage, PXR_NS::UsdStageRefPtr());
        auto dstLayer = getDictValue(
            context, PXR_NS::MayaUsdEditRoutingTokens->DstLayer, PXR_NS::SdfLayerRefPtr());
        auto dstSdfPath
            = getDictValue(context, PXR_NS::MayaUsdEditRoutingTokens->DstPath, PXR_NS::SdfPath());

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
                stage, relDistLayerPath, dstPrimPath, cachePrimPath, asReference, appendListEdit);
        }
    } else {
        createCachePrim(
            stage, relDistLayerPath, dstPrimPath, cachePrimPath, asReference, appendListEdit);
    }

    // Fill routing info
    routingData[UsdUfe::EditRoutingTokens->Layer] = PXR_NS::VtValue(dstLayerPath);
    routingData[PXR_NS::MayaUsdEditRoutingTokens->SaveLayer] = PXR_NS::VtValue("yes");
    routingData[PXR_NS::MayaUsdEditRoutingTokens->Path] = PXR_NS::VtValue(dstPrimPath.GetString());
}

} // namespace

namespace MAYAUSD_NS_DEF {

void registerMayaEditRouters()
{
    UsdUfe::registerDefaultEditRouter(
        PXR_NS::MayaUsdEditRoutingTokens->RouteCacheToUSD,
        std::make_shared<UsdUfe::CxxEditRouter>(cacheMayaReference));
}

} // namespace MAYAUSD_NS_DEF
