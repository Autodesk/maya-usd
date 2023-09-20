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

#include "pullInformation.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/primActivation.h>
#include <mayaUsd/utils/variants.h>

#include <usdUfe/utils/usdUtils.h>

#include <pxr/usd/usd/editContext.h>

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <ufe/hierarchy.h>
#include <ufe/pathString.h>
#include <ufe/sceneItem.h>

namespace MAYAUSD_NS_DEF {

namespace {

// Metadata key used to store pull information on a prim.
//
// Note: we have two versions, because historically we did not author
//       the metadata inside variants. To support backward compatibility
//       we need to read from both, starting with the newer, variant-specific
//       one.
const PXR_NS::TfToken kPullPrimMetadataKey("Maya:Pull:DagPath");
const PXR_NS::TfToken kPullPrimVariantMetadataKey("Maya:PullVariant:DagPath");

// Metadata key used to store pull information on a DG node
const MString kPullDGMetadataKey("Pull_UfePath");

} // namespace

//------------------------------------------------------------------------------
//
// Read on the Maya node the information necessary to merge the USD prim
// that is edited as Maya.

bool readPullInformation(const PXR_NS::UsdPrim& prim, std::string& dagPathStr)
{
    PXR_NS::VtValue value = prim.GetCustomDataByKey(kPullPrimVariantMetadataKey);
    if (!value.IsEmpty() && value.CanCast<std::string>()) {
        dagPathStr = value.Get<std::string>();
        return !dagPathStr.empty();
    }
    value = prim.GetCustomDataByKey(kPullPrimMetadataKey);
    if (!value.IsEmpty() && value.CanCast<std::string>()) {
        dagPathStr = value.Get<std::string>();
        return !dagPathStr.empty();
    }
    return false;
}

bool readPullInformation(const PXR_NS::UsdPrim& prim, Ufe::SceneItem::Ptr& dagPathItem)
{
    std::string dagPathStr;
    if (readPullInformation(prim, dagPathStr)) {
        dagPathItem = Ufe::Hierarchy::createItem(Ufe::PathString::path(dagPathStr));
        return (bool)dagPathItem;
    }
    return false;
}

bool readPullInformation(const Ufe::Path& ufePath, MDagPath& dagPath)
{
    auto prim = MayaUsd::ufe::ufePathToPrim(ufePath);
    if (!prim.IsValid())
        return false;

    std::string dagPathStr;
    if (readPullInformation(prim, dagPathStr)) {
        MSelectionList sel;
        sel.add(dagPathStr.c_str());
        sel.getDagPath(0, dagPath);
        return dagPath.isValid();
    }
    return false;
}

bool readPullInformation(const MDagPath& dagPath, Ufe::Path& ufePath)
{
    MStatus status;

    MFnDependencyNode depNode(dagPath.node());
    MPlug             dgMetadata = depNode.findPlug(kPullDGMetadataKey, &status);
    if (status == MStatus::kSuccess) {
        MString pulledUfePathStr;
        status = dgMetadata.getValue(pulledUfePathStr);
        if (status) {
            ufePath = Ufe::PathString::path(pulledUfePathStr.asChar());
            return !ufePath.empty();
        }
    }

    return false;
}

//------------------------------------------------------------------------------
//
// Write on the Maya node the information necessary later-on to merge
// the USD prim that is edited as Maya.

bool writePullInformation(const Ufe::Path& ufePulledPath, const MDagPath& editedAsMayaRoot)
{
    auto              ufePathString = Ufe::PathString::string(ufePulledPath);
    MFnDependencyNode depNode(editedAsMayaRoot.node());
    MStatus           status;
    MPlug             dgMetadata = depNode.findPlug(kPullDGMetadataKey, &status);
    if (status != MStatus::kSuccess) {
        MFnStringData fnStringData;
        MObject       strAttrObject = fnStringData.create("");

        MFnTypedAttribute attr;
        MObject           attrObj
            = attr.create(kPullDGMetadataKey, kPullDGMetadataKey, MFnData::kString, strAttrObject);
        status = depNode.addAttribute(attrObj);
        dgMetadata = depNode.findPlug(kPullDGMetadataKey, &status);
        if (status != MStatus::kSuccess) {
            return false;
        }
    }
    return dgMetadata.setValue(ufePathString.c_str());
}

//------------------------------------------------------------------------------
//
// Write on the USD prim the information necessary later-on to merge
// the USD prim that is edited as Maya.

bool writePulledPrimMetadata(const Ufe::Path& ufePulledPath, const MDagPath& editedAsMayaRoot)
{
    return writePulledPrimMetadata(ufePulledPath, editedAsMayaRoot, PXR_NS::UsdEditTarget());
}

bool writePulledPrimMetadata(
    const Ufe::Path&             ufePulledPath,
    const MDagPath&              editedAsMayaRoot,
    const PXR_NS::UsdEditTarget& editTarget)
{
    auto pulledPrim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    if (!pulledPrim)
        return false;

    return writePulledPrimMetadata(pulledPrim, editedAsMayaRoot, editTarget);
}

bool writePulledPrimMetadata(PXR_NS::UsdPrim& pulledPrim, const MDagPath& editedAsMayaRoot)
{
    return writePulledPrimMetadata(pulledPrim, editedAsMayaRoot, PXR_NS::UsdEditTarget());
}

bool writePulledPrimMetadata(
    PXR_NS::UsdPrim&             pulledPrim,
    const MDagPath&              editedAsMayaRoot,
    const PXR_NS::UsdEditTarget& editTarget)
{
    auto stage = pulledPrim.GetStage();
    if (!stage)
        return false;

    // If the edit target is null, then target the exact set of variants under
    // which the USD prim lives to set the custom metadata. That way if multiple
    // prims with the same name but under different variants exists, they won't
    // step on each other's data.
    const PXR_NS::UsdEditContext editContext(
        stage,
        editTarget.IsNull() ? getEditTargetForVariants(pulledPrim, stage->GetSessionLayer())
                            : editTarget);

    const PXR_NS::VtValue value(editedAsMayaRoot.fullPathName().asChar());
    return pulledPrim.SetMetadataByDictKey(
        SdfFieldKeys->CustomData, kPullPrimVariantMetadataKey, value);
}

//------------------------------------------------------------------------------
//
// Remove from the USD prim the information necessary to merge the USD prim
// that was edited as Maya.

void removePulledPrimMetadata(const Ufe::Path& ufePulledPath)
{
    removePulledPrimMetadata(ufePulledPath, PXR_NS::UsdEditTarget());
}

void removePulledPrimMetadata(
    const Ufe::Path&             ufePulledPath,
    const PXR_NS::UsdEditTarget& editTarget)
{
    PXR_NS::UsdPrim prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    if (!prim.IsValid()) {
        TF_WARN(
            "Could not find prim to remove pulled prim metadata on %s.",
            ufePulledPath.string().c_str());
        return;
    }

    PXR_NS::UsdStagePtr stage = prim.GetStage();
    if (!stage)
        return;

    removePulledPrimMetadata(stage, prim, editTarget);
}

void removePulledPrimMetadata(const PXR_NS::UsdStagePtr& stage, PXR_NS::UsdPrim& pulledPrim)
{
    removePulledPrimMetadata(stage, pulledPrim, PXR_NS::UsdEditTarget());
}

void removePulledPrimMetadata(
    const PXR_NS::UsdStagePtr&   stage,
    PXR_NS::UsdPrim&             pulledPrim,
    const PXR_NS::UsdEditTarget& editTarget)
{
    // Note: this is the old prim data that was not variant-specific,
    //       so it is removed without using the edit target.
    pulledPrim.ClearCustomDataByKey(kPullPrimMetadataKey);

    // If the edit target is null, then target the exact set of variants
    // under which the USD prim lives to clear the custom metadata. That
    // way if multiple prims with the same name but under  different
    // variants exists, they won't step on each other's data.
    {
        const PXR_NS::UsdEditContext editContext(
            stage,
            editTarget.IsNull() ? getEditTargetForVariants(pulledPrim, stage->GetSessionLayer())
                                : editTarget);

        pulledPrim.ClearCustomDataByKey(kPullPrimVariantMetadataKey);
    }

    // Session layer cleanup.
    auto rootPrims = stage->GetSessionLayer()->GetRootPrims();
    for (const SdfPrimSpecHandle& rootPrimSpec : rootPrims) {
        stage->GetSessionLayer()->RemovePrimIfInert(rootPrimSpec);
    }
}

//------------------------------------------------------------------------------
//
// Hide the USD prim that is edited as Maya.
// This is done so that the USD prim and edited Maya data are not superposed
// in the viewport.

bool addExcludeFromRendering(const Ufe::Path& ufePulledPath)
{
    // Note: passing an invalid edit target will write the exclusion
    //       in the active variants in the session layer.
    return addExcludeFromRendering(ufePulledPath, PXR_NS::UsdEditTarget());
}

bool addExcludeFromRendering(
    const Ufe::Path&             ufePulledPath,
    const PXR_NS::UsdEditTarget& editTarget)
{
    // Note: must make sure the prim is accessible by activating all its ancestors.
    PrimActivation activation(ufePulledPath);

    PXR_NS::UsdPrim prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    if (!prim.IsValid())
        return false;

    auto stage = prim.GetStage();
    if (!stage)
        return false;

    // Receiving an invalid edit target means that we write the exclusion
    // in the active variants in the session layer.
    if (editTarget.IsNull()) {
        const PXR_NS::SdfLayerHandle layer = stage->GetSessionLayer();
        const PXR_NS::UsdEditTarget  editTarget = getEditTargetForVariants(prim, layer);

        PXR_NS::UsdEditContext editContext(stage, stage->GetSessionLayer());
        if (!prim.SetActive(false))
            return false;
    } else {
        PXR_NS::UsdEditContext editContext(stage, editTarget);
        if (!prim.SetActive(false))
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------
//
// Show again the USD prim that was edited as Maya.
// This is done once the Maya data is meged into USD and removed from the scene.

bool removeExcludeFromRendering(const Ufe::Path& ufePulledPath)
{
    // Note: passing an invalid edit target will remove the exclusion
    //       from the active variants in the session layer.
    return removeExcludeFromRendering(ufePulledPath, PXR_NS::UsdEditTarget());
}

bool removeExcludeFromRendering(
    const Ufe::Path&             ufePulledPath,
    const PXR_NS::UsdEditTarget& editTarget)
{
    // Note: must make sure the prim is accessible by activating all its ancestors.
    PrimActivation activation(ufePulledPath);

    PXR_NS::UsdPrim prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    if (!prim.IsValid())
        return false;

    // If already active, nothing to do. This happens in some recursive
    // notification situations.
    if (prim.IsActive())
        return true;

    auto stage = prim.GetStage();
    if (!stage)
        return false;

    PXR_NS::SdfLayerHandle sessionLayer = stage->GetSessionLayer();
    if (!sessionLayer)
        return false;

    auto clearActive = [&stage, &sessionLayer, &prim](const PXR_NS::UsdEditTarget& editTarget) {
        PXR_NS::UsdEditContext editContext(stage, editTarget);

        // Cleanup the field and potentially empty over
        prim.ClearActive();

        PXR_NS::SdfPrimSpecHandle primSpec = UsdUfe::getPrimSpecAtEditTarget(prim);
        if (primSpec)
            sessionLayer->ScheduleRemoveIfInert(primSpec.GetSpec());
    };

    // Note: older version of MayaUSD wrote the exclusion outside all variants,
    //       so for backward compatibility, we always try to remove it from there.
    clearActive(PXR_NS::UsdEditTarget(sessionLayer));

    // Remove the exclusion in the correct variants, if any.
    if (editTarget.IsNull()) {
        clearActive(getEditTargetForVariants(prim, sessionLayer));
    } else {
        clearActive(editTarget);
    }

    return true;
}

//------------------------------------------------------------------------------
//
// Verify if the edited as Maya nodes corresponding to the given prim is orphaned.

bool isEditedAsMayaOrphaned(const PXR_NS::UsdPrim& prim)
{
    std::string dagPathStr;
    return !readPullInformation(prim, dagPathStr);
}

MAYAUSD_CORE_PUBLIC
bool isEditedAsMayaOrphaned(const Ufe::Path& editedUsdPrim)
{
    MDagPath dagPath;
    return !readPullInformation(editedUsdPrim, dagPath);
}

} // namespace MAYAUSD_NS_DEF
