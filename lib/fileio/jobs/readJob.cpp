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
#include "readJob.h"

#include "../primReaderRegistry.h"
#include "../../utils/stageCache.h"
#include "../../nodes/stageNode.h"
#include "../translators/translatorMaterial.h"
#include "../translators/translatorXformable.h"
#include "../../utils/util.h"

#include "pxr/base/tf/token.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primFlags.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usd/usdUtils/stageCache.h"

#include <maya/MAnimControl.h>
#include <maya/MDagModifier.h>
#include <maya/MDGModifier.h>
#include <maya/MDistance.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

UsdMaya_ReadJob::UsdMaya_ReadJob(
        const std::string &iFileName,
        const std::string &iPrimPath,
        const std::map<std::string, std::string>& iVariants,
        const UsdMayaJobImportArgs &iArgs) :
    mArgs(iArgs),
    mFileName(iFileName),
    mVariants(iVariants),
    mMayaRootDagPath(),
    mPrimPath(iPrimPath),
    mDagModifierUndo(),
    mDagModifierSeeded(false)
{
}

UsdMaya_ReadJob::~UsdMaya_ReadJob()
{
}

bool
UsdMaya_ReadJob::Read(std::vector<MDagPath>* addedDagPaths)
{
    MStatus status;

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(mFileName);
    if (!rootLayer) {
        return false;
    }

    TfToken modelName = UsdUtilsGetModelNameFromRootLayer(rootLayer);

    std::vector<std::pair<std::string, std::string> > varSelsVec;
    TF_FOR_ALL(iter, mVariants) {
        const std::string& variantSetName = iter->first;
        const std::string& variantSelectionName = iter->second;
        varSelsVec.push_back(
            std::make_pair(variantSetName, variantSelectionName));
    }

    SdfLayerRefPtr sessionLayer =
        UsdUtilsStageCache::GetSessionLayerForVariantSelections(modelName,
                                                                varSelsVec);

    // Layer and Stage used to Read in the USD file
    UsdStageCacheContext stageCacheContext(UsdMayaStageCache::Get());
    UsdStageRefPtr stage = UsdStage::Open(rootLayer, sessionLayer);
    if (!stage) {
        return false;
    }

    stage->SetEditTarget(stage->GetSessionLayer());

    // XXX Currently all distance values are set directly from USD and will be 
    // interpreted as centimeters (Maya's internal distance unit). Future work
    // could include converting distance values based on the specified meters-
    // per-unit in the USD stage metadata. For now, simply warn. 
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        MDistance::Unit mdistanceUnit = 
            UsdMayaUtil::ConvertUsdGeomLinearUnitToMDistanceUnit(
                UsdGeomGetStageMetersPerUnit(stage));

        if (mdistanceUnit != MDistance::internalUnit()) {
            TF_WARN("Distance unit conversion is not yet supported. "
                "All distance values will be imported in Maya's internal "
                "distance unit.");
        }
    }

    // If the import time interval isn't empty, we expand the Min/Max time
    // sliders to include the stage's range if necessary.
    if (!mArgs.timeInterval.IsEmpty()) {
        MTime currentMinTime = MAnimControl::minTime();
        MTime currentMaxTime = MAnimControl::maxTime();

        GfInterval stageInterval;
        if (mArgs.timeInterval.IsFinite()) {
            if (mArgs.timeInterval.GetMin() > mArgs.timeInterval.GetMax()) {
                TF_RUNTIME_ERROR(
                        "Frame range start (%f) was greater than end (%f)",
                        mArgs.timeInterval.GetMin(),
                        mArgs.timeInterval.GetMax());
                return false;
            }
            stageInterval = mArgs.timeInterval;
        } else {
            stageInterval.SetMin(stage->GetStartTimeCode());
            stageInterval.SetMax(stage->GetEndTimeCode());
        }

        if (stageInterval.GetMin() < currentMinTime.value()) {
            MAnimControl::setMinTime(MTime(stageInterval.GetMin()));
        }
        if (stageInterval.GetMax() > currentMaxTime.value()) {
            MAnimControl::setMaxTime(MTime(stageInterval.GetMax()));
        }
    }

    // Use the primPath to get the root usdNode
    UsdPrim usdRootPrim = mPrimPath.empty() ? stage->GetDefaultPrim() :
        stage->GetPrimAtPath(SdfPath(mPrimPath));
    if (!usdRootPrim && !(mPrimPath.empty() || mPrimPath == "/")) {
        TF_RUNTIME_ERROR(
                "Unable to set root prim to <%s> when reading USD file '%s'; "
                "using the pseudo-root </> instead",
                mPrimPath.c_str(), mFileName.c_str());
        usdRootPrim = stage->GetPseudoRoot();
    }

    bool isImportingPseudoRoot = (usdRootPrim == stage->GetPseudoRoot());

    if (!usdRootPrim) {
        TF_RUNTIME_ERROR(
                "No default prim found in USD file '%s'",
                mFileName.c_str());
        return false;
    }

    // Set the variants on the usdRootPrim
    for (auto& variant : mVariants) {
        usdRootPrim
                .GetVariantSet(variant.first)
                .SetVariantSelection(variant.second);
    }

    Usd_PrimFlagsPredicate predicate = UsdPrimDefaultPredicate;

    PreImport(predicate);

    UsdPrimRange range(usdRootPrim, predicate);
    if (range.empty()) {
        // XXX: This shouldn't really be possible, but it currently is because
        // combinations of nested assembly nodes with variant set selections
        // made in Maya are not being handled correctly. usdRootPrim can end up
        // being an "over" prim spec created by the parent assembly with no
        // scene description underneath, which results in an empty range.
        return false;
    }

    // We maintain a registry mapping SdfPaths to MObjects as we create Maya
    // nodes, so prime the registry with the root Maya node and the
    // usdRootPrim's path.
    SdfPath rootPathToRegister = usdRootPrim.GetPath();

    if (SkipRootPrim(isImportingPseudoRoot)) {
        range.increment_begin();
    } else {
        // Otherwise, associate the usdRootPrim's *parent* with the root Maya
        // node instead.
        rootPathToRegister = rootPathToRegister.GetParentPath();
    }

    mNewNodeRegistry.insert(std::make_pair(
                rootPathToRegister.GetString(),
                mMayaRootDagPath.node()));

    if (mArgs.useAsAnimationCache) {
        MDGModifier dgMod;
        MObject usdStageNode = dgMod.createNode(UsdMayaStageNode::typeId,
                                                &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        // We only ever create a single stage node per usdImport, so we can
        // simply register it and later look it up in the registry using its
        // type name.
        mNewNodeRegistry.insert(
            std::make_pair(UsdMayaStageNodeTokens->MayaTypeName.GetString(),
                           usdStageNode));

        MFnDependencyNode depNodeFn(usdStageNode, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug filePathPlug = depNodeFn.findPlug(UsdMayaStageNode::filePathAttr,
                                                true,
                                                &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.newPlugValueString(filePathPlug, mFileName.c_str());
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.doIt();
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    DoImport(range, usdRootPrim);

    SdfPathSet topImportedPaths;
    if (isImportingPseudoRoot) {
        // get all the dag paths for the root prims
        TF_FOR_ALL(childIter, stage->GetPseudoRoot().GetChildren()) {
            topImportedPaths.insert(childIter->GetPath());
        }
    } else {
        topImportedPaths.insert(usdRootPrim.GetPath());
    }

    TF_FOR_ALL(pathsIter, topImportedPaths) {
        std::string key = pathsIter->GetString();
        MObject obj;
        if (TfMapLookup(mNewNodeRegistry, key, &obj)) {
            if (obj.hasFn(MFn::kDagNode)) {
                addedDagPaths->push_back(MDagPath::getAPathTo(obj));
            }
        }
    }

    return (status == MS::kSuccess);
}

bool
UsdMaya_ReadJob::DoImport(UsdPrimRange& rootRange, const UsdPrim& usdRootPrim)
{
    return _DoImport(rootRange, usdRootPrim);
}

bool UsdMaya_ReadJob::OverridePrimReader(
    const UsdPrim&               usdRootPrim,
    const UsdPrim&               prim,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext&    readCtx,
    UsdPrimRange::iterator&      primIt
)
{
    return false;
}

bool
UsdMaya_ReadJob::_DoImport(UsdPrimRange& rootRange, const UsdPrim& usdRootPrim)
{
    // We want both pre- and post- visit iterations over the prims in this
    // method. To do so, iterate over all the root prims of the input range,
    // and create new PrimRanges to iterate over their subtrees.
    for (auto rootIt = rootRange.begin(); rootIt != rootRange.end(); ++rootIt) {
        const UsdPrim& rootPrim = *rootIt;
        rootIt.PruneChildren();

        std::unordered_map<SdfPath, UsdMayaPrimReaderSharedPtr,
                SdfPath::Hash> primReaders;
        const UsdPrimRange range = UsdPrimRange::PreAndPostVisit(rootPrim);
        for (auto primIt = range.begin(); primIt != range.end(); ++primIt) {
            const UsdPrim& prim = *primIt;

            // The iterator will hit each prim twice. IsPostVisit tells us if
            // this is the pre-visit (Read) step or post-visit (PostReadSubtree)
            // step.
            if (!primIt.IsPostVisit()) {
                // This is the normal Read step (pre-visit).
                UsdMayaPrimReaderArgs args(prim, mArgs);
                UsdMayaPrimReaderContext readCtx(&mNewNodeRegistry);

                if (OverridePrimReader(usdRootPrim, prim, args, readCtx, primIt)) {
                    continue;
                }

                TfToken typeName = prim.GetTypeName();
                if (UsdMayaPrimReaderRegistry::ReaderFactoryFn factoryFn
                        = UsdMayaPrimReaderRegistry::FindOrFallback(typeName)) {
                    UsdMayaPrimReaderSharedPtr primReader = factoryFn(args);
                    if (primReader) {
                        primReader->Read(&readCtx);
                        if (primReader->HasPostReadSubtree()) {
                            primReaders[prim.GetPath()] = primReader;
                        }
                        if (readCtx.GetPruneChildren()) {
                            primIt.PruneChildren();
                        }
                    }
                }
            }
            else {
                // This is the PostReadSubtree step, if the PrimReader has
                // specified one.
                UsdMayaPrimReaderContext postReadCtx(&mNewNodeRegistry);
                auto primReaderIt = primReaders.find(prim.GetPath());
                if (primReaderIt != primReaders.end()) {
                    primReaderIt->second->PostReadSubtree(&postReadCtx);
                }
            }
        }
    }

    return true;
}

void UsdMaya_ReadJob::PreImport(Usd_PrimFlagsPredicate& returnPredicate)
{}

bool UsdMaya_ReadJob::SkipRootPrim(bool isImportingPseudoRoot)
{
    return isImportingPseudoRoot;
}

bool
UsdMaya_ReadJob::Redo()
{
    // Undo the undo
    MStatus status = mDagModifierUndo.undoIt();

    return (status == MS::kSuccess);
}

bool
UsdMaya_ReadJob::Undo()
{
    if (!mDagModifierSeeded) {
        mDagModifierSeeded = true;
        MStatus dagStatus;
        // Construct list of top level DAG nodes to delete and any DG nodes
        for (auto& it : mNewNodeRegistry) {
            if (it.second != mMayaRootDagPath.node() ) { // if not the parent root node
                MFnDagNode dagFn(it.second, &dagStatus);
                if (dagStatus == MS::kSuccess) {
                    if (mMayaRootDagPath.node() != MObject::kNullObj) {
                        if (!dagFn.hasParent(mMayaRootDagPath.node() ))  { // skip if a DAG Node, but not under the root
                            continue;
                        }
                    }
                    else {
                        if (dagFn.parentCount() == 0)  { // under scene root
                            continue;
                        }
                    }
                }
                mDagModifierUndo.deleteNode(it.second);
            }
        }
    }

    MStatus status = mDagModifierUndo.doIt();

    return (status == MS::kSuccess);
}

void
UsdMaya_ReadJob::SetMayaRootDagPath(const MDagPath &mayaRootDagPath)
{
    mMayaRootDagPath = mayaRootDagPath;
}

const MDagPath&
UsdMaya_ReadJob::GetMayaRootDagPath() const
{
    return mMayaRootDagPath;
}

PXR_NAMESPACE_CLOSE_SCOPE
