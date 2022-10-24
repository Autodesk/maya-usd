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
#include <mayaUsdUtils/util.h>

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

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Metadata key used to store pull information on a prim
const TfToken kPullPrimMetadataKey("Maya:Pull:DagPath");

// Metadata key used to store pull information on a DG node
const MString kPullDGMetadataKey("Pull_UfePath");

} // namespace

//------------------------------------------------------------------------------
//

bool readPullInformation(const PXR_NS::UsdPrim& prim, std::string& dagPathStr)
{
    auto value = prim.GetCustomDataByKey(kPullPrimMetadataKey);
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
    auto        prim = MayaUsd::ufe::ufePathToPrim(ufePath);
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

bool writePulledPrimMetadata(const Ufe::Path& ufePulledPath, const MDagPath& editedAsMayaRoot)
{
    auto pulledPrim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    if (!pulledPrim)
        return false;
    return writePulledPrimMetadata(pulledPrim, editedAsMayaRoot);
}

bool writePulledPrimMetadata(UsdPrim& pulledPrim, const MDagPath& editedAsMayaRoot)
{
    auto stage = pulledPrim.GetStage();
    if (!stage)
        return false;

    UsdEditContext editContext(stage, stage->GetSessionLayer());
    VtValue        value(editedAsMayaRoot.fullPathName().asChar());
    return pulledPrim.SetMetadataByDictKey(SdfFieldKeys->CustomData, kPullPrimMetadataKey, value);
}

//------------------------------------------------------------------------------
//

void removePulledPrimMetadata(const Ufe::Path& ufePulledPath)
{
    UsdPrim     prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    UsdStagePtr stage = prim.GetStage();
    if (!stage)
        return;
    removePulledPrimMetadata(stage, prim);
}

void removePulledPrimMetadata(const UsdStagePtr& stage, UsdPrim& prim)
{
    UsdEditContext editContext(stage, stage->GetSessionLayer());
    prim.ClearCustomDataByKey(kPullPrimMetadataKey);
}

//------------------------------------------------------------------------------
//

bool addExcludeFromRendering(const Ufe::Path& ufePulledPath)
{
    UsdPrim prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);

    auto stage = prim.GetStage();
    if (!stage)
        return false;

    UsdEditContext editContext(stage, stage->GetSessionLayer());
    if (!prim.SetActive(false))
        return false;

    return true;
}

//------------------------------------------------------------------------------
//

bool removeExcludeFromRendering(const Ufe::Path& ufePulledPath)
{
    UsdPrim prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);

    auto stage = prim.GetStage();
    if (!stage)
        return false;

    SdfLayerHandle sessionLayer = stage->GetSessionLayer();
    UsdEditContext editContext(stage, sessionLayer);

    // Cleanup the field and potentially empty over
    if (!prim.ClearActive())
        return false;

    SdfPrimSpecHandle primSpec = MayaUsdUtils::getPrimSpecAtEditTarget(prim);
    if (sessionLayer && primSpec)
        sessionLayer->ScheduleRemoveIfInert(primSpec.GetSpec());

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
