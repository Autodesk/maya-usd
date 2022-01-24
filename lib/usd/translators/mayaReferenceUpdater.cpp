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
    const MFnDependencyNode& depNodeFn,
    const Ufe::Path&         path)
    : UsdMayaPrimUpdater(depNodeFn, path)
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
UsdMayaPrimUpdater::PushCopySpecs PxrUsdTranslators_MayaReferenceUpdater::pushCopySpecs(
    UsdStageRefPtr srcStage,
    SdfLayerRefPtr srcLayer,
    const SdfPath& srcSdfPath,
    UsdStageRefPtr dstStage,
    SdfLayerRefPtr dstLayer,
    const SdfPath& dstSdfPath)
{
    bool success = false;

    // Prototype code, subject to change shortly.  PPT, 3-Nov-2021.
    // We are looking for a very specific configuration in here
    // i.e. a parent prim with a variant set called "animVariant"
    // and two variants "cache" and "rig"
    SdfPath parentSdfPath = dstSdfPath.GetParentPath();
    UsdPrim primWithVariant = dstStage->GetPrimAtPath(parentSdfPath);

    // Switching variant to cache to discover payload and where to write the data
    UsdVariantSet variantSet = primWithVariant.GetVariantSet("animVariant");
    variantSet.SetVariantSelection("cache");

    // Find the layer and prim to which we need to copy the content of push
    // There is currently no easy way to query this information at prim level
    // so we go to pcp and sdf.
    UsdPrim                 primWithPayload = primWithVariant.GetChildren().front();
    UsdPrimCompositionQuery query(primWithPayload);
    for (const auto& arc : query.GetCompositionArcs()) {
        if (arc.GetArcType() == PcpArcTypePayload) {
            SdfLayerHandle payloadLayer
                = arc.GetTargetNode().GetLayerStack()->GetIdentifier().rootLayer;
            SdfPath payloadPrimPath = arc.GetTargetNode().GetPath();
            // The Maya reference is meant as a cache, and therefore fully
            // overwritten, so we don't call MayaUsdUtils::mergePrims().
            success = SdfCopySpec(srcLayer, srcSdfPath, payloadLayer, payloadPrimPath);

            // As of 13-Dec-2021 pushEnd() will not be called on the
            // MayaReferenceUpdater, because the prim updater type information
            // is not correctly preserved.  Unload the reference here.  PPT.
            if (success) {
                const MObject& parentNode = getMayaObject();
                UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);
            }

            break;
        }
    }

    // If we successfully found the payload arc, no further traversal should
    // take place.
    return success ? PushCopySpecs::Prune : PushCopySpecs::Failed;
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::discardEdits(const UsdMayaPrimUpdaterContext& context)
{
    const MObject& parentNode = getMayaObject();
    UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);

    return UsdMayaPrimUpdater::discardEdits(context);
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::pushEnd(const UsdMayaPrimUpdaterContext& context)
{
    const MObject& parentNode = getMayaObject();
    return UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode) == MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
