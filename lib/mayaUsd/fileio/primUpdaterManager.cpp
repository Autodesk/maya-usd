//
// Copyright 2021 Autodesk
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
#include "primUpdaterManager.h"

#include <mayaUsd/fileio/fallbackPrimUpdater.h>
#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/readJob.h>
#include <mayaUsd/fileio/jobs/writeJob.h>
#include <mayaUsd/fileio/primUpdaterRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/traverseLayer.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>

#include <maya/MAnimControl.h>
#include <maya/MDagModifier.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MSceneMessage.h>
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/sceneNotification.h>

#include <functional>
#include <tuple>

// Allow for use of MObjectHandle with std::unordered_map.
namespace std {
template <> struct hash<MObjectHandle>
{
    std::size_t operator()(const MObjectHandle& obj) const { return obj.hashCode(); }
};
} // namespace std

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
extern Ufe::Rtid g_MayaRtid;

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

namespace {

const std::string kPullParentPathKey("Maya:Pull:ParentPath");

// Set name that will be used to hold all pulled objects
const std::string kPullSetName("pullStateSet");

// Metadata key used to store pull information on a prim
const TfToken kPullPrimMetadataKey("Maya:Pull:DagPath");

// Metadata key used to store pull information on a DG node
const MString kPullDGMetadataKey("Pull_UfePath");

// Name of Dag node under which all pulled sub-hierarchies are rooted.
const MString kPullRootName("__mayaUsd__");

//! Lock or unlock hierarchy starting at given root.
void lockNodes(const MDagPath& root, bool state)
{
    MItDag dagIt;
    dagIt.reset(root);
    for (; !dagIt.isDone(); dagIt.next()) {
        MFnDependencyNode node(dagIt.currentItem());
        if (node.isFromReferencedFile()) {
            dagIt.prune();
            continue;
        }
        node.setLocked(state);
    }
}

Ufe::Path usdToMaya(const Ufe::Path& usdPath)
{
    auto prim = MayaUsd::ufe::ufePathToPrim(usdPath);
    if (!TF_VERIFY(prim)) {
        return Ufe::Path();
    }
    std::string dagPathStr;
    if (!TF_VERIFY(PXR_NS::PrimUpdaterManager::readPullInformation(prim, dagPathStr))) {
        return Ufe::Path();
    }

    return Ufe::PathString::path(dagPathStr);
}

SdfPath ufeToSdfPath(const Ufe::Path& usdPath)
{
    auto segments = usdPath.getSegments();

    // Can be only a gateway node
    if (segments.size() <= 1) {
        return {};
    }

    return SdfPath(segments[1].string());
}

//------------------------------------------------------------------------------
//
void select(const MDagPath& dagPath)
{
    MSelectionList selList;
    selList.add(dagPath);
    MGlobal::setActiveSelectionList(selList, MGlobal::kReplaceList);
}

//------------------------------------------------------------------------------
//
// The UFE path and the prim refer to the same object: the prim is passed in as
// an optimization to avoid an additional call to ufePathToPrim().
bool writePullInformation(
    const Ufe::Path&       ufePulledPath,
    const PXR_NS::UsdPrim& pulledPrim,
    const MDagPath&        path)
{
    // Add to a set
    MObject pullSetObj;
    auto    status = UsdMayaUtil::GetMObjectByName(kPullSetName, pullSetObj);
    if (status != MStatus::kSuccess) {
        MFnSet         fnSet;
        MSelectionList selList;
        MObject        pullSetObj = fnSet.create(selList, MFnSet::kNone, &status);
        fnSet.setName(kPullSetName.c_str());
        fnSet.addMember(path);
    } else {
        MFnSet fnPullSet(pullSetObj);
        fnPullSet.addMember(path);
    }

    // Store metadata on the prim.
    VtValue value(path.fullPathName().asChar());
    pulledPrim.SetCustomDataByKey(kPullPrimMetadataKey, value);

    // Store medata on DG node
    auto              ufePathString = Ufe::PathString::string(ufePulledPath);
    MFnDependencyNode depNode(path.node());
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
    dgMetadata.setValue(ufePathString.c_str());

    return true;
}

//------------------------------------------------------------------------------
//
void removePullInformation(const Ufe::Path& ufePulledPath)
{
    UsdPrim prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    prim.ClearCustomDataByKey(kPullPrimMetadataKey);
}

//------------------------------------------------------------------------------
//
bool addExcludeFromRendering(const Ufe::Path& ufePulledPath)
{
    auto              proxyShape = MayaUsd::ufe::getProxyShape(ufePulledPath);
    MStatus           status;
    MFnDependencyNode depNode(proxyShape->thisMObject(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug excludePrimPathsPlug
        = depNode.findPlug(MayaUsdProxyShapeBase::excludePrimPathsAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MString excludePrimPathsStr;
    status = excludePrimPathsPlug.getValue(excludePrimPathsStr);
    std::vector<std::string> excludePrimPaths = TfStringTokenize(excludePrimPathsStr.asChar(), ",");

    std::string sdfPathStr = ufeToSdfPath(ufePulledPath).GetText();
    if (std::find(excludePrimPaths.begin(), excludePrimPaths.end(), sdfPathStr)
        != excludePrimPaths.end())
        return true;

    excludePrimPaths.push_back(sdfPathStr);
    excludePrimPathsStr = TfStringJoin(excludePrimPaths, ",").c_str();
    excludePrimPathsPlug.setValue(excludePrimPathsStr);

    return true;
}

//------------------------------------------------------------------------------
//
bool removeExcludeFromRendering(const Ufe::Path& ufePulledPath)
{
    auto        proxyShape = MayaUsd::ufe::getProxyShape(ufePulledPath);
    auto        prim = MayaUsd::ufe::ufePathToPrim(ufePulledPath);
    std::string sdfPathStr = prim.GetPath().GetText();

    MStatus           status;
    MFnDependencyNode depNode(proxyShape->thisMObject(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug excludePrimPathsPlug
        = depNode.findPlug(MayaUsdProxyShapeBase::excludePrimPathsAttr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MString excludePrimPathsStr;
    status = excludePrimPathsPlug.getValue(excludePrimPathsStr);
    std::vector<std::string> excludePrimPaths = TfStringTokenize(excludePrimPathsStr.asChar(), ",");

    auto foundIt = std::find(excludePrimPaths.begin(), excludePrimPaths.end(), sdfPathStr);
    if (foundIt == excludePrimPaths.end())
        return true;

    excludePrimPaths.erase(foundIt);
    excludePrimPathsStr = TfStringJoin(excludePrimPaths, ",").c_str();
    excludePrimPathsPlug.setValue(excludePrimPathsStr);
    return true;
}

//------------------------------------------------------------------------------
//
// Perform the import step of the pull (first step), with the argument
// prim as the root of the USD hierarchy to be pulled.  The UFE path and
// the prim refer to the same object: the prim is passed in as an
// optimization to avoid an additional call to ufePathToPrim().
using PullImportPaths = std::pair<std::vector<MDagPath>, std::vector<Ufe::Path>>;
PullImportPaths pullImport(
    const Ufe::Path&                 ufePulledPath,
    const UsdPrim&                   pulledPrim,
    const UsdMayaPrimUpdaterContext& context)
{
    std::vector<MDagPath>  addedDagPaths;
    std::vector<Ufe::Path> pulledUfePaths;

    std::string mFileName = context.GetUsdStage()->GetRootLayer()->GetIdentifier();
    if (mFileName.empty()) {
        TF_RUNTIME_ERROR("Empty file specified. Exiting.");
        return PullImportPaths(addedDagPaths, pulledUfePaths);
    }

    const VtDictionary& userArgs = context.GetUserArgs();

    UsdMayaJobImportArgs jobArgs = UsdMayaJobImportArgs::CreateFromDictionary(
        userArgs,
        /* importWithProxyShapes = */ false,
        GfInterval::GetFullInterval());

    MayaUsd::ImportData importData(mFileName);
    importData.setRootPrimPath(pulledPrim.GetPath().GetText());

    UsdMaya_ReadJob readJob(importData, jobArgs);
    auto            found = userArgs.find("Maya:Pull:ParentPath");
    if (found != userArgs.end()) {
        const std::string& dagPathStr = found->second.Get<std::string>();
        auto               pullParent = UsdMayaUtil::nameToDagPath(dagPathStr);
        if (pullParent.isValid()) {
            readJob.SetMayaRootDagPath(pullParent);
        }
    }

    // Execute the command
    bool success = readJob.Read(&addedDagPaths);

    const bool isCopy = context.GetArgs()._copyOperation;
    if (success && !isCopy) {
        // Quick workaround to reuse some POC code - to rewrite later
        auto ufeChild = MayaUsd::ufe::dagPathToUfe(addedDagPaths[0]);

        // Since we haven't pulled yet, obtaining the parent is simple, and
        // doesn't require going through the Hierarchy interface, which can do
        // non-trivial work on pulled objects to get their parent.
        auto ufeParent = ufePulledPath.pop();

        MString pyCommand;
        // The "child" is the node that will receive the computed parent
        // transformation, in its offsetParentMatrix attribute.  We are using
        // the pull parent for this purpose, so pop the path of the ufeChild to
        // get to its pull parent.
        pyCommand.format(
            "from mayaUsd.lib import proxyAccessor as pa\n"
            "import maya.cmds as cmds\n"
            "cmds.select('^1s', '^2s')\n"
            "pa.parent()\n"
            "cmds.select(clear=True)\n",
            Ufe::PathString::string(ufeChild.pop()).c_str(),
            Ufe::PathString::string(ufeParent).c_str());
        MGlobal::executePythonCommand(pyCommand);
        // -- end --

        // Finalize the pull.
        writePullInformation(ufePulledPath, pulledPrim, addedDagPaths[0]);
        addExcludeFromRendering(ufePulledPath);

        select(addedDagPaths[0]);
    }

    // Invert the new node registry, for MObject to Ufe::Path lookup.
    using ObjToUfePath = std::unordered_map<MObjectHandle, Ufe::Path>;
    ObjToUfePath objToUfePath;
    const auto&  ps = ufePulledPath.getSegments()[0];
    const auto   rtid = MayaUsd::ufe::getUsdRunTimeId();
    for (const auto& v : readJob.GetNewNodeRegistry()) {
        Ufe::Path::Segments s { ps, Ufe::PathSegment(v.first, rtid, '/') };
        Ufe::Path           p(std::move(s));
        objToUfePath.insert(ObjToUfePath::value_type(MObjectHandle(v.second), p));
    }

    // For each added Dag path, get the UFE path of the pulled USD prim.
    pulledUfePaths.reserve(addedDagPaths.size());
    for (const auto& dagPath : addedDagPaths) {
        auto found = objToUfePath.find(MObjectHandle(dagPath.node()));
        TF_AXIOM(found != objToUfePath.end());
        pulledUfePaths.emplace_back(found->second);
    }

    return PullImportPaths(addedDagPaths, pulledUfePaths);
}

//------------------------------------------------------------------------------
//
// Perform the customization step of the pull (second step).
bool pullCustomize(const PullImportPaths& importedPaths, const UsdMayaPrimUpdaterContext& context)
{
    TF_AXIOM(importedPaths.first.size() == importedPaths.second.size());
    auto dagPathIt = importedPaths.first.begin();
    auto ufePathIt = importedPaths.second.begin();
    for (; dagPathIt != importedPaths.first.end(); ++dagPathIt, ++ufePathIt) {
        const auto&       dagPath = *dagPathIt;
        const auto&       pulledUfePath = *ufePathIt;
        MFnDependencyNode dgNodeFn(dagPath.node());

        const std::string mayaTypeName(dgNodeFn.typeName().asChar());

        auto registryItem = UsdMayaPrimUpdaterRegistry::FindOrFallback(mayaTypeName);
        auto factory = std::get<UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn>(registryItem);
        auto updater = factory(dgNodeFn, pulledUfePath);

        // The failure of a single updater causes failure of the whole
        // customization step.  This is a frequent difficulty for operations on
        // multiple data, especially since we can't roll back the result of
        // the execution of previous updaters.  Revisit this.  PPT, 15-Sep-2021.
        if (!updater->pull(context)) {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
//
// Perform the export step of the merge to USD (first step).  Returns the
// source SdfPath and SdfLayer for the next step, push customize.  The source
// SdfPath will be empty on error.
using UsdPathToDagPathMap = TfHashMap<SdfPath, MDagPath, SdfPath::Hash>;
using UsdPathToDagPathMapPtr = std::shared_ptr<UsdPathToDagPathMap>;
using PushCustomizeSrc
    = std::tuple<SdfPath, UsdStageRefPtr, SdfLayerRefPtr, UsdPathToDagPathMapPtr>;

PushCustomizeSrc pushExport(
    const Ufe::Path&                 ufePulledPath,
    const MObject&                   mayaObject,
    const UsdMayaPrimUpdaterContext& context)
{
    UsdStageRefPtr         srcStage = UsdStage::CreateInMemory();
    SdfLayerRefPtr         srcLayer = srcStage->GetRootLayer();
    UsdPathToDagPathMapPtr pathMapPtr;
    auto pushCustomizeSrc = std::make_tuple(SdfPath(), srcStage, srcLayer, pathMapPtr);

    // Copy to be able to add the export root.
    VtDictionary userArgs = context.GetUserArgs();

    std::string fileName = srcLayer->GetIdentifier();

    MFnDagNode fnDag(mayaObject);
    MDagPath   dagPath;
    fnDag.getPath(dagPath);

    UsdMayaUtil::MDagPathSet dagPaths;
    dagPaths.insert(dagPath);

    GfInterval       timeInterval = PXR_NS::UsdMayaPrimUpdater::isAnimated(dagPath)
              ? GfInterval(MAnimControl::minTime().value(), MAnimControl::maxTime().value())
              : GfInterval();
    double           frameStride = 1.0;
    std::set<double> frameSamples;

    const std::vector<double> timeSamples
        = UsdMayaWriteUtil::GetTimeSamples(timeInterval, frameSamples, frameStride);

    // The pushed Dag node is the root of the export job.
    std::vector<VtValue> rootPathString(
        1, VtValue(std::string(dagPath.partialPathName().asChar())));
    userArgs[UsdMayaJobExportArgsTokens->exportRoots] = rootPathString;

    UsdMayaJobExportArgs jobArgs
        = UsdMayaJobExportArgs::CreateFromDictionary(userArgs, dagPaths, timeSamples);

    UsdMaya_WriteJob writeJob(jobArgs);
    if (!writeJob.Write(fileName, false /* append */)) {
        return pushCustomizeSrc;
    }

    std::get<SdfPath>(pushCustomizeSrc) = writeJob.MapDagPathToSdfPath(dagPath);

    // Invert the Dag path to USD path map, to return it for prim updater use.
    auto usdPathToDagPathMap = std::make_shared<UsdPathToDagPathMap>();
    for (const auto& v : writeJob.GetDagPathToUsdPathMap()) {
        usdPathToDagPathMap->insert(UsdPathToDagPathMap::value_type(v.second, v.first));
    }

    std::get<UsdPathToDagPathMapPtr>(pushCustomizeSrc) = usdPathToDagPathMap;

    const bool isCopy = context.GetArgs()._copyOperation;
    if (!isCopy) {
        // FIXME  Is it too early to remove the pull information?  If we remove
        // it here, the push customize step won't have access to it --- but
        // maybe it doesn't need to, because the UsdPathToDagPathMap is
        // available in the context.  PPT, 14-Oct-2021.
        removePullInformation(ufePulledPath);
        removeExcludeFromRendering(ufePulledPath);
    }

    return pushCustomizeSrc;
}

//------------------------------------------------------------------------------
//
SdfPath getDstSdfPath(const Ufe::Path& ufePulledPath, const SdfPath& srcSdfPath, bool isCopy)
{
    // If we got the destination path, extract it, otherwise use src path as
    // the destination.
    SdfPath dstSdfPath;
    if (ufePulledPath.nbSegments() == 2) {
        dstSdfPath = SdfPath(ufePulledPath.getSegments()[1].string());

        if (isCopy) {
            SdfPath relativeSrcSdfPath = srcSdfPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
            dstSdfPath = dstSdfPath.AppendPath(relativeSrcSdfPath);
        }
    } else {
        dstSdfPath = srcSdfPath;
    }

    return dstSdfPath;
}

//------------------------------------------------------------------------------
//
UsdMayaPrimUpdaterSharedPtr createUpdater(
    const SdfLayerRefPtr&            srcLayer,
    const SdfPath&                   primSpecPath,
    const UsdMayaPrimUpdaterContext& context)
{
    using UpdaterFactoryFn = UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn;

    // Get the primSpec from the src layer.
    auto primSpec = srcLayer->GetPrimAtPath(primSpecPath);
    if (!TF_VERIFY(primSpec)) {
        return nullptr;
    }

    TfToken typeName = primSpec->GetTypeName();
    auto    regItem = UsdMayaPrimUpdaterRegistry::FindOrFallback(typeName);
    auto    factory = std::get<UpdaterFactoryFn>(regItem);

    // Create the UFE path corresponding to the primSpecPath, as required
    // by the prim updater factory.
    auto                psPath = MayaUsd::ufe::stagePath(context.GetUsdStage());
    Ufe::Path::Segments segments { psPath.getSegments()[0],
                                   MayaUsd::ufe::usdPathToUfePathSegment(primSpecPath) };
    Ufe::Path           ufePath(std::move(segments));

    // Get the Maya object corresponding to the SdfPath.  As of 19-Oct-2021,
    // the export write job only registers Maya Dag path to SdfPath
    // correspondence, so prims that correspond to Maya DG nodes (e.g. material
    // networks) don't have a corresponding Dag path.  The prim updater
    // receives a null MObject in this case.
    auto              mayaDagPath = context.MapSdfPathToDagPath(primSpecPath);
    MFnDependencyNode depNodeFn(mayaDagPath.isValid() ? mayaDagPath.node() : MObject());

    return factory(depNodeFn, ufePath);
}

//------------------------------------------------------------------------------
//
// Perform the customization step of the merge to USD (second step).  Traverse
// the in-memory layer, creating a prim updater for each prim, and call Push
// for each updater.
bool pushCustomize(
    const Ufe::Path&                 ufePulledPath,
    const PushCustomizeSrc&          src,
    const UsdMayaPrimUpdaterContext& context)

{
    const auto& srcRootPath = std::get<SdfPath>(src);
    const auto& srcLayer = std::get<SdfLayerRefPtr>(src);
    const auto& srcStage = std::get<UsdStageRefPtr>(src);
    if (srcRootPath.IsEmpty() || !srcLayer || !srcStage) {
        return false;
    }

    const bool  isCopy = context.GetArgs()._copyOperation;
    const auto& editTarget = context.GetUsdStage()->GetEditTarget();
    auto dstRootPath = editTarget.MapToSpecPath(getDstSdfPath(ufePulledPath, srcRootPath, isCopy));
    auto dstRootParentPath = dstRootPath.GetParentPath();
    const auto& dstLayer = editTarget.GetLayer();

    // Traverse the layer, creating a prim updater for each primSpec
    // along the way, and call PushCopySpec on the prim.
    using UpdaterFactoryFn = UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn;
    auto pushCopySpecsFn
        = [&context, srcStage, srcLayer, dstLayer, dstRootParentPath](const SdfPath& srcPath) {
              // We can be called with a primSpec path that is not a prim path
              // (e.g. a property path like "/A.xformOp:translate").  This is not an
              // error, just prune the traversal.  FIXME Is this still true?  We
              // should not be traversing property specs.  PPT, 20-Oct-2021.
              if (!srcPath.IsPrimPath()) {
                  return false;
              }

              auto updater = createUpdater(srcLayer, srcPath, context);
              // If we cannot find an updater for the srcPath, prune the traversal.
              if (!updater) {
                  TF_WARN(
                      "Could not create a prim updater for path %s during PushCopySpecs traversal, "
                      "pruning at that point.",
                      srcPath.GetText());
                  return false;
              }
              auto relativeSrcPath = srcPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
              auto dstPath = dstRootParentPath.AppendPath(relativeSrcPath);

              // Report PushCopySpecs() failure.
              if (!updater->pushCopySpecs(
                      srcStage, srcLayer, srcPath, context.GetUsdStage(), dstLayer, dstPath)) {
                  throw MayaUsd::TraversalFailure(std::string("PushCopySpecs() failed."), srcPath);
              }

              // Continue normal traversal without pruning.
              return true;
          };

    if (!MayaUsd::traverseLayer(srcLayer, srcRootPath, pushCopySpecsFn)) {
        return false;
    }

    // Push end is a separate traversal, not a second phase of the same
    // traversal, because it is post-order: parents are traversed after
    // children.  This allows for proper parent lifescope, if push end
    // deletes the Maya node (which is the default behavior).
    if (isCopy) {
        return true;
    }

    // SdfLayer::TraversalFn does not return a status, so must report
    // failure through an exception.
    auto pushEndFn = [&context, srcLayer](const SdfPath& primSpecPath) {
        // We can be called with a primSpec path that is not a prim path
        // (e.g. a property path like "/A.xformOp:translate").  This is not an
        // error, just a no-op.
        if (!primSpecPath.IsPrimPath()) {
            return;
        }

        auto updater = createUpdater(srcLayer, primSpecPath, context);
        if (!updater) {
            TF_WARN(
                "Could not create a prim updater for path %s during PushEnd() traversal, pruning "
                "at that point.",
                primSpecPath.GetText());
            return;
        }

        // Report pushEnd() failure.
        if (!updater->pushEnd(context)) {
            throw MayaUsd::TraversalFailure(std::string("PushEnd() failed."), primSpecPath);
        }
    };

    // SdfLayer::Traverse does not return a status, so must report failure
    // through an exception.
    try {
        srcLayer->Traverse(srcRootPath, pushEndFn);
    } catch (const MayaUsd::TraversalFailure& e) {
        TF_WARN(
            "PushEnd() layer traversal failed for path %s: %s",
            e.path().GetText(),
            e.reason().c_str());
        return false;
    }

    return true;
}

class PushPullScope
{
public:
    PushPullScope(bool& controlingFlag)
    {
        if (!controlingFlag) {
            controlingFlag = true;
            _controlingFlag = &controlingFlag;
        }
    }
    ~PushPullScope()
    {
        if (_controlingFlag) {
            *_controlingFlag = false;
        }
    }

private:
    bool* _controlingFlag { nullptr };
};

} // namespace

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(PrimUpdaterManager);

PrimUpdaterManager::PrimUpdaterManager()
{
    MStatus res;
    _cbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeNew, beforeNewOrOpenCallback, this, &res));
    CHECK_MSTATUS(res);
    _cbIds.append(MSceneMessage::addCallback(
        MSceneMessage::kBeforeOpen, beforeNewOrOpenCallback, this, &res));
    CHECK_MSTATUS(res);

    TfSingleton<PrimUpdaterManager>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<PrimUpdaterManager>();

    TfWeakPtr<PrimUpdaterManager> me(this);
    TfNotice::Register(me, &PrimUpdaterManager::onProxyContentChanged);
}

PrimUpdaterManager::~PrimUpdaterManager()
{
    MMessage::removeCallbacks(_cbIds);
    _cbIds.clear();
}

bool PrimUpdaterManager::push(const MFnDependencyNode& depNodeFn, const Ufe::Path& pulledPath)
{
    MayaUsdProxyShapeBase* proxyShape = MayaUsd::ufe::getProxyShape(pulledPath);
    if (!proxyShape) {
        return false;
    }

    auto pulledPrim = MayaUsd::ufe::ufePathToPrim(pulledPath);
    if (!pulledPrim) {
        return false;
    }

    PushPullScope scopeIt(_inPushPull);

    VtDictionary exportArgs = UsdMayaJobExportArgs::GetDefaultDictionary();
    auto         updaterArgs = UsdMayaPrimUpdaterArgs::createFromDictionary(exportArgs);
    auto         mayaPath = usdToMaya(pulledPath);
    MDagPath     pullParentPath;
    const bool   isCopy = updaterArgs._copyOperation;
    if (!isCopy) {
        // The pull parent is simply the parent of the pulled path.
        pullParentPath = MayaUsd::ufe::ufeToDagPath(mayaPath.pop());
        if (!TF_VERIFY(pullParentPath.isValid())) {
            return false;
        }
        lockNodes(pullParentPath, false);
    }

    UsdStageRefPtr            proxyStage = proxyShape->usdPrim().GetStage();
    UsdMayaPrimUpdaterContext context(proxyShape->getTime(), proxyStage, exportArgs);

    auto  ufeMayaItem = Ufe::Hierarchy::createItem(mayaPath);
    auto& scene = Ufe::Scene::instance();
    if (!isCopy && TF_VERIFY(ufeMayaItem))
        scene.notify(Ufe::ObjectPreDelete(ufeMayaItem));

    // The push is done in two stages:
    // 1) Perform the export into a temporary layer.
    // 2) Traverse the layer and call the prim updater for each prim, for
    //    per-prim customization.

    // 1) Perform the export to the temporary layer.
    auto pushCustomizeSrc = pushExport(pulledPath, depNodeFn.object(), context);

    // 2) Traverse the in-memory layer, creating a prim updater for each prim,
    // and call Push for each updater.  Build a new context with the USD path
    // to Maya path mapping information.
    UsdMayaPrimUpdaterContext customizeContext(
        proxyShape->getTime(),
        proxyStage,
        exportArgs,
        std::get<UsdPathToDagPathMapPtr>(pushCustomizeSrc));

    if (!pushCustomize(pulledPath, pushCustomizeSrc, customizeContext)) {
        return false;
    }

    if (!isCopy) {
        if (!TF_VERIFY(removePullParent(pullParentPath))) {
            return false;
        }
    }

    auto ufeUsdItem = Ufe::Hierarchy::createItem(pulledPath);
    auto hier = Ufe::Hierarchy::hierarchy(ufeUsdItem);
    if (TF_VERIFY(hier)) {
        scene.notify(Ufe::SubtreeInvalidate(hier->defaultParent()));
    }

    return true;
}

bool PrimUpdaterManager::pull(const Ufe::Path& path)
{
    MayaUsdProxyShapeBase* proxyShape = MayaUsd::ufe::getProxyShape(path);
    if (!proxyShape) {
        return false;
    }

    auto pulledPrim = MayaUsd::ufe::ufePathToPrim(path);
    if (!pulledPrim) {
        return false;
    }

    PushPullScope scopeIt(_inPushPull);

    VtDictionary importArgs = UsdMayaJobImportArgs::GetDefaultDictionary();
    auto         updaterArgs = UsdMayaPrimUpdaterArgs::createFromDictionary(importArgs);

    MDagPath pullParentPath;
    if (!updaterArgs._copyOperation
        && !(pullParentPath = setupPullParent(path, importArgs)).isValid()) {
        return false;
    }

    UsdMayaPrimUpdaterContext context(proxyShape->getTime(), pulledPrim.GetStage(), importArgs);

    auto& scene = Ufe::Scene::instance();
    auto  ufeItem = Ufe::Hierarchy::createItem(path);
    if (!updaterArgs._copyOperation && TF_VERIFY(ufeItem))
        scene.notify(Ufe::ObjectPreDelete(ufeItem));

    // The pull is done in two stages:
    // 1) Perform the import into Maya.
    // 2) Iterate over all imported Dag paths and call the prim updater on
    //    each, for per-prim customization.

    // 1) Perform the import
    PullImportPaths importedPaths = pullImport(path, pulledPrim, context);

    // 2) Iterate over all imported Dag paths.
    if (!pullCustomize(importedPaths, context)) {
        return false;
    }

    if (!updaterArgs._copyOperation) {
        // Lock pulled nodes starting at the pull parent.
        lockNodes(pullParentPath, true);
    }

    // We must recreate the UFE item because it has changed data models (USD -> Maya).
    ufeItem = Ufe::Hierarchy::createItem(usdToMaya(path));
    if (TF_VERIFY(ufeItem))
        scene.notify(Ufe::ObjectAdd(ufeItem));

    return true;
}

bool PrimUpdaterManager::discardEdits(const Ufe::Path& pulledPath)
{
    MayaUsdProxyShapeBase* proxyShape = MayaUsd::ufe::getProxyShape(pulledPath);
    if (!proxyShape) {
        return false;
    }

    PushPullScope scopeIt(_inPushPull);

    auto mayaPath = usdToMaya(pulledPath);
    auto mayaDagPath = MayaUsd::ufe::ufeToDagPath(mayaPath);

    VtDictionary              userArgs;
    UsdMayaPrimUpdaterContext context(
        proxyShape->getTime(), proxyShape->usdPrim().GetStage(), userArgs);

    auto  ufeMayaItem = Ufe::Hierarchy::createItem(mayaPath);
    auto& scene = Ufe::Scene::instance();
    if (TF_VERIFY(ufeMayaItem))
        scene.notify(Ufe::ObjectPreDelete(ufeMayaItem));

    // Unlock the pulled hierarchy, clear the pull information, and remove the
    // pull parent, which is simply the parent of the pulled path.
    auto pullParent = mayaDagPath;
    pullParent.pop();
    if (!TF_VERIFY(pullParent.isValid())) {
        return false;
    }
    lockNodes(pullParent, false);

    MItDag dagIt;
    for (dagIt.reset(mayaDagPath); !dagIt.isDone(); dagIt.next()) {
        MDagPath curDagPath;
        dagIt.getPath(curDagPath);
        MFnDependencyNode   depNodeFn(curDagPath.node());
        FallbackPrimUpdater fallback(depNodeFn, Ufe::Path());
        fallback.discardEdits(context);
    }

    removePullInformation(pulledPath);
    removeExcludeFromRendering(pulledPath);

    if (!TF_VERIFY(removePullParent(pullParent))) {
        return false;
    }

    auto ufeUsdItem = Ufe::Hierarchy::createItem(pulledPath);
    auto hier = Ufe::Hierarchy::hierarchy(ufeUsdItem);
    if (TF_VERIFY(hier)) {
        scene.notify(Ufe::SubtreeInvalidate(hier->defaultParent()));
    }
    return true;
}

bool PrimUpdaterManager::copyBetween(const Ufe::Path& srcPath, const Ufe::Path& dstPath)
{
    MayaUsdProxyShapeBase* srcProxyShape = MayaUsd::ufe::getProxyShape(srcPath);
    MayaUsdProxyShapeBase* dstProxyShape = MayaUsd::ufe::getProxyShape(dstPath);

    PushPullScope scopeIt(_inPushPull);

    // Copy from USD to DG
    if (srcProxyShape && dstProxyShape == nullptr) {
        auto srcPrim = MayaUsd::ufe::ufePathToPrim(srcPath);
        if (!srcPrim) {
            return false;
        }

        VtDictionary userArgs = UsdMayaJobImportArgs::GetDefaultDictionary();
        // We will only do copy between two data models, setting this in arguments
        // to configure the updater
        userArgs[UsdMayaPrimUpdaterArgsTokens->copyOperation] = true;

        UsdMayaPrimUpdaterContext context(
            srcProxyShape->getTime(), srcProxyShape->getUsdStage(), userArgs);

        pullImport(srcPath, srcPrim, context);
        return true;
    }
    // Copy from DG to USD
    else if (srcProxyShape == nullptr && dstProxyShape) {
        TF_AXIOM(srcPath.nbSegments() == 1);
        MDagPath dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(Ufe::PathString::string(srcPath));
        if (!dagPath.isValid()) {
            return false;
        }
        VtDictionary userArgs = UsdMayaJobExportArgs::GetDefaultDictionary();

        // We will only do copy between two data models, setting this in arguments
        // to configure the updater
        userArgs[UsdMayaPrimUpdaterArgsTokens->copyOperation] = true;
        auto                      dstStage = dstProxyShape->getUsdStage();
        UsdMayaPrimUpdaterContext context(dstProxyShape->getTime(), dstStage, userArgs);

        // Export out to a temporary layer.
        auto        pushExportOutput = pushExport(srcPath, dagPath.node(), context);
        const auto& srcRootPath = std::get<SdfPath>(pushExportOutput);
        if (srcRootPath.IsEmpty()) {
            return false;
        }

        // Copy the temporary layer contents out to the proper destination.
        const auto& srcLayer = std::get<SdfLayerRefPtr>(pushExportOutput);
        const auto& editTarget = dstStage->GetEditTarget();
        auto        dstRootPath = editTarget.MapToSpecPath(srcRootPath);
        const auto& dstLayer = editTarget.GetLayer();

        if (!SdfCopySpec(srcLayer, srcRootPath, dstLayer, dstRootPath)) {
            return false;
        }

        auto ufeItem = Ufe::Hierarchy::createItem(dstPath);
        if (TF_VERIFY(ufeItem)) {
            Ufe::Scene::instance().notify(Ufe::SubtreeInvalidate(ufeItem));
        }
        return true;
    }

    // Copy operations to the same data model not supported here.
    return false;
}

void PrimUpdaterManager::onProxyContentChanged(
    const MayaUsdProxyStageObjectsChangedNotice& proxyNotice)
{
    if (_inPushPull) {
        return;
    }

    const UsdNotice::ObjectsChanged& notice = proxyNotice.GetNotice();

    Usd_PrimFlagsPredicate predicate = UsdPrimDefaultPredicate;

    auto stage = notice.GetStage();
    for (const auto& changedPath : notice.GetResyncedPaths()) {
        if (changedPath == SdfPath::AbsoluteRootPath()) {
            continue;
        }

        UsdPrim      resyncPrim = stage->GetPrimAtPath(changedPath);
        UsdPrimRange range(resyncPrim, predicate);

        for (auto it = range.begin(); it != range.end(); it++) {
            const UsdPrim& prim = *it;

            TfToken typeName = prim.GetTypeName();

            auto registryItem = UsdMayaPrimUpdaterRegistry::FindOrFallback(typeName);
            auto supports = std::get<UsdMayaPrimUpdater::Supports>(registryItem);

            if ((supports & UsdMayaPrimUpdater::Supports::AutoPull)
                == UsdMayaPrimUpdater::Supports::AutoPull) {
                it.PruneChildren();

                const Ufe::PathSegment pathSegment
                    = MayaUsd::ufe::usdPathToUfePathSegment(prim.GetPath());
                const Ufe::Path path = proxyNotice.GetProxyShape().ufePath() + pathSegment;

                pull(path);
            }
        }
    }
}

PrimUpdaterManager& PrimUpdaterManager::getInstance()
{
    return TfSingleton<PrimUpdaterManager>::GetInstance();
}

bool PrimUpdaterManager::findOrCreatePullRoot()
{
    // If we already found the pull root, good to go.
    if (!_pullRoot.isNull()) {
        return true;
    }

    // No saved pull root.  Try to find one in the scene, and save its MObject.
    auto       worldObj = MItDag().root();
    MFnDagNode world(worldObj);
    auto       nbWorldChildren = world.childCount();
    for (unsigned int i = 0; i < nbWorldChildren; ++i) {
        auto              childObj = world.child(i);
        MFnDependencyNode child(childObj);
        if (child.name() == kPullRootName) {
            _pullRoot = childObj;
            return true;
        }
    }

    // No pull root in the scene, so create one.
    MDagModifier dagMod;
    MStatus      status;
    MObject      pullRootObj = dagMod.createNode(MString("transform"), MObject::kNullObj, &status);
    if (status != MStatus::kSuccess) {
        return false;
    }
    status = dagMod.renameNode(pullRootObj, kPullRootName);
    if (status != MStatus::kSuccess) {
        return false;
    }

    if (dagMod.doIt() != MStatus::kSuccess) {
        return false;
    }

    // Hide all objects under the pull root in the Outliner so only the pulled
    // objects under a proxy shape will be shown.
    MFnDependencyNode pullRootFn(pullRootObj);
    UsdMayaUtil::SetHiddenInOutliner(pullRootFn, true);

    _pullRoot = pullRootObj;
    return true;
}

MObject PrimUpdaterManager::createPullParent(const Ufe::Path& pulledPath)
{
    MDagModifier dagMod;
    MStatus      status;
    MObject      pullParentObj = dagMod.createNode(MString("transform"), _pullRoot, &status);
    if (status != MStatus::kSuccess) {
        return MObject::kNullObj;
    }

    // Rename the pull parent to be the name of the node plus a "Parent" suffix.
    status = dagMod.renameNode(
        pullParentObj, MString(pulledPath.back().string().c_str()) + MString("Parent"));

    return (dagMod.doIt() == MStatus::kSuccess) ? pullParentObj : MObject::kNullObj;
}

bool PrimUpdaterManager::removePullParent(const MDagPath& parentDagPath)
{
    if (!TF_VERIFY(parentDagPath.isValid())) {
        return false;
    }

    MDGModifier dgMod;
    if (dgMod.deleteNode(parentDagPath.node()) != MStatus::kSuccess) {
        return false;
    }

    // If the pull parent was the last child of the pull root, remove the pull
    // root as well, and null out our pull root cache.
    MFnDagNode pullRoot(_pullRoot);
    auto       nbPullRootChildren = pullRoot.childCount();
    if (nbPullRootChildren == 1) {
        if (dgMod.deleteNode(_pullRoot) != MStatus::kSuccess) {
            return false;
        }
        _pullRoot = MObject::kNullObj;
    }

    return dgMod.doIt() == MStatus::kSuccess;
}

MDagPath PrimUpdaterManager::setupPullParent(const Ufe::Path& pulledPath, VtDictionary& args)
{
    if (!findOrCreatePullRoot()) {
        return MDagPath();
    }

    auto pullParent = createPullParent(pulledPath);
    if (pullParent == MObject::kNullObj) {
        return MDagPath();
    }

    // Pull parent is not instanced, so use first path found.
    MDagPath pullParentPath;
    if (MDagPath::getAPathTo(pullParent, pullParentPath) != MStatus::kSuccess) {
        return MDagPath();
    }

    // Add pull parent path to import args as a string.
    args[kPullParentPathKey] = VtValue(std::string(pullParentPath.fullPathName().asChar()));

    return pullParentPath;
}

/*static*/
void PrimUpdaterManager::beforeNewOrOpenCallback(void* clientData)
{
    auto um = static_cast<PrimUpdaterManager*>(clientData);
    um->_pullRoot = MObject::kNullObj;
}

/* static */
bool PrimUpdaterManager::readPullInformation(const PXR_NS::UsdPrim& prim, std::string& dagPathStr)
{
    auto value = prim.GetCustomDataByKey(kPullPrimMetadataKey);
    if (!value.IsEmpty() && value.CanCast<std::string>()) {
        dagPathStr = value.Get<std::string>();
        return !dagPathStr.empty();
    }
    return false;
}

/* static */
bool PrimUpdaterManager::readPullInformation(
    const PXR_NS::UsdPrim& prim,
    Ufe::SceneItem::Ptr&   dagPathItem)
{
    std::string dagPathStr;
    if (readPullInformation(prim, dagPathStr)) {
        dagPathItem = Ufe::Hierarchy::createItem(Ufe::PathString::path(dagPathStr));
        return (bool)dagPathItem;
    }
    return false;
}

/* static */
bool PrimUpdaterManager::readPullInformation(const Ufe::Path& ufePath, MDagPath& dagPath)
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

/* static */
bool PrimUpdaterManager::readPullInformation(const MDagPath& dagPath, Ufe::Path& ufePath)
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

PXR_NAMESPACE_CLOSE_SCOPE
