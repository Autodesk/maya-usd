//
// Copyright 2016 Pixar
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
#include "mayaReferenceUpdater.h"

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/fileio/primUpdaterRegistry.h>
#include <mayaUsd/fileio/translators/translatorMayaReference.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/xformStack.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/editRouter.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilSerialization.h>
#include <mayaUsd/utils/variants.h>
#include <mayaUsdUtils/MergePrims.h>
#include <mayaUsd_Schemas/ALMayaReference.h>
#include <mayaUsd_Schemas/MayaReference.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/layerStackIdentifier.h>
#include <pxr/usd/pcp/node.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnAttribute.h>

namespace {

using AutoVariantRestore = MAYAUSD_NS_DEF::AutoVariantRestore;

// Clear the auto-edit flag on a USD Maya Reference so that it does not
// get edited immediately again. Clear in all variants, since each
// variant has its own copy of the flag.
void clearAutoEdit(const UsdPrim& prim)
{
    UsdPrim parentPrim = prim.GetParent();
    MAYAUSD_NS::applyToAllVariants(parentPrim, true, [prim]() {
        // Note: the prim might not exist in all variants, so check its validity.
        if (!prim.IsValid())
            return;

        UsdAttribute mayaAutoEditAttr = prim.GetAttribute(MayaUsd_SchemasTokens->mayaAutoEdit);
        if (mayaAutoEditAttr.IsValid())
            mayaAutoEditAttr.Set<bool>(false);
    });
}

std::string findValue(const PXR_NS::VtDictionary& routingData, const PXR_NS::TfToken& key)
{
    auto found = routingData.find(key);
    if (found == routingData.end() || !found->second.IsHolding<std::string>()) {
        return std::string();
    }
    return found->second.UncheckedGet<std::string>();
}

// Verify if the prim at the given path in the given stage is a transformable.
bool isValidXformable(const UsdStageRefPtr& stage, const SdfPath& mayaRefPath)
{
    // Note: the operrator is tagged as explicit, so we need to use a bool context.
    return UsdGeomXformable::Get(stage, mayaRefPath) ? true : false;
}

// Find the variant edit target that contains the Maya Reference.
//
// Since both the variant set name and the variant name are under the user's
// control and that where these can be found could vary from plugin-to-plugin,
// we need to search for it.
bool findMayaRefVariantEditTarget(
    const UsdStageRefPtr&                stage,
    const SdfPath&                       mayaRefPath,
    const VtDictionary&                  context,
    std::unique_ptr<AutoVariantRestore>& variantRestore)
{
    // Find where the variant sets would be: in the parent of the reference.
    const std::string pulledPathStr = VtDictionaryGet<std::string>(context, "prim", VtDefault = "");

    if (!SdfPath::IsValidPathString(pulledPathStr))
        return false;

    const SdfPath pulledPath(pulledPathStr);
    if (!pulledPath.IsPrimPath())
        return false;

    // Search each variant in each variant set for a Maya Reference.
    const SdfPath  primWithVariantPath = pulledPath.GetParentPath();
    UsdPrim        primWithVariant = stage->GetPrimAtPath(primWithVariantPath);
    UsdVariantSets variantSets = primWithVariant.GetVariantSets();
    for (const std::string& variantSetName : variantSets.GetNames()) {
        UsdVariantSet variantSet = primWithVariant.GetVariantSet(variantSetName);
        std::string   mayaRefVariantName;

        // We will need to restore the variant before creating the final, returned
        // AutoVariantRestore. That is why we are using a scope here. Otherwise,
        // the final AutoVariantRestore would capture the wrong "current variant".
        {
            AutoVariantRestore currentVariant(variantSet);
            for (const std::string& variantName : variantSet.GetVariantNames()) {
                variantSet.SetVariantSelection(variantName);

                if (isValidXformable(stage, mayaRefPath)) {
                    mayaRefVariantName = variantName;
                    break;
                }
            }
        }

        // Note: UsdVariantSet has no publicly accessible constructor except the
        //       copy constructor. In addition to that, it is returned by value
        //       from UsdPrim. So that is why AutoVariantRestore is returned with
        //       a unique_ptr.
        if (!mayaRefVariantName.empty()) {
            variantRestore = std::make_unique<AutoVariantRestore>(variantSet);
            variantSet.SetVariantSelection(mayaRefVariantName);
            return true;
        }
    }

    return false;
}

// Find the edit target that contains the Maya Reference.
bool findMayaRefEditTarget(
    const UsdStageRefPtr&                stage,
    const SdfPath&                       mayaRefPath,
    const VtDictionary&                  context,
    std::unique_ptr<AutoVariantRestore>& variantRestore)
{
    // First, let's see if we can find the Maya Reference right-away.
    if (isValidXformable(stage, mayaRefPath))
        return true;

    // Otherwise assume that it is in a different variant.
    return findMayaRefVariantEditTarget(stage, mayaRefPath, context, variantRestore);
}

// Copy the transform from the source to target transformable.
//
// For a while we used mergePrims(), but that fails to do the right thing if the source
// is not in a variant but the target is.
void copyTransform(const UsdGeomXformable& srcXform, UsdGeomXformable& dstXform)
{
    UsdGeomXformable::XformQuery srcQuery(srcXform);
    std::vector<double>          times;
    dstXform.ClearXformOpOrder();
    UsdGeomXformOp dstOp = dstXform.AddTransformOp();
    if (srcQuery.TransformMightBeTimeVarying()) {
        srcQuery.GetTimeSamples(&times);
        for (auto time : times) {
            GfMatrix4d mtx;
            srcQuery.GetLocalTransformation(&mtx, UsdTimeCode(time));
            dstOp.Set(mtx, time);
        }
    } else {
        GfMatrix4d mtx;
        srcQuery.GetLocalTransformation(&mtx, UsdTimeCode());
        dstOp.Set(mtx, UsdTimeCode());
    }
}

} // namespace

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_UPDATER(
    MayaUsd_SchemasMayaReference,
    reference,
    PxrUsdTranslators_MayaReferenceUpdater,
    (UsdMayaPrimUpdater::Supports::Push | UsdMayaPrimUpdater::Supports::Clear
     | UsdMayaPrimUpdater::Supports::AutoPull));
PXRUSDMAYA_REGISTER_UPDATER(
    MayaUsd_SchemasALMayaReference,
    reference,
    PxrUsdTranslators_MayaReferenceUpdater,
    (UsdMayaPrimUpdater::Supports::Push | UsdMayaPrimUpdater::Supports::Clear
     | UsdMayaPrimUpdater::Supports::AutoPull));

PxrUsdTranslators_MayaReferenceUpdater::PxrUsdTranslators_MayaReferenceUpdater(
    const UsdMayaPrimUpdaterContext& context,
    const MFnDependencyNode&         depNodeFn,
    const Ufe::Path&                 path)
    : UsdMayaPrimUpdater(context, depNodeFn, path)
{
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::shouldAutoEdit() const
{
    UsdPrim prim = getUsdPrim();

    // Check to see if auto-edit is enabled
    UsdAttribute mayaAutoEditAttr = prim.GetAttribute(MayaUsd_SchemasTokens->mayaAutoEdit);
    bool         autoEdit;
    mayaAutoEditAttr.Get<bool>(&autoEdit);
    if (!autoEdit) {
        return false;
    }

    // Check to see if we have a valid Maya reference attribute
    SdfAssetPath mayaReferenceAssetPath;
    UsdAttribute mayaReferenceAttr = prim.GetAttribute(MayaUsd_SchemasTokens->mayaReference);
    mayaReferenceAttr.Get(&mayaReferenceAssetPath);

    MString mayaReferencePath(mayaReferenceAssetPath.GetResolvedPath().c_str());

    // The resolved path is empty if the maya reference is a full path.
    if (!mayaReferencePath.length())
        mayaReferencePath = mayaReferenceAssetPath.GetAssetPath().c_str();

    // If the path is still empty return, there is no reference to import
    if (!mayaReferencePath.length())
        return false;

    return true;
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::canEditAsMaya() const { return true; }

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::editAsMaya()
{
    // Lock the transform node that corresponds to the Maya reference prim.
    MDagPath transformPath;
    if (MDagPath::getAPathTo(getMayaObject(), transformPath) != MS::kSuccess) {
        return false;
    }

    MayaUsd::LockNodesUndoItem::lock(
        "Maya reference pulled transform locking", transformPath, true);

    // Lock all attributes except the transform attributes.  Create a set of
    // Maya attributes that are converted to USD transform attributes.
    // Children of compounds are ignored, as their parent will be locked.
    static std::set<std::string> xformAttrNames;
    if (xformAttrNames.empty()) {
        for (const auto& opClass : UsdMayaXformStack::MayaStack().GetOps()) {
            if (!opClass.IsInvertedTwin()) {
                xformAttrNames.insert(std::string(opClass.GetName().GetText()));
            }
        }
    }

    MFnDependencyNode fn(getMayaObject());
    auto              attrCount = fn.attributeCount();
    for (unsigned int i = 0; i < attrCount; ++i) {
        auto attr = fn.attribute(i);
        auto plug = fn.findPlug(attr, true);
        if (plug.isChild()) {
            // Skip children of compounds, as their parent will be locked.
            continue;
        }
        // If the attribute is not in our set of transform attributes, lock it.
        MFnAttribute fnAttr(attr);
        if (xformAttrNames.count(std::string(fnAttr.name().asChar())) == 0) {
            plug.setLocked(true);
        }
    }

    return true;
}

/* virtual */
UsdMayaPrimUpdater::PushCopySpecs PxrUsdTranslators_MayaReferenceUpdater::pushCopySpecs(
    UsdStageRefPtr srcStage,
    SdfLayerRefPtr srcLayer,
    const SdfPath& srcSdfPath,
    UsdStageRefPtr dstStage,
    SdfLayerRefPtr dstLayer,
    const SdfPath& dstSdfPath)
{
    // We need context to access user arguments
    if (!getContext()) {
        return PushCopySpecs::Failed;
    }

    // Use the edit router to find the destination layer and path.
    auto dstEditRouter = MayaUsd::getEditRouter(TfToken("mayaReferencePush"));
    if (!dstEditRouter) {
        return PushCopySpecs::Failed;
    }

    PXR_NS::VtDictionary routerContext = getContext()->GetUserArgs();
    routerContext["stage"] = PXR_NS::VtValue(getContext()->GetUsdStage());
    routerContext["prim"] = PXR_NS::VtValue(dstSdfPath.GetString());
    PXR_NS::VtDictionary routingData;

    (*dstEditRouter)(routerContext, routingData);

    // Retrieve the destination layer and prim path from the routing data.
    auto cacheDstLayerStr = findValue(routingData, TfToken("layer"));
    if (!TF_VERIFY(!cacheDstLayerStr.empty())) {
        return PushCopySpecs::Failed;
    }
    auto cacheDstPathStr = findValue(routingData, TfToken("path"));
    if (!TF_VERIFY(!cacheDstPathStr.empty())) {
        return PushCopySpecs::Failed;
    }

    // Copy transform changes that came from the Maya transform node into the
    // Maya reference prim.  The Maya transform node changes have already been
    // exported into the temporary layer as a transform prim, which is our
    // source.  The destination prim in the stage is the Maya reference prim.
    auto srcPrim = srcStage->GetPrimAtPath(srcSdfPath);
    UsdGeomXformable srcXform(srcPrim);
    if (TF_VERIFY(srcXform)) {
        std::unique_ptr<AutoVariantRestore> variantRestore;
        if (TF_VERIFY(findMayaRefEditTarget(dstStage, dstSdfPath, routerContext, variantRestore))) {
            // The Maya transform that corresponds to the Maya reference prim
            // only has its transform attributes unlocked.  Bring any transform
            // attribute edits over to the Maya reference prim.
            UsdEditTarget editTarget = dstStage->GetEditTarget();
            if (variantRestore) {
                editTarget
                    = variantRestore->getVariantSet().GetVariantEditTarget(editTarget.GetLayer());
            }
            UsdEditContext editContext(dstStage, editTarget);

            UsdPrim          dstPrim = dstStage->GetPrimAtPath(dstSdfPath);
            UsdGeomXformable dstXform(dstPrim);
            if (TF_VERIFY(dstXform)) {
                copyTransform(srcXform, dstXform);
            }
        }
    }

    auto cacheDstLayer = SdfLayer::FindOrOpen(cacheDstLayerStr);
    if (!TF_VERIFY(cacheDstLayer)) {
        return PushCopySpecs::Failed;
    }

    // The Maya reference is meant as a cache, and therefore fully
    // overwritten, so we don't call MayaUsdUtils::mergePrims().
    if (SdfCopySpec(srcLayer, srcSdfPath, cacheDstLayer, SdfPath(cacheDstPathStr))) {
        const MObject& parentNode = getMayaObject();
        UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);

        auto saveLayer = findValue(routingData, TfToken("save_layer"));
        if (saveLayer == "yes")
            cacheDstLayer->Save();

        // No further traversal should take place.
        return PushCopySpecs::Prune;
    }

    return PushCopySpecs::Failed;
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::discardEdits()
{
    const MObject& parentNode = getMayaObject();

    MDagPath dagPath;
    MStatus  status = MDagPath::getAPathTo(parentNode, dagPath);
    if (status == MS::kSuccess) {
        Ufe::Path pulledPath;
        if (PrimUpdaterManager::readPullInformation(dagPath, pulledPath)) {
            // Reset the auto-edit when discarding the edit.
            UsdPrim prim = MayaUsd::ufe::ufePathToPrim(pulledPath);
            clearAutoEdit(prim);
        }
    }

    UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);

    return UsdMayaPrimUpdater::discardEdits();
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::pushEnd()
{
    // Unnecessary to unlock individual attributes, as the Maya transform node
    // is removed at pushEnd().
    MDagPath transformPath;
    if (MDagPath::getAPathTo(getMayaObject(), transformPath) != MS::kSuccess) {
        return false;
    }

    MayaUsd::LockNodesUndoItem::lock(
        "Maya reference pulled transform unlocking", transformPath, false);

    // Clear the auto-edit flag.
    clearAutoEdit(getUsdPrim());

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
