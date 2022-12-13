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
#include <mayaUsd/ufe/SetVariantSelectionCommand.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/editRouter.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilSerialization.h>
#include <mayaUsd/utils/variants.h>
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

// Clear the auto-edit flag on a USD Maya Reference so that it does not
// get edited immediately again. Clear in all variants, since each
// variant has its own copy of the flag.
void clearAutoEdit(const UsdPrim& prim)
{
    // The given prim can be invalid. This happens for example if an
    // ancestor was deactivated.
    if (!prim.IsValid())
        return;

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

bool callEditRouter(
    const char*                 routerName,
    const PXR_NS::VtDictionary& routerContext,
    PXR_NS::VtDictionary&       routingData)
{
    MayaUsd::EditRouter::Ptr dstEditRouter = MayaUsd::getEditRouter(TfToken(routerName));
    if (!dstEditRouter)
        return false;

    (*dstEditRouter)(routerContext, routingData);
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

    PXR_NS::VtDictionary routerContext = getContext()->GetUserArgs();

    // Pass the source and destination stage, layer and path to routers.
    routerContext["src_stage"] = PXR_NS::VtValue(srcStage);
    routerContext["src_layer"] = PXR_NS::VtValue(srcLayer);
    routerContext["src_path"] = PXR_NS::VtValue(srcSdfPath);

    routerContext["dst_stage"] = PXR_NS::VtValue(dstStage);
    routerContext["dst_layer"] = PXR_NS::VtValue(dstLayer);
    routerContext["dst_path"] = PXR_NS::VtValue(dstSdfPath);

    // Use the edit router to find the destination layer and path.
    routerContext["stage"] = PXR_NS::VtValue(getContext()->GetUsdStage());
    routerContext["prim"] = PXR_NS::VtValue(dstSdfPath.GetString());

    PXR_NS::VtDictionary routingData;

    if (!callEditRouter("mayaReferencePush", routerContext, routingData))
        return PushCopySpecs::Failed;

    // Retrieve the destination layer and prim path from the routing data.
    auto cacheDstLayerStr = findValue(routingData, TfToken("layer"));
    if (!TF_VERIFY(!cacheDstLayerStr.empty()))
        return PushCopySpecs::Failed;

    auto cacheDstPathStr = findValue(routingData, TfToken("path"));
    if (!TF_VERIFY(!cacheDstPathStr.empty()))
        return PushCopySpecs::Failed;

    auto cacheDstLayer = SdfLayer::FindOrOpen(cacheDstLayerStr);
    if (!TF_VERIFY(cacheDstLayer))
        return PushCopySpecs::Failed;

    // The Maya reference is meant as a cache, and therefore fully
    // overwritten, so we don't call MayaUsdUtils::mergePrims().
    if (!SdfCopySpec(srcLayer, srcSdfPath, cacheDstLayer, SdfPath(cacheDstPathStr)))
        return PushCopySpecs::Failed;

    const MObject& parentNode = getMayaObject();
    UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);

    auto saveLayer = findValue(routingData, TfToken("save_layer"));
    if (saveLayer == "yes")
        cacheDstLayer->Save();

    // No further traversal should take place.
    return PushCopySpecs::Prune;
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::discardEdits()
{
    const MObject& parentNode = getMayaObject();

    MDagPath dagPath;
    MStatus  status = MDagPath::getAPathTo(parentNode, dagPath);
    if (status == MS::kSuccess) {
        Ufe::Path pulledPath;
        if (MAYAUSD_NS_DEF::readPullInformation(dagPath, pulledPath)) {
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

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
