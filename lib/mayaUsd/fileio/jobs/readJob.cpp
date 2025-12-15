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

#include <mayaUsd/fileio/chaser/importChaserRegistry.h>
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/translators/translatorMaterial.h>
#include <mayaUsd/fileio/translators/translatorXformable.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/nodes/stageNode.h>
#include <mayaUsd/undo/OpUndoItemMuting.h>
#include <mayaUsd/utils/dynamicAttribute.h>
#include <mayaUsd/utils/progressBarScope.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stageCacheContext.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usd/variantSets.h>
#if PXR_VERSION < 2508
#include <pxr/usd/usd/zipFile.h>
#else
#include <pxr/usd/sdf/zipFile.h>
#endif
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdUtils/pipeline.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include <maya/MAnimControl.h>
#include <maya/MDGModifier.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPathArray.h>
#include <maya/MDistance.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>

#include <cctype>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
// Simple RAII class to ensure tracking does not extend past the scope.
struct TempNodeTrackerScope
{
    TempNodeTrackerScope(UsdMayaPrimReaderContext& context)
        : _context(context)
    {
        _context.StartNewMayaNodeTracking();
    }

    ~TempNodeTrackerScope() { _context.StopNewMayaNodeTracking(); }

    UsdMayaPrimReaderContext& _context;
};

struct AutoTimelineRestore
{
    AutoTimelineRestore(bool preserve)
        : preserveTimeline(preserve)
        , originalAnimStartTime(MAnimControl::animationStartTime())
        , originalAnimEndTime(MAnimControl::animationEndTime())
        , originalMinTime(MAnimControl::minTime())
        , originalMaxTime(MAnimControl::maxTime())

    {
    }

    ~AutoTimelineRestore()
    {
        if (!preserveTimeline)
            return;

        try {
            if (MAnimControl::minTime() != originalMinTime) {
                MAnimControl::setMinTime(originalMinTime);
            }

            if (MAnimControl::maxTime() != originalMaxTime) {
                MAnimControl::setMaxTime(originalMaxTime);
            }

            if (MAnimControl::animationStartTime() != originalAnimStartTime) {
                MAnimControl::setAnimationStartTime(originalAnimStartTime);
            }

            if (MAnimControl::animationEndTime() != originalAnimEndTime) {
                MAnimControl::setAnimationEndTime(originalAnimEndTime);
            }

        } catch (std::exception&) {
            // Ignore - don't trhow exceptions from the destructor.
        }
    }

    const bool  preserveTimeline; // If false, the timeline values are not preserved.
    const MTime originalAnimStartTime;
    const MTime originalAnimEndTime;
    const MTime originalMinTime;
    const MTime originalMaxTime;
};

bool IsValidVariant(const UsdVariantSet& varSet)
{
    if (!varSet.IsValid())
        return false;

    if (varSet.GetVariantNames().size() == 0)
        return false;

    return true;
}

bool IsValidVariantSelection(const UsdVariantSet& varSet, const std::string& selection)
{
    const std::vector<std::string> names = varSet.GetVariantNames();
    if (std::find(names.begin(), names.end(), selection) == names.end())
        return false;

    return true;
}

void ApplyVariantSelections(const UsdPrim& prim, const SdfVariantSelectionMap& selections)
{
    for (auto& variant : selections) {
        const std::string& varSetName = variant.first;
        const std::string& varSelection = variant.second;
        UsdVariantSet      varSet = prim.GetVariantSet(varSetName);

        if (!IsValidVariant(varSet)) {
            TF_WARN(
                "Invalid variant (%s) for prim (%s).",
                varSetName.c_str(),
                prim.GetName().GetText());
            continue;
        }

        if (!IsValidVariantSelection(varSet, varSelection)) {
            TF_WARN(
                "Invalid variant selection (%s) in variant (%s) for prim (%s).",
                varSelection.c_str(),
                varSetName.c_str(),
                prim.GetName().GetText());
            continue;
        }

        varSet.SetVariantSelection(varSelection);
    }
}

} // namespace

UsdMaya_ReadJob::UsdMaya_ReadJob(
    const MayaUsd::ImportData&  iImportData,
    const UsdMayaJobImportArgs& iArgs)
    : mArgs(iArgs)
    , mImportData(iImportData)
    , mMayaRootDagPath()
    , mDagModifierUndo()
    , mDagModifierSeeded(false)
{
}

UsdMaya_ReadJob::~UsdMaya_ReadJob() { }

bool UsdMaya_ReadJob::Read(std::vector<MDagPath>* addedDagPaths)
{
    // When we are called from PrimUpdaterManager we should already have
    // a computation scope. If we are called from elsewhere don't show any
    // progress bar here.
    MayaUsd::ProgressBarScope progressBar(16);

    // Do not use the global undo info recording system.
    // The read job Undo() / redo() functions will handle all operations.
    MayaUsd::OpUndoItemMuting undoMuting;

    MStatus status;

    if (!TF_VERIFY(!mImportData.empty())) {
        return false;
    }

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(mImportData.filename());
    if (!rootLayer) {
        return false;
    }
    progressBar.advance();

    // Construct the variant opinion for the session layer.
    SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous();
    {
        TfToken modelName = UsdUtilsGetModelNameFromRootLayer(rootLayer);
        if (modelName != TfToken()) {
            SdfVariantSelectionMap varSelsMap = mImportData.rootVariantSelections();
            if (!modelName.IsEmpty()) {
                SdfPrimSpecHandle over
                    = SdfPrimSpec::New(sessionLayer, modelName, SdfSpecifierOver);
                for (auto varSel : varSelsMap) {
                    over->GetVariantSelections()[varSel.first] = varSel.second;
                }
            }
        }
    }

    progressBar.advance();

    // Layer and Stage used to Read in the USD file
    UsdStageRefPtr stage;
    if (mImportData.hasPopulationMask()) {
        // OpenMasked doesn't use the UsdStageCache, so don't create a UsdStageCacheContext
        stage = UsdStage::OpenMasked(
            rootLayer,
            sessionLayer,
            mImportData.stagePopulationMask(),
            mImportData.stageInitialLoadSet());
    } else {
        UsdStageCache        stageCache;
        UsdStageCacheContext stageCacheContext(stageCache);
        if (mArgs.pullImportStage)
            stage = mArgs.pullImportStage;
        else
            stage = UsdStage::Open(rootLayer, sessionLayer, mImportData.stageInitialLoadSet());
    }
    if (!stage) {
        return false;
    }
    progressBar.advance();

    UsdEditContext editContext(stage, stage->GetSessionLayer());
    stage->SetEditTarget(stage->GetSessionLayer());
    _setTimeSampleMultiplierFrom(stage->GetTimeCodesPerSecond());
    progressBar.advance();

    // If the import time interval isn't empty, we expand the Min/Max time
    // sliders to include the stage's range if necessary.
    AutoTimelineRestore timelineRestore(mArgs.preserveTimeline);
    if (!mArgs.timeInterval.IsEmpty()) {
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

        MTime::Unit timeUnit = MTime::uiUnit();
        if (stageInterval.GetMin() < timelineRestore.originalMinTime.value()) {
            MAnimControl::setMinTime(
                MTime(stageInterval.GetMin() * mTimeSampleMultiplier, timeUnit));
        }
        if (stageInterval.GetMax() > timelineRestore.originalMaxTime.value()) {
            MAnimControl::setMaxTime(
                MTime(stageInterval.GetMax() * mTimeSampleMultiplier, timeUnit));
        }
    }
    progressBar.advance();

    // Use the primPath to get the root usdNode
    std::string primPath = mImportData.rootPrimPath();
    UsdPrim     usdRootPrim
        = primPath.empty() ? stage->GetDefaultPrim() : stage->GetPrimAtPath(SdfPath(primPath));
    if (!usdRootPrim && !(primPath.empty() || primPath == "/")) {
        TF_RUNTIME_ERROR(
            "Unable to set root prim to <%s> when reading USD file '%s'; "
            "using the pseudo-root </> instead",
            primPath.c_str(),
            mImportData.filename().c_str());
        usdRootPrim = stage->GetPseudoRoot();
    }
    progressBar.advance();

    bool isImportingPseudoRoot = (usdRootPrim == stage->GetPseudoRoot());

    if (!usdRootPrim) {
        TF_RUNTIME_ERROR("No default prim found in USD file '%s'", mImportData.filename().c_str());
        return false;
    }

    // Set the variants on the usdRootPrim
    ApplyVariantSelections(usdRootPrim, mImportData.rootVariantSelections());
    progressBar.advance();

    // Set the variants on all the import data prims.
    for (auto& varPrim : mImportData.primVariantSelections()) {
        const SdfPath& primName = varPrim.first;
        UsdPrim        usdVarPrim = stage->GetPrimAtPath(primName);
        if (!usdVarPrim.IsValid()) {
            TF_WARN("Invalid prim specified (%s) for variant selection.", primName.GetText());
            continue;
        }
        ApplyVariantSelections(usdVarPrim, varPrim.second);
    }
    progressBar.advance();

    Usd_PrimFlagsPredicate predicate = UsdPrimDefaultPredicate;

    PreImport(predicate);
    progressBar.advance();

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

    mNewNodeRegistry.insert(
        std::make_pair(rootPathToRegister.GetString(), mMayaRootDagPath.node()));
    progressBar.advance();

    if (mArgs.useAsAnimationCache) {
        MDGModifier dgMod;
        MObject     usdStageNode = dgMod.createNode(UsdMayaStageNode::typeId, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        // We only ever create a single stage node per usdImport, so we can
        // simply register it and later look it up in the registry using its
        // type name.
        mNewNodeRegistry.insert(
            std::make_pair(UsdMayaStageNodeTokens->MayaTypeName.GetString(), usdStageNode));

        MFnDependencyNode depNodeFn(usdStageNode, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug filePathPlug = depNodeFn.findPlug(UsdMayaStageNode::filePathAttr, true, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.newPlugValueString(filePathPlug, mImportData.filename().c_str());
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.doIt();
        CHECK_MSTATUS_AND_RETURN(status, false);
    }
    progressBar.advance();

    // check if "USDZ Texture Import" option is checked and the archive in question is a USDZ.
    if (mArgs.importUSDZTextures && stage->GetRootLayer()->GetFileFormat()->IsPackage()) {
        if (mArgs.importUSDZTexturesFilePath.length() == 0) {
            MString currentMayaWorkspacePath = UsdMayaUtil::GetCurrentMayaWorkspacePath();
            TF_WARN(
                "Because -importUSDZTexturesFilePath was not explicitly specified, textures "
                "will be imported to the workspace folder: %s.",
                currentMayaWorkspacePath.asChar());
        }
    }

    DoImport(range, usdRootPrim);
    progressBar.advance();

    // NOTE: (yliangsiew) Storage to later pass on to `PostImport` for import chasers.
    MDagPathArray currentAddedDagPaths;
    SdfPathVector fromSdfPaths;

    SdfPathSet topImportedPaths;
    if (isImportingPseudoRoot) {
        // get all the dag paths for the root prims
        TF_FOR_ALL(childIter, stage->GetPseudoRoot().GetChildren())
        {
            topImportedPaths.insert(childIter->GetPath());
        }
    } else {
        topImportedPaths.insert(usdRootPrim.GetPath());
    }

    MSdfToDagMap sdfToDagMap;
    for (const UsdPrim prim : stage->TraverseAll()) {
        SdfPath     primSdfPath = prim.GetPath();
        std::string key = primSdfPath.GetString();
        MObject     obj;
        if (TfMapLookup(mNewNodeRegistry, key, &obj)) {
            if (obj.hasFn(MFn::kDagNode)) {
                sdfToDagMap[primSdfPath] = MDagPath::getAPathTo(obj);
            }
        }
    }

    TF_FOR_ALL(pathsIter, topImportedPaths)
    {
        std::string key = pathsIter->GetString();
        MObject     obj;
        if (TfMapLookup(mNewNodeRegistry, key, &obj)) {
            if (obj.hasFn(MFn::kDagNode)) {
                addedDagPaths->push_back(MDagPath::getAPathTo(obj));
                currentAddedDagPaths.append(MDagPath::getAPathTo(obj));
                fromSdfPaths.push_back(pathsIter->GetPrimPath());
            }
        }
    }

    progressBar.advance();

    // NOTE: (yliangsiew) Look into a registry of post-import "chasers" here
    // and call `PostImport` on each of them.
    this->mImportChasers.clear();

    UsdMayaImportChaserRegistry::FactoryContext ctx(
        predicate, stage, currentAddedDagPaths, fromSdfPaths, this->mArgs);
    for (const std::string& importChaserName : this->mArgs.chaserNames) {
        if (UsdMayaImportChaserRefPtr fn
            = UsdMayaImportChaserRegistry::GetInstance().Create(importChaserName.c_str(), ctx)) {
            this->mImportChasers.emplace_back(fn);
        } else {
            TF_RUNTIME_ERROR("Failed to create import chaser: %s", importChaserName.c_str());
        }
    }
    progressBar.advance();

    for (const UsdMayaImportChaserRefPtr& chaser : this->mImportChasers) {
        chaser->SetSdfToDagMap(sdfToDagMap);
        bool bStat
            = chaser->PostImport(predicate, stage, currentAddedDagPaths, fromSdfPaths, this->mArgs);
        if (!bStat) {
            TF_WARN("Failed to execute import chaser!");
            return false;
        }
    }
    progressBar.advance();

    _ConvertUpAxisAndUnits(stage);
    progressBar.advance();

    UsdMayaReadUtil::mapFileHashes.clear();

    return (status == MS::kSuccess);
}

static bool getMayaUpAxisZ() { return MGlobal::isZAxisUp(); }

static bool getUSDUpAxisZ(const UsdStageRefPtr& stage)
{
    return UsdGeomGetStageUpAxis(stage) == UsdGeomTokens->z;
}

void UsdMaya_ReadJob::_ConvertUpAxisAndUnits(const UsdStageRefPtr& stage)
{
    ConversionInfo conversion;

    // Convert up-axis based if required and different between Maya and USD.
    const bool convertUpAxis = mArgs.upAxis;
    conversion.isMayaUpAxisZ = getMayaUpAxisZ();
    conversion.isUSDUpAxisUZ = getUSDUpAxisZ(stage);
    conversion.needUpAxisConversion
        = (convertUpAxis && (conversion.isMayaUpAxisZ != conversion.isUSDUpAxisUZ));

    // Convert units if required and different between Maya and USD.
    const bool convertUnits = mArgs.unit;
    // Note: when changing preference, we need to compare to the UI units.
    //       When adding scaling transforms, we need to compare to internal units.
    const MDistance::Unit mayaUnits
        = (mArgs.axisAndUnitMethod == UsdMayaJobImportArgsTokens->overwritePrefs)
        ? MDistance::uiUnit()
        : MDistance::internalUnit();
    conversion.mayaMetersPerUnit = UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(mayaUnits);
    conversion.usdMetersPerUnit = UsdGeomGetStageMetersPerUnit(stage);
    conversion.needUnitsConversion
        = (convertUnits && (conversion.mayaMetersPerUnit != conversion.usdMetersPerUnit));

    // If neither up-axis nor units need to change, do nothing.
    if (!conversion.needUpAxisConversion && !conversion.needUnitsConversion)
        return;

    if (mArgs.axisAndUnitMethod == UsdMayaJobImportArgsTokens->rotateScale)
        _ConvertUpAxisAndUnitsByModifyingData(stage, conversion, false);
    else if (mArgs.axisAndUnitMethod == UsdMayaJobImportArgsTokens->addTransform)
        _ConvertUpAxisAndUnitsByModifyingData(stage, conversion, true);
    else if (mArgs.axisAndUnitMethod == UsdMayaJobImportArgsTokens->overwritePrefs)
        _ConvertUpAxisAndUnitsByChangingMayaPrefs(stage, conversion);
    else
        TF_WARN(
            "Unknown method of converting the USD up axis and units to Maya: %s",
            mArgs.axisAndUnitMethod.c_str());
}

// Construct list of top level DAG nodes.
static std::vector<MDagPath> _findAllRootDagNodePaths(
    const UsdMayaPrimReaderContext::ObjectRegistry& newNodes,
    const MDagPath&                                 rootPath)
{
    std::vector<MDagPath> rootNodePaths;

    for (auto& it : newNodes) {
        // Do not process the root DAG path, which was not part of the import.
        if (it.second == rootPath.node())
            continue;

        // Note: if it is not a DAG node, then it is a DG node and we don't
        //       need to process it.
        MStatus    dagStatus;
        MFnDagNode dagFn(it.second, &dagStatus);
        if (dagStatus != MS::kSuccess)
            continue;

        MDagPathArray paths;
        dagFn.getAllPaths(paths);
        for (const MDagPath& path : paths) {
            if (path.length() == 1) {
                rootNodePaths.emplace_back(path);
            }
        }
    }

    return rootNodePaths;
}

static std::vector<std::string> _convertDagPathToNames(const std::vector<MDagPath>& dagNodePaths)
{
    std::vector<std::string> dagNodeNames;

    dagNodeNames.reserve(dagNodePaths.size());
    for (const MDagPath& path : dagNodePaths)
        dagNodeNames.emplace_back(path.fullPathName().asChar());

    return dagNodeNames;
}

static std::string _cleanMayaNodeName(const std::string& name)
{
    std::string cleaned(name);
    for (char& c : cleaned) {
        if (!std::isalpha(c) && !std::isdigit(c))
            c = '_';
    }
    return cleaned;
}

static void _addOrignalUpAxisAttribute(const std::vector<MDagPath>& dagNodePaths, bool isUSDUpAxisZ)
{
    const MString originalUpAxis = isUSDUpAxisZ ? "Z" : "Y";
    const MString attrName = "OriginalUpAxis";
    for (const MDagPath& dagPath : dagNodePaths) {
        MFnDependencyNode depNode(dagPath.node());
        using namespace MayaUsd;
        const auto flags = DynamicAttrFlags::kDefaults & ~DynamicAttrFlags::kHidden;
        MayaUsd::setDynamicAttribute(depNode, attrName, originalUpAxis, flags);
    }
}

static void
_addOrignalUnitsAttribute(const std::vector<MDagPath>& dagNodePaths, double usdMetersPerUnit)
{
    MString originalUnits;
    originalUnits.set(usdMetersPerUnit);
    const MString attrName = "OriginalMetersPerUnit";
    for (const MDagPath& dagPath : dagNodePaths) {
        MFnDependencyNode depNode(dagPath.node());
        using namespace MayaUsd;
        const auto flags = DynamicAttrFlags::kDefaults & ~DynamicAttrFlags::kHidden;
        MayaUsd::setDynamicAttribute(depNode, attrName, originalUnits, flags);
    }
}

void UsdMaya_ReadJob::_ConvertUpAxisAndUnitsByModifyingData(
    const UsdStageRefPtr& stage,
    const ConversionInfo& conversion,
    bool                  keepParentGroup)
{
    std::vector<MDagPath> dagNodePaths
        = _findAllRootDagNodePaths(mNewNodeRegistry, mMayaRootDagPath);
    if (dagNodePaths.size() <= 0)
        return;

    std::vector<std::string> dagNodeNames = _convertDagPathToNames(dagNodePaths);

    std::string fullScript;

    // The rules for the group name are:
    //
    //    - If there is a single root object, use that object name + _converted
    //    - Otherwise use the import file name + _converted
    std::string groupName;
    {
        if (dagNodeNames.size() == 1) {
            groupName = dagNodeNames[0];
            if (TfStringContains(groupName, "|")) {
                groupName = TfStringGetSuffix(groupName, '|');
            }
        } else {
            groupName = TfGetBaseName(mImportData.filename());
        }

        static const char groupNameSuffix[] = "_converted";
        groupName = _cleanMayaNodeName(groupName);
        groupName = groupName + groupNameSuffix;
    }

    // Group all root node under a new group:
    //
    //    - Use -name to force the desired group name
    //    - Use -absolute to keep the grouped node world positions
    //    - Use -parent if the import was done under a given Maya node
    //    - Use -world to create the group under the root ofthe scene
    //      if the import was done at the root of the scene
    //    - Capture the new group name in a MEL variable called $groupName

    {
        std::string groupCmd;

        // Note: for the group commands, the node names must be separated  by, literally, " ",
        //       so that they form a space-separated serie of quoted names.
        static const char nodeNameSeparator[] = "\" \"";
        std::string       joinedDAGNodeNames = TfStringJoin(dagNodeNames, nodeNameSeparator);

        if (mMayaRootDagPath.node() != MObject::kNullObj) {
            static const char groupUnderParentCmdFormat[]
                = "string $groupName = `group -name \"%s\" -absolute -parent \"%s\" \"%s\"`;\n";
            std::string rootName = mMayaRootDagPath.fullPathName().asChar();
            groupCmd = TfStringPrintf(
                groupUnderParentCmdFormat,
                groupName.c_str(),
                joinedDAGNodeNames.c_str(),
                rootName.c_str());
        } else {
            static const char groupUnderWorldCmdFormat[]
                = "string $groupName = `group -name \"%s\" -absolute -world \"%s\"`;\n";
            groupCmd = TfStringPrintf(
                groupUnderWorldCmdFormat, groupName.c_str(), joinedDAGNodeNames.c_str());
        }

        fullScript += groupCmd;
    }

    // Rotate the group to align with the desired axis.
    //
    //    - Use relative rotation since we want to rotate the group as it is already positioned
    //    - Use -euler to make teh angle be relative to the current angle
    //    - Use forceOrderXYZ to force  the rotation to be relative to world
    //    - Use -pivot to make sure we are rotating relative to the origin
    //      (The group is positioned at the center of all sub-object, so we need to specify the
    //      pivot)
    if (conversion.needUpAxisConversion) {
        static const char rotationCmdFormat[]
            = "rotate -relative -euler -pivot 0 0 0 -forceOrderXYZ %d 0 0 $groupName;\n";
        const int   angleYtoZ = 90;
        const int   angleZtoY = -90;
        std::string rotationCmd
            = TfStringPrintf(rotationCmdFormat, conversion.isMayaUpAxisZ ? angleYtoZ : angleZtoY);
        fullScript += rotationCmd;
    }

    if (conversion.needUnitsConversion) {
        static const char scalingCmdFormat[]
            = "scale -relative -pivot 0 0 0 -scaleXYZ %f %f %f $groupName;\n";
        const double usdToMayaScaling = conversion.usdMetersPerUnit / conversion.mayaMetersPerUnit;
        std::string  scalingCmd = TfStringPrintf(
            scalingCmdFormat, usdToMayaScaling, usdToMayaScaling, usdToMayaScaling);
        fullScript += scalingCmd;
    }

    if (!keepParentGroup) {
        static const char ungroupCmdFormat[] = "ungroup -absolute \"%s\";\n";
        std::string       ungroupCmd = TfStringPrintf(ungroupCmdFormat, groupName.c_str());
        fullScript += ungroupCmd;
    }

    if (!MGlobal::executeCommand(fullScript.c_str())) {
        MGlobal::displayWarning("Failed to add a transform to convert the up-axis to align "
                                "the USD data with Maya up-axis.");
        return;
    }

    if (keepParentGroup) {
        MDagPath groupDagPath;
        {
            MSelectionList sel;
            sel.add(groupName.c_str());
            sel.getDagPath(0, groupDagPath);
        }
        if (conversion.needUpAxisConversion)
            _addOrignalUpAxisAttribute({ groupDagPath }, conversion.isUSDUpAxisUZ);
        if (conversion.needUnitsConversion)
            _addOrignalUnitsAttribute({ groupDagPath }, conversion.usdMetersPerUnit);
    } else {
        if (conversion.needUpAxisConversion)
            _addOrignalUpAxisAttribute(dagNodePaths, conversion.isUSDUpAxisUZ);
        if (conversion.needUnitsConversion)
            _addOrignalUnitsAttribute(dagNodePaths, conversion.usdMetersPerUnit);
    }

    MGlobal::displayInfo(
        "Mismatching axis and units have been converted for accurate orientation and scale.");
}

void UsdMaya_ReadJob::_ConvertUpAxisAndUnitsByChangingMayaPrefs(
    const UsdStageRefPtr& stage,
    const ConversionInfo& conversion)
{
    bool success = true;

    // Close preference window if needed.
    {
        static const char* closePrefsCmd
            = "global string $gPreferenceWindow;\n"
              "if (`window -query -exists $gPreferenceWindow`) {\n"
              "    if (`window -query -visible $gPreferenceWindow`) {\n"
              "         string $title = getMayaUsdLibString(\"kAboutToChangePrefsTitle\");\n"
              "         string $body = getMayaUsdLibString(\"kAboutToChangePrefs\");\n"
              "         string $ok = getMayaUsdLibString(\"kSavePrefsChange\");\n"
              "         string $cancel = getMayaUsdLibString(\"kDiscardPrefsChange\");\n"
              "         string $result = `confirmDialog -title $title -message $body\n"
              "                          -button $ok -button $cancel\n"
              "                          -defaultButton $ok\n"
              "                          -cancelButton $cancel`;\n"
              "         if ($result == $ok) {\n"
              "             savePrefsChanges;\n"
              "         }\n"
              "         else {\n"
              "             cancelPrefsChanges;\n"
              "         }\n"
              "    }\n"
              "}\n";

        if (!MGlobal::executeCommand(closePrefsCmd)) {
            MGlobal::displayWarning("Failed to close the Maya preferences windows.");
        }
    }

    // Set up-axis preferences if needed.
    if (conversion.needUpAxisConversion) {
        const bool    rotateView = true;
        const MStatus status = conversion.isUSDUpAxisUZ ? MGlobal::setZAxisUp(rotateView)
                                                        : MGlobal::setYAxisUp(rotateView);
        if (!status) {
            MGlobal::displayWarning(
                "Failed to change the Maya up-axis preferences to match USD data up-axis.");
            success = false;
        }
    }

    // Set units preferences if needed.
    if (conversion.needUnitsConversion) {
        const MDistance::Unit mayaUnit
            = UsdMayaUtil::ConvertUsdGeomLinearUnitToMDistanceUnit(conversion.usdMetersPerUnit);
        if (mayaUnit == MDistance::kInvalid) {
            MGlobal::displayWarning(
                "Unable to convert <unit> to a Maya unit. Supported units include millimeters, "
                "centimeters, meters, kilometers, inches, feet, yards and miles.");
            success = false;
        } else {
            const MString mayaUnitText = UsdMayaUtil::ConvertMDistanceUnitToText(mayaUnit);
            MString       changeUnitsCmd;
            changeUnitsCmd.format("currentUnit -linear ^1s;", mayaUnitText);

            // Note: we *must* execute the units change on-idle because the import process
            //       saves and restores all units! If we change it now, the change would be
            //       lost.
            if (!MGlobal::executeCommandOnIdle(changeUnitsCmd)) {
                MGlobal::displayWarning(
                    "Failed to change the Maya units preferences to match USD data "
                    "because the units are not supported by Maya.");
                success = false;
            }
        }
    }

    if (success)
        MGlobal::displayInfo(
            "Changed Maya preferences to match up-axis and units from the imported USD scene.");
}

bool UsdMaya_ReadJob::DoImport(UsdPrimRange& rootRange, const UsdPrim& usdRootPrim)
{
    return _DoImport(rootRange, usdRootPrim);
}

bool UsdMaya_ReadJob::OverridePrimReader(
    const UsdPrim&               usdRootPrim,
    const UsdPrim&               prim,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext&    readCtx,
    UsdPrimRange::iterator&      primIt)
{
    return false;
}

void UsdMaya_ReadJob::_DoImportPrimIt(
    UsdPrimRange::iterator&   primIt,
    const UsdPrim&            usdRootPrim,
    UsdMayaPrimReaderContext& readCtx,
    _PrimReaderMap&           primReaderMap)
{
    const UsdPrim& prim = *primIt;
    // The iterator will hit each prim twice. IsPostVisit tells us if
    // this is the pre-visit (Read) step or post-visit (PostReadSubtree)
    // step.
    if (primIt.IsPostVisit()) {
        // This is the PostReadSubtree step, if the PrimReader has
        // specified one.
        auto primReaderIt = primReaderMap.find(prim.GetPath());
        if (primReaderIt != primReaderMap.end()) {
            primReaderIt->second->PostReadSubtree(readCtx);
        }
    } else {
        // This is the normal Read step (pre-visit).
        UsdMayaPrimReaderArgs args(prim, mArgs);
        if (OverridePrimReader(usdRootPrim, prim, args, readCtx, primIt)) {
            return;
        }

        TfToken typeName = prim.GetTypeName();
        if (UsdMayaPrimReaderRegistry::ReaderFactoryFn factoryFn
            = UsdMayaPrimReaderRegistry::FindOrFallback(typeName, mArgs, prim)) {
            UsdMayaPrimReaderSharedPtr primReader = factoryFn(args);
            if (primReader) {
                TempNodeTrackerScope scope(readCtx);
                primReader->Read(readCtx);
                if (primReader->HasPostReadSubtree()) {
                    primReaderMap[prim.GetPath()] = primReader;
                }
                if (readCtx.GetPruneChildren()) {
                    primIt.PruneChildren();
                }
                UsdMayaReadUtil::ReadAPISchemaAttributesFromPrim(args, readCtx);
            }
        }
    }
}

void UsdMaya_ReadJob::_DoImportInstanceIt(
    UsdPrimRange::iterator&   primIt,
    const UsdPrim&            usdRootPrim,
    UsdMayaPrimReaderContext& readCtx,
    _PrimReaderMap&           primReaderMap)
{
    const UsdPrim& prim = *primIt;
    if (!primIt.IsPostVisit()) {
        return;
    }
    const UsdPrim prototype = prim.GetPrototype();
    if (!prototype) {
        return;
    }

    const SdfPath prototypePath = prototype.GetPath();
    MObject       prototypeObject = readCtx.GetMayaNode(prototypePath, false);
    if (prototypeObject == MObject::kNullObj) {
        // Note: we are not passing the primReaderMap to _ImportPrototype because
        //       _ImportPrototype does its own prim range loop and immediate post-visit
        //       on the prims of the prototype.
        _ImportPrototype(prototype, usdRootPrim, readCtx);
        prototypeObject = readCtx.GetMayaNode(prototypePath, false);
        if (prototypeObject == MObject::kNullObj) {
            return;
        }
    }
    MStatus    status;
    MFnDagNode prototypeNode(prototypeObject, &status);
    if (!status) {
        return;
    }
    const auto primPath = prim.GetPath();
    MObject    parentObject = readCtx.GetMayaNode(primPath.GetParentPath(), false);
    MFnDagNode duplicateNode;
    MObject    duplicateObject
        = duplicateNode.create("transform", primPath.GetName().c_str(), parentObject, &status);
    if (!status) {
        return;
    }

    const unsigned int childCount = prototypeNode.childCount();
    for (unsigned int child = 0; child < childCount; ++child) {
        MObject childObject = prototypeNode.child(child);
        duplicateNode.addChild(childObject, MFnDagNode::kNextPos, true);
    }

    // Add duplicate node to registry.
    readCtx.RegisterNewMayaNode(primPath.GetString(), duplicateObject);

    // Read xformable attributes from the
    // UsdPrim on to the transform node.
    UsdGeomXformable      xformable(prim);
    UsdMayaPrimReaderArgs readerArgs(prim, mArgs);
    UsdMayaTranslatorXformable::Read(xformable, duplicateObject, readerArgs, &readCtx);
}

void UsdMaya_ReadJob::_ImportPrototype(
    const UsdPrim&            prototype,
    const UsdPrim&            usdRootPrim,
    UsdMayaPrimReaderContext& readCtx)
{
    const UsdPrimRange range = UsdPrimRange::PreAndPostVisit(prototype);
    _ImportPrimRange(range, usdRootPrim);
}

bool UsdMaya_ReadJob::_DoImport(const UsdPrimRange& rootRange, const UsdPrim& usdRootPrim)
{
    // We want both pre- and post- visit iterations over the prims in this
    // method. To do so, iterate over all the root prims of the input range,
    // and create new PrimRanges to iterate over their subtrees.
    for (auto rootIt = rootRange.begin(); rootIt != rootRange.end(); ++rootIt) {
        const UsdPrim& rootPrim = *rootIt;
        rootIt.PruneChildren();

        const UsdPrimRange range = mArgs.importInstances
            ? UsdPrimRange::PreAndPostVisit(rootPrim)
            : UsdPrimRange::PreAndPostVisit(
                rootPrim, UsdTraverseInstanceProxies(UsdPrimAllPrimsPredicate));
        _ImportPrimRange(range, usdRootPrim);
    }

    return _CleanupPrototypes(usdRootPrim);
}

void UsdMaya_ReadJob::_ImportPrimRange(const UsdPrimRange& range, const UsdPrim& usdRootPrim)
{
    const int                     loopSize = std::distance(range.begin(), range.end());
    MayaUsd::ProgressBarLoopScope instanceLoop(loopSize);

    _PrimReaderMap primReaderMap;

    for (auto primIt = range.begin(); primIt != range.end(); ++primIt) {
        const UsdPrim&           prim = *primIt;
        UsdMayaPrimReaderContext readCtx(&mNewNodeRegistry);
        readCtx.SetTimeSampleMultiplier(mTimeSampleMultiplier);

        if (mArgs.importInstances && prim.IsInstance()) {
            _DoImportInstanceIt(primIt, usdRootPrim, readCtx, primReaderMap);
        } else {
            _DoImportPrimIt(primIt, usdRootPrim, readCtx, primReaderMap);
        }
        instanceLoop.loopAdvance();
    }
}

bool UsdMaya_ReadJob::_CleanupPrototypes(const UsdPrim& usdRootPrim)
{
    if (!mArgs.importInstances)
        return true;

    MayaUsd::ProgressBarScope progressBar(1);

    MDGModifier              deletePrototypeMod;
    UsdMayaPrimReaderContext readCtx(&mNewNodeRegistry);
    readCtx.SetTimeSampleMultiplier(mTimeSampleMultiplier);

    auto                          prototypes = usdRootPrim.GetStage()->GetPrototypes();
    const int                     loopSize = prototypes.size();
    MayaUsd::ProgressBarLoopScope prototypesLoop(loopSize);
    for (const auto& prototype : prototypes) {

        const SdfPath prototypePath = prototype.GetPath();
        MObject       prototypeObject = readCtx.GetMayaNode(prototypePath, false);
        if (prototypeObject != MObject::kNullObj) {
            MStatus    status;
            MFnDagNode prototypeNode(prototypeObject, &status);
            if (status) {
                while (prototypeNode.childCount()) {
                    prototypeNode.removeChildAt(prototypeNode.childCount() - 1);
                }
            }
            deletePrototypeMod.deleteNode(prototypeObject, false);
            mNewNodeRegistry.erase(prototypePath.GetString());
        }
        prototypesLoop.loopAdvance();
    }
    deletePrototypeMod.doIt();
    progressBar.advance();

    return true;
}

void UsdMaya_ReadJob::PreImport(Usd_PrimFlagsPredicate& returnPredicate) { }

bool UsdMaya_ReadJob::SkipRootPrim(bool isImportingPseudoRoot) { return isImportingPseudoRoot; }

bool UsdMaya_ReadJob::Redo()
{
    // Do not use the global undo info recording system.
    // The read job Undo() / redo() functions will handle all operations.
    MayaUsd::OpUndoItemMuting undoMuting;

    // Undo the undo
    MStatus status = mDagModifierUndo.undoIt();

    // NOTE: (yliangsiew) All chasers need to have their Redo run as well.
    for (const UsdMayaImportChaserRefPtr& chaser : this->mImportChasers) {
        bool bStat = chaser->Redo();
        if (!bStat) {
            TF_WARN("Failed to execute import chaser's Redo()!");
            return false;
        }
    }

    return (status == MS::kSuccess);
}

bool UsdMaya_ReadJob::Undo()
{
    // Do not use the global undo info recording system.
    // The read job Undo() / redo() functions will handle all operations.
    MayaUsd::OpUndoItemMuting undoMuting;

    // NOTE: (yliangsiew) All chasers need to have their Undo run as well.
    for (const UsdMayaImportChaserRefPtr& chaser : this->mImportChasers) {
        bool bStat = chaser->Undo();
        if (!bStat) {
            TF_WARN("Failed to execute import chaser's Redo()!");
            return false;
        }
    }

    if (!mDagModifierSeeded) {
        mDagModifierSeeded = true;
        MStatus dagStatus;
        // Construct list of top level DAG nodes to delete and any DG nodes
        for (auto& it : mNewNodeRegistry) {
            if (it.second != mMayaRootDagPath.node()) { // if not the parent root node
                MFnDagNode dagFn(it.second, &dagStatus);
                if (dagStatus == MS::kSuccess) {
                    if (mMayaRootDagPath.node() != MObject::kNullObj) {
                        if (!dagFn.hasParent(mMayaRootDagPath.node())) { // skip if a DAG Node, but
                                                                         // not under the root
                            continue;
                        }
                    } else {
                        if (dagFn.parentCount() == 0) { // under scene root
                            continue;
                        }
                    }
                }
                mDagModifierUndo.deleteNode(it.second, false);
            }
        }
    }

    MStatus status = mDagModifierUndo.doIt();

    return (status == MS::kSuccess);
}

void UsdMaya_ReadJob::SetMayaRootDagPath(const MDagPath& mayaRootDagPath)
{
    mMayaRootDagPath = mayaRootDagPath;
}

const MDagPath& UsdMaya_ReadJob::GetMayaRootDagPath() const { return mMayaRootDagPath; }

double UsdMaya_ReadJob::timeSampleMultiplier() const { return mTimeSampleMultiplier; }

const UsdMayaPrimReaderContext::ObjectRegistry& UsdMaya_ReadJob::GetNewNodeRegistry() const
{
    return mNewNodeRegistry;
}

double UsdMaya_ReadJob::_setTimeSampleMultiplierFrom(const double layerFPS)
{
    double sceneFPS = UsdMayaUtil::GetSceneMTimeUnitAsDouble();
    mTimeSampleMultiplier = sceneFPS / layerFPS;
    return mTimeSampleMultiplier;
}

PXR_NAMESPACE_CLOSE_SCOPE
