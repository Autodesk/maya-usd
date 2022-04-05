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

#include <mayaUsd/fileio/primUpdaterRegistry.h>
#include <mayaUsd/fileio/translators/translatorMayaReference.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/xformStack.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/editRouter.h>
#include <mayaUsd/utils/util.h>
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

/* virtual */
UsdMayaPrimUpdater::PushCopySpecs PxrUsdTranslators_MayaReferenceUpdater::pushCopySpecs(
    UsdStageRefPtr srcStage,
    SdfLayerRefPtr srcLayer,
    const SdfPath& srcSdfPath,
    UsdStageRefPtr /* dstStage */,
    SdfLayerRefPtr /* dstLayer */,
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
    auto dstLayerStr = findValue(routingData, TfToken("layer"));
    if (!TF_VERIFY(!dstLayerStr.empty())) {
        return PushCopySpecs::Failed;
    }
    auto dstPathStr = findValue(routingData, TfToken("path"));
    if (!TF_VERIFY(!dstPathStr.empty())) {
        return PushCopySpecs::Failed;
    }

    auto dstLayer = SdfLayer::FindOrOpen(dstLayerStr);
    if (!TF_VERIFY(dstLayer)) {
        return PushCopySpecs::Failed;
    }
    SdfPath dstPath(dstPathStr);

    // The Maya reference is meant as a cache, and therefore fully
    // overwritten, so we don't call MayaUsdUtils::mergePrims().
    // As of 13-Dec-2021 pushEnd() will not be called on the
    // MayaReferenceUpdater, because the prim updater type information
    // is not correctly preserved.  Unload the reference here.  PPT.
    if (SdfCopySpec(srcLayer, srcSdfPath, dstLayer, dstPath)) {
        const MObject& parentNode = getMayaObject();
        UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);

        auto dstLayerStr = findValue(routingData, TfToken("save_layer"));
        if (dstLayerStr == "yes")
            dstLayer->Save();

        // No further traversal should take place.
        return PushCopySpecs::Prune;
    }

    return PushCopySpecs::Failed;
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::discardEdits()
{
    const MObject& parentNode = getMayaObject();
    UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);

    return UsdMayaPrimUpdater::discardEdits();
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::pushEnd()
{
    // As of 25-Feb-2022 the Maya transform node pulled from the Maya reference
    // prim ends up being unlocked by the unlock traversal in
    // PrimUpdaterManager::mergeToUsd().  However, more robust to enforce
    // separation of concerns and perform the inverse of editAsMaya() here.

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
