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
#include "primUpdaterManager.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/fileio/fallbackPrimUpdater.h>
#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/readJob.h>
#include <mayaUsd/fileio/jobs/writeJob.h>
#ifdef HAS_ORPHANED_NODES_MANAGER
#include <mayaUsd/fileio/orphanedNodesManager.h>
#endif
#include <mayaUsd/fileio/primUpdaterRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/undo/OpUndoItemMuting.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/copyLayerPrims.h>
#include <mayaUsd/utils/dynamicAttribute.h>
#include <mayaUsd/utils/progressBarScope.h>
#include <mayaUsd/utils/traverseLayer.h>
#include <mayaUsd/utils/trieVisitor.h>

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>

#include <maya/MAnimControl.h>
#include <maya/MDagModifier.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MSceneMessage.h>
#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/sceneNotification.h>
#include <ufe/trie.imp.h>

#include <functional>
#include <tuple>

using UpdaterFactoryFn = UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn;
using namespace MayaUsd;

// Allow for use of MObjectHandle with std::unordered_map.
namespace std {
template <> struct hash<MObjectHandle>
{
    std::size_t operator()(const MObjectHandle& obj) const { return obj.hashCode(); }
};
} // namespace std

namespace {

const std::string kPullParentPathKey("Maya:Pull:ParentPath");

// Set name that will be used to hold all pulled objects
const MString kPullSetName("usdEditAsMaya");

// Name of Dag node under which all pulled sub-hierarchies are rooted.
const MString kPullRootName("__mayaUsd__");
const MString kPullRootPath("|__mayaUsd__");

MObject findPullRoot()
{
    // Try to find the pull root in the scene.
    MSelectionList sel;
    sel.add(kPullRootPath);
    if (sel.isEmpty())
        return MObject();

    MObject obj;
    sel.getDependNode(0, obj);
    return obj;
}

Ufe::Path usdToMaya(const Ufe::Path& usdPath)
{
    auto prim = MayaUsd::ufe::ufePathToPrim(usdPath);
    if (!TF_VERIFY(prim)) {
        return Ufe::Path();
    }
    std::string dagPathStr;
    if (!TF_VERIFY(MayaUsd::readPullInformation(prim, dagPathStr))) {
        return Ufe::Path();
    }

    return Ufe::PathString::path(dagPathStr);
}

SdfPath makeDstPath(const SdfPath& dstRootParentPath, const SdfPath& srcPath)
{
    auto relativeSrcPath = srcPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
    return dstRootParentPath.AppendPath(relativeSrcPath);
}
} // namespace

//------------------------------------------------------------------------------
//
// Verify if the given prim under the given UFE path is an ancestor of an already edited prim.
bool PrimUpdaterManager::hasEditedDescendant(const Ufe::Path& ufeQueryPath) const
{
#ifdef HAS_ORPHANED_NODES_MANAGER
    if (_orphanedNodesManager->has(ufeQueryPath))
        return true;
#endif

    MObject pullSetObj;
    auto    status = UsdMayaUtil::GetMObjectByName(kPullSetName, pullSetObj);
    if (status != MStatus::kSuccess)
        return false;

    MFnSet         fnPullSet(pullSetObj);
    MSelectionList members;
    const bool     flatten = true;
    fnPullSet.getMembers(members, flatten);

    for (unsigned int i = 0; i < members.length(); ++i) {
        MDagPath pulledDagPath;
        members.getDagPath(i, pulledDagPath);
        Ufe::Path pulledUfePath;
        if (!readPullInformation(pulledDagPath, pulledUfePath))
            continue;

#ifdef HAS_ORPHANED_NODES_MANAGER
        // If the alread-edited node is orphaned, don't take it into consideration.
        if (_orphanedNodesManager) {
            if (_orphanedNodesManager->isOrphaned(pulledUfePath, pulledDagPath)) {
                continue;
            }
        }
#endif

        if (pulledUfePath.startsWith(ufeQueryPath))
            return true;
    }

    return false;
}

namespace {
//------------------------------------------------------------------------------
//
// The UFE path is to the pulled prim, and the Dag path is the corresponding
// Maya pulled object.
bool writeAllPullInformation(const Ufe::Path& ufePulledPath, const MDagPath& editedAsMayaRoot)
{
    MayaUsd::ProgressBarScope progressBar(3);

    // Add to a set, the set should already been created.
    if (!FunctionUndoItem::execute(
            "Add edited item to pull set.",
            [editedAsMayaRoot]() {
                MObject pullSetObj;
                auto    status = UsdMayaUtil::GetMObjectByName(kPullSetName, pullSetObj);
                if (status != MStatus::kSuccess)
                    return false;
                MFnSet fnPullSet(pullSetObj);
                fnPullSet.addMember(editedAsMayaRoot);
                return true;
            },
            [editedAsMayaRoot]() {
                MObject pullSetObj;
                auto    status = UsdMayaUtil::GetMObjectByName(kPullSetName, pullSetObj);
                if (status != MStatus::kSuccess)
                    return false;
                MFnSet fnPullSet(pullSetObj);
                fnPullSet.removeMember(editedAsMayaRoot, MObject::kNullObj);
                return true;
            })) {
        TF_WARN("Cannot edited object to pulled set.");
        return false;
    }
    progressBar.advance();

    // Store metadata on the prim in the Session Layer.
    writePulledPrimMetadata(ufePulledPath, editedAsMayaRoot);
    progressBar.advance();

    // Store medata on DG node
    writePullInformation(ufePulledPath, editedAsMayaRoot);
    progressBar.advance();

    return true;
}

//------------------------------------------------------------------------------
//
void removeAllPullInformation(const Ufe::Path& ufePulledPath)
{
    MayaUsd::ProgressBarScope progressBar(1);
    removePulledPrimMetadata(ufePulledPath);
    progressBar.advance();
}

//------------------------------------------------------------------------------
//
// Turn on the mesh flag to allow topological modifications.
bool allowTopologyModifications(MDagPath& root)
{
    MDGModifier& dgMod = MDGModifierUndoItem::create("Allow topology modifications");

    MItDag dagIt;
    dagIt.reset(root, MItDag::kDepthFirst, MFn::kMesh);
    for (; !dagIt.isDone(); dagIt.next()) {
        MFnDependencyNode depNode(dagIt.item());
        MPlug             topoPlug = depNode.findPlug("allowTopologyMod");
        if (topoPlug.isNull())
            continue;
        dgMod.newPlugValueBool(topoPlug, true);
    }

    return dgMod.doIt();
}

UsdMayaJobImportArgs CreateImportArgsForPullImport(const VtDictionary& basicUserArgs)
{
    VtDictionary userArgs(basicUserArgs);

    MString              optionsString;
    static const MString optionVarName("usdMaya_EditAsMayaDataOptions");
    if (MGlobal::optionVarExists(optionVarName)) {
        optionsString = MGlobal::optionVarStringValue(optionVarName);
    }

    bool readAnimData = true;
    if (optionsString.length() > 0) {
        MStringArray optionList;
        MStringArray theOption;
        optionsString.split(';', optionList);
        for (int i = 0, n = optionList.length(); i < n; ++i) {
            theOption.clear();
            optionList[i].split('=', theOption);
            if (theOption.length() != 2) {
                continue;
            }

            std::string argName(theOption[0].asChar(), theOption[0].length());
            if (argName == "readAnimData") {
                readAnimData = (theOption[1].asInt() != 0);
            } else {
                userArgs[argName] = UsdMayaUtil::ParseArgumentValue(
                    argName, theOption[1].asChar(), UsdMayaJobImportArgs::GetGuideDictionary());
            }
        }
    }

    GfInterval timeInterval;
    if (readAnimData) {
        timeInterval = GfInterval::GetFullInterval();
    }

    return UsdMayaJobImportArgs::CreateFromDictionary(
        userArgs,
        /* importWithProxyShapes = */ false,
        timeInterval);
}

//------------------------------------------------------------------------------
//
// Perform the import step of the pull (first step), with the argument
// prim as the root of the USD hierarchy to be pulled.  The UFE path and
// the prim refer to the same object: the prim is passed in as an
// optimization to avoid an additional call to ufePathToPrim().
using PullImportPaths = std::vector<std::pair<MDagPath, Ufe::Path>>;
PullImportPaths pullImport(
    const Ufe::Path&                 ufePulledPath,
    const UsdPrim&                   pulledPrim,
    const UsdMayaPrimUpdaterContext& context)
{
    MayaUsd::ProgressBarScope progressBar(9);

    std::string mFileName = context.GetUsdStage()->GetRootLayer()->GetIdentifier();
    if (mFileName.empty()) {
        TF_WARN("Nothing to edit: invalid layer.");
        return PullImportPaths();
    }
    progressBar.advance();

    VtDictionary userArgs(context.GetUserArgs());
    userArgs[UsdMayaJobImportArgsTokens->pullImportStage] = PXR_NS::VtValue(context.GetUsdStage());
    userArgs[UsdMayaJobImportArgsTokens->preserveTimeline] = true;

    UsdMayaJobImportArgs jobArgs = CreateImportArgsForPullImport(userArgs);
    MayaUsd::ImportData  importData(mFileName);
    importData.setRootPrimPath(pulledPrim.GetPath().GetText());

    auto readJob = std::make_shared<UsdMaya_ReadJob>(importData, jobArgs);
    progressBar.advance();

    MDagPath pullParentPath;
    {
        auto found = userArgs.find(kPullParentPathKey);
        if (found != userArgs.end()) {
            const std::string& dagPathStr = found->second.Get<std::string>();
            pullParentPath = UsdMayaUtil::nameToDagPath(dagPathStr);
            if (pullParentPath.isValid()) {
                readJob->SetMayaRootDagPath(pullParentPath);
            }
        }
    }
    progressBar.advance();

    std::vector<MDagPath> addedDagPaths;

    // Execute the command, which can succeed but import nothing.
    bool success = readJob->Read(&addedDagPaths);
    if (!success || addedDagPaths.size() == 0) {
        TF_WARN("Nothing to edit in the selection.");
        return PullImportPaths();
    }
    progressBar.advance();

    // Note: UsdMaya_ReadJob has explicit Read(), Undo() and Redo() functions,
    //       and Read() has already been called, so create the function-undo item
    //       but do not execute it.
    FunctionUndoItem::create(
        "Edit as Maya USD import",
        [readJob]() { return readJob->Redo(); },
        [readJob]() { return readJob->Undo(); });

    MDagPath addedDagPath = addedDagPaths[0];
    progressBar.advance();

    const bool isCopy = context.GetArgs()._copyOperation;
    if (!isCopy) {
        progressBar.addSteps(4);

        // Quick workaround to reuse some POC code - to rewrite later

        // Communication to current proxyAccessor code is through the global
        // selection, so we must save the current selection for proper undo.
        // This is not logically necessary, and should be re-written to avoid
        // going through the global selection.
        if (!UfeSelectionUndoItem::select(
                "Pre-proxyAccessor selection", *Ufe::GlobalSelection::get())) {
            TF_WARN("Cannot save the selection.");
            return PullImportPaths();
        }

        // The "child" is the node that will receive the computed parent
        // transformation, in its offsetParentMatrix attribute.  We are using
        // the pull parent for this purpose, so pop the path of the ufeChild to
        // get to its pull parent.
        auto ufeChild = MayaUsd::ufe::dagPathToUfe(addedDagPath).pop();

        // Since we haven't pulled yet, obtaining the parent is simple, and
        // doesn't require going through the Hierarchy interface, which can do
        // non-trivial work on pulled objects to get their parent.
        auto ufeParent = ufePulledPath.pop();

        MString pyCommand;
        pyCommand.format(
            "from mayaUsd.lib import proxyAccessor as pa\n"
            "import maya.cmds as cmds\n"
            "cmds.select('^1s', '^2s')\n"
            "pa.parent()\n"
            "cmds.select(clear=True)\n",
            Ufe::PathString::string(ufeChild).c_str(),
            Ufe::PathString::string(ufeParent).c_str());

        MString pyUndoCommand;
        pyUndoCommand.format(
            "from mayaUsd.lib import proxyAccessor as pa\n"
            "import maya.cmds as cmds\n"
            "cmds.select('^1s', '^2s')\n"
            "pa.unparent()\n"
            "cmds.select(clear=True)\n",
            Ufe::PathString::string(ufeChild).c_str(),
            Ufe::PathString::string(ufeParent).c_str());

        if (!PythonUndoItem::execute(
                "Pull import proxy accessor parenting", pyCommand, pyUndoCommand)) {
            TF_WARN("Cannot parent pulled object.");
            return PullImportPaths();
        }
        progressBar.advance();

        // Create the pull set if it does not exists.
        //
        // Note: do not use the MFnSet API to create it as it clears the redo stack
        // and thus prevents redo.
        MObject pullSetObj;
        MStatus status = UsdMayaUtil::GetMObjectByName(kPullSetName, pullSetObj);
        if (status != MStatus::kSuccess) {
            MString createSetCmd;
            createSetCmd.format("sets -em -name \"^1s\";lockNode \"^1s\";", kPullSetName.asChar());
            MDGModifier& dgMod
                = MDGModifierUndoItem::create("Pull import pull set creation and lock");
            dgMod.commandToExecute(createSetCmd);
            dgMod.doIt();
        }
        progressBar.advance();

        // Finalize the pull.
        if (!FunctionUndoItem::execute(
                "Pull import pull info writing",
                [ufePulledPath, addedDagPath]() {
                    return writeAllPullInformation(ufePulledPath, addedDagPath);
                },
                [ufePulledPath]() {
                    removeAllPullInformation(ufePulledPath);
                    return true;
                })) {
            TF_WARN("Cannot write pull information metadata.");
            return PullImportPaths();
        }
        progressBar.advance();

        if (!FunctionUndoItem::execute(
                "Pull import rendering exclusion",
                [ufePulledPath]() { return addExcludeFromRendering(ufePulledPath); },
                [ufePulledPath]() {
                    removeExcludeFromRendering(ufePulledPath);
                    return true;
                })) {
            TF_WARN("Cannot exclude original USD data from viewport rendering.");
            return PullImportPaths();
        }
        progressBar.advance();

        if (!UfeSelectionUndoItem::select("Pull import select DAG node", addedDagPath)) {
            TF_WARN("Cannot select the pulled nodes.");
            return PullImportPaths();
        }
    }
    progressBar.advance();

    // Invert the new node registry, for MObject to Ufe::Path lookup.
    using ObjToUfePath = std::unordered_map<MObjectHandle, Ufe::Path>;
    ObjToUfePath objToUfePath;
    const auto&  ps = ufePulledPath.getSegments()[0];
    const auto   rtid = MayaUsd::ufe::getUsdRunTimeId();
    for (const auto& v : readJob->GetNewNodeRegistry()) {
        Ufe::Path::Segments s { ps, Ufe::PathSegment(v.first, rtid, '/') };
        Ufe::Path           p(std::move(s));
        objToUfePath.insert(ObjToUfePath::value_type(MObjectHandle(v.second), p));

        context._pullExtras.processItem(p, v.second);
    }
    progressBar.advance();

    // For each added Dag path, get the UFE path of the pulled USD prim.
    PullImportPaths pulledPaths;
    pulledPaths.reserve(addedDagPaths.size());
    for (const auto& dagPath : addedDagPaths) {
        auto found = objToUfePath.find(MObjectHandle(dagPath.node()));
        if (TF_VERIFY(found != objToUfePath.end())) {
            pulledPaths.emplace_back(std::make_pair(dagPath, found->second));
        }
    }
    progressBar.advance();

    progressBar.advance();
    return pulledPaths;
}

//------------------------------------------------------------------------------
//
UsdMayaPrimUpdaterRegistry::RegisterItem getUpdaterItem(const MFnDependencyNode& dgNodeFn)
{
    MPlug usdTypeNamePlug = dgNodeFn.findPlug("USD_typeName", true);

    // If the Maya node holds USD type information (e.g. a dummy transform
    // node which is a stand-in for a non-transform USD prim type), use that
    // USD type intead of the Maya node type name.
    if (!usdTypeNamePlug.isNull())
        return UsdMayaPrimUpdaterRegistry::FindOrFallback(
            TfToken(usdTypeNamePlug.asString().asChar()));

    // In the absence of explicit USD type name, use the Maya type name.
    return UsdMayaPrimUpdaterRegistry::FindOrFallback(dgNodeFn.typeName().asChar());
}

//------------------------------------------------------------------------------
//
// Perform the customization step of the pull (second step).
bool pullCustomize(const PullImportPaths& importedPaths, const UsdMayaPrimUpdaterContext& context)
{
    // The number of imported paths should (hopefully) never be so great
    // as to overwhelm the computation with progress bar updates.
    MayaUsd::ProgressBarScope progressBar(importedPaths.size());

    // Record all USD modifications in an undo block and item.
    UsdUfe::UsdUndoBlock undoBlock(
        &UsdUndoableItemUndoItem::create("Pull customize USD data modifications"));

    for (const auto& importedPair : importedPaths) {
        const auto&       dagPath = importedPair.first;
        const auto&       pulledUfePath = importedPair.second;
        MFnDependencyNode dgNodeFn(dagPath.node());

        auto registryItem = getUpdaterItem(dgNodeFn);
        auto factory = std::get<UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn>(registryItem);
        auto updater = factory(context, dgNodeFn, pulledUfePath);

        // The failure of a single updater causes failure of the whole
        // customization step.  This is a frequent difficulty for operations on
        // multiple data, especially since we can't roll back the result of
        // the execution of previous updaters.  Revisit this.  PPT, 15-Sep-2021.
        if (!updater->editAsMaya()) {
            return false;
        }
        progressBar.advance();
    }
    return true;
}

// The user arguments might not contain the final output filename,
// so fill the user args dictionary with the known output file name.
void fillUserArgsFileIfEmpty(VtDictionary& userArgs, const std::string& fileName)
{
    if (userArgs.count(UsdMayaJobExportArgsTokens->file) == 0
        || userArgs[UsdMayaJobExportArgsTokens->file].Get<std::string>() == "") {
        userArgs[UsdMayaJobExportArgsTokens->file] = fileName;
    }
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
    MayaUsd::ProgressBarScope progressBar(3);

    UsdStageRefPtr         srcStage = UsdStage::CreateInMemory();
    SdfLayerRefPtr         srcLayer = srcStage->GetRootLayer();
    UsdPathToDagPathMapPtr pathMapPtr;
    auto pushCustomizeSrc = std::make_tuple(SdfPath(), srcStage, srcLayer, pathMapPtr);

    // Copy to be able to add the export root.
    VtDictionary userArgs = context.GetUserArgs();

    std::string fileName = srcLayer->GetIdentifier();

    fillUserArgsFileIfEmpty(userArgs, fileName);

    MFnDagNode fnDag(mayaObject);
    MDagPath   dagPath;
    fnDag.getPath(dagPath);

    UsdMayaUtil::MDagPathSet dagPaths;
    dagPaths.insert(dagPath);

    std::vector<double> timeSamples;
    UsdMayaJobExportArgs::GetDictionaryTimeSamples(userArgs, timeSamples);

    // The pushed Dag node is the root of the export job.
    std::vector<VtValue> rootPathString(
        1, VtValue(std::string(dagPath.partialPathName().asChar())));
    userArgs[UsdMayaJobExportArgsTokens->exportRoots] = rootPathString;

    UsdMayaJobExportArgs jobArgs
        = UsdMayaJobExportArgs::CreateFromDictionary(userArgs, dagPaths, timeSamples);
    progressBar.advance();

    UsdMaya_WriteJob writeJob(jobArgs);
    if (!writeJob.Write(fileName, false /* append */)) {
        return pushCustomizeSrc;
    }
    progressBar.advance();

    std::get<SdfPath>(pushCustomizeSrc) = writeJob.MapDagPathToSdfPath(dagPath);

    // Invert the Dag path to USD path map, to return it for prim updater use.
    auto usdPathToDagPathMap = std::make_shared<UsdPathToDagPathMap>();
    for (const auto& v : writeJob.GetDagPathToUsdPathMap()) {
        usdPathToDagPathMap->insert(UsdPathToDagPathMap::value_type(v.second, v.first));
        context._pushExtras.processItem(v.first, v.second);
    }

    std::get<UsdPathToDagPathMapPtr>(pushCustomizeSrc) = usdPathToDagPathMap;
    progressBar.advance();

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
// Create an updater for use with both pushCustomize() traversals /
// customization points: pushCopySpec() and pushEnd().
//
// pushCopySpec() and pushEnd() must use the same updater type.  An earlier
// version of this function tried to ensure this by using the pulled prim to
// create the updater.  However, this prim cannot be relied on, as
// pushCopySpec() has an edit router customization point that can remove the
// pulled prim from the USD scene (e.g. by switching a variant set to a
// different variant, such as what occurs when caching to a variant).  It is
// more robust to use the USD primSpec type at srcPath, which is in the
// srcLayer in the temporary stage.  If USD type round-tripping is set up
// properly (see UsdMayaTranslatorUtil::CreateDummyTransformNode()), this
// primSpec will have the type of the original pulled prim.
//
UsdMayaPrimUpdaterSharedPtr createUpdater(
    const SdfLayerRefPtr&            srcLayer,
    const SdfPath&                   srcPath,
    const SdfPath&                   dstPath,
    const UsdMayaPrimUpdaterContext& context)
{
    auto primSpec = srcLayer->GetPrimAtPath(srcPath);
    if (!TF_VERIFY(primSpec)) {
        return nullptr;
    }

    auto typeName = primSpec->GetTypeName();
    auto regItem = UsdMayaPrimUpdaterRegistry::FindOrFallback(typeName);
    auto factory = std::get<UpdaterFactoryFn>(regItem);

    // We cannot use the srcPath to create the UFE path, as this path is in the
    // in-memory stage in the temporary srcLayer and does not exist in UFE.
    // Use the dstPath instead, which can be validly added to the proxy shape
    // path to form a proper UFE path.
    auto                psPath = MayaUsd::ufe::stagePath(context.GetUsdStage());
    Ufe::Path::Segments segments { psPath.getSegments()[0],
                                   UsdUfe::usdPathToUfePathSegment(dstPath) };
    Ufe::Path           ufePath(std::move(segments));

    // Get the Maya object corresponding to the SdfPath.  As of 19-Oct-2021,
    // the export write job only registers Maya Dag path to SdfPath
    // correspondence, so prims that correspond to Maya DG nodes (e.g. material
    // networks) don't have a corresponding Dag path.  The prim updater
    // receives a null MObject in this case.
    auto              mayaDagPath = context.MapSdfPathToDagPath(srcPath);
    MFnDependencyNode depNodeFn(mayaDagPath.isValid() ? mayaDagPath.node() : MObject());

    return factory(context, depNodeFn, ufePath);
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

    MayaUsd::ProgressBarScope progressBar(2);

    const bool            isCopy = context.GetArgs()._copyOperation;
    const UsdEditTarget&  editTarget = context.GetUsdStage()->GetEditTarget();
    const SdfPath         dstPath = getDstSdfPath(ufePulledPath, srcRootPath, isCopy);
    const SdfPath         dstRootPath = editTarget.MapToSpecPath(dstPath);
    const SdfPath         dstRootParentPath = dstRootPath.GetParentPath();
    const SdfLayerHandle& dstLayer = editTarget.GetLayer();

    // Traverse the layer, creating a prim updater for each primSpec
    // along the way, and call PushCopySpec on the prim.
    auto pushCopySpecsFn
        = [&context, srcStage, srcLayer, dstLayer, dstRootParentPath](const SdfPath& srcPath) {
              // We can be called with a primSpec path that is not a prim path
              // (e.g. a property path like "/A.xformOp:translate").  This is not an
              // error, just prune the traversal.  FIXME Is this still true?  We
              // should not be traversing property specs.  PPT, 20-Oct-2021.
              if (!srcPath.IsPrimPath()) {
                  return false;
              }

              auto dstPath = makeDstPath(dstRootParentPath, srcPath);
              auto updater = createUpdater(srcLayer, srcPath, dstPath, context);
              // If we cannot find an updater for the srcPath, prune the traversal.
              if (!updater) {
                  TF_WARN(
                      "Could not create a prim updater for path %s during PushCopySpecs traversal, "
                      "pruning at that point.",
                      srcPath.GetText());
                  return false;
              }

              // Report PushCopySpecs() failure.
              auto result = updater->pushCopySpecs(
                  srcStage, srcLayer, srcPath, context.GetUsdStage(), dstLayer, dstPath);
              if (result == UsdMayaPrimUpdater::PushCopySpecs::Failed) {
                  throw MayaUsd::TraversalFailure(std::string("PushCopySpecs() failed."), srcPath);
              }

              // If we don't continue, we prune.
              return result == UsdMayaPrimUpdater::PushCopySpecs::Continue;
          };

    if (!MayaUsd::traverseLayer(srcLayer, srcRootPath, pushCopySpecsFn)) {
        return false;
    }
    progressBar.advance();

    // Push end is a separate traversal, not a second phase of the same
    // traversal, because it is post-order: parents are traversed after
    // children.  This allows for proper parent lifescope, if push end
    // deletes the Maya node (which is the default behavior).
    if (isCopy) {
        return true;
    }

    // SdfLayer::TraversalFn does not return a status, so must report
    // failure through an exception.
    auto pushEndFn = [&context, srcLayer, dstLayer, dstRootParentPath](const SdfPath& srcPath) {
        // We can be called with a primSpec path that is not a prim path
        // (e.g. a property path like "/A.xformOp:translate").  This is not an
        // error, just a no-op.
        if (!srcPath.IsPrimPath()) {
            return;
        }

        auto dstPath = makeDstPath(dstRootParentPath, srcPath);
        auto updater = createUpdater(srcLayer, srcPath, dstPath, context);
        if (!updater) {
            TF_WARN(
                "Could not create a prim updater for path %s during PushEnd() traversal, pruning "
                "at that point.",
                srcPath.GetText());
            return;
        }

        // Report pushEnd() failure.
        if (!updater->pushEnd()) {
            throw MayaUsd::TraversalFailure(std::string("PushEnd() failed."), srcPath);
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
    progressBar.advance();

    return true;
}

class PushPullScope
{
public:
    PushPullScope(bool& controllingFlag)
    {
        if (!controllingFlag) {
            controllingFlag = true;
            _controllingFlag = &controllingFlag;
        }
    }
    ~PushPullScope()
    {
        if (_controllingFlag) {
            *_controllingFlag = false;
        }
    }

    void end()
    {
        if (_controllingFlag) {
            *_controllingFlag = false;
            _controllingFlag = nullptr;
        }
    }

private:
    bool* _controllingFlag { nullptr };
};

#ifdef HAS_ORPHANED_NODES_MANAGER
class RecordPullVariantInfoUndoItem : public MayaUsd::OpUndoItem
{
public:
    // Add the path to the orphaned nodes manager, and add an undo entry onto
    // the global undo list.
    static bool execute(
        const std::shared_ptr<OrphanedNodesManager>& orphanedNodesManager,
        const Ufe::Path&                             pulledPath,
        const MDagPath&                              editedAsMayaRoot)
    {
        // Get the global undo list.
        auto& undoInfo = OpUndoItemList::instance();

        auto item = std::make_unique<RecordPullVariantInfoUndoItem>(
            orphanedNodesManager, pulledPath, editedAsMayaRoot);
        if (!item->redo()) {
            return false;
        }

        undoInfo.addItem(std::move(item));

        return true;
    }

    RecordPullVariantInfoUndoItem(
        const std::shared_ptr<OrphanedNodesManager>& orphanedNodesManager,
        const Ufe::Path&                             pulledPath,
        const MDagPath&                              editedAsMayaRoot)
        : OpUndoItem(
            std::string("Add to orphaned nodes manager pull path ")
            + Ufe::PathString::string(pulledPath))
        , _orphanedNodesManager(orphanedNodesManager)
        , _pulledPath(pulledPath)
        , _editedAsMayaRoot(editedAsMayaRoot)
    {
    }

    bool undo() override
    {
        _orphanedNodesManager->remove(_pulledPath, _editedAsMayaRoot);
        return true;
    }

    bool redo() override
    {
        _orphanedNodesManager->add(_pulledPath, _editedAsMayaRoot);
        return true;
    }

private:
    const std::shared_ptr<OrphanedNodesManager> _orphanedNodesManager;
    const Ufe::Path                             _pulledPath;
    const MDagPath                              _editedAsMayaRoot;
};

class RemovePullVariantInfoUndoItem : public MayaUsd::OpUndoItem
{
public:
    // Remove the path from the orphaned nodes manager, and add an entry onto
    // the global undo list.
    static bool execute(
        const std::shared_ptr<OrphanedNodesManager>& orphanedNodesManager,
        const Ufe::Path&                             pulledPath,
        const MDagPath&                              editedAsMayaRoot)
    {
        // Get the global undo list.
        auto& undoInfo = OpUndoItemList::instance();

        auto item = std::make_unique<RemovePullVariantInfoUndoItem>(
            orphanedNodesManager, pulledPath, editedAsMayaRoot);
        if (!item->redo()) {
            return false;
        }

        undoInfo.addItem(std::move(item));

        return true;
    }

    RemovePullVariantInfoUndoItem(
        const std::shared_ptr<OrphanedNodesManager>& orphanedNodesManager,
        const Ufe::Path&                             pulledPath,
        const MDagPath&                              editedAsMayaRoot)
        : OpUndoItem(std::string("Remove pull path ") + Ufe::PathString::string(pulledPath))
        , _orphanedNodesManager(orphanedNodesManager)
        , _pulledPath(pulledPath)
        , _editedAsMayaRoot(editedAsMayaRoot)
    {
    }

    bool undo() override
    {
        _orphanedNodesManager->restore(std::move(_memento));
        return true;
    }

    bool redo() override
    {
        _memento = _orphanedNodesManager->remove(_pulledPath, _editedAsMayaRoot);
        return true;
    }

private:
    const std::shared_ptr<OrphanedNodesManager> _orphanedNodesManager;
    const Ufe::Path                             _pulledPath;
    const MDagPath                              _editedAsMayaRoot;

    // Created by redo().
    OrphanedNodesManager::Memento _memento;
};
#endif

void executeAdditionalCommands(const UsdMayaPrimUpdaterContext& context)
{
    std::shared_ptr<Ufe::CompositeUndoableCommand> cmds = context.GetAdditionalFinalCommands();
    UfeCommandUndoItem::execute("Additional final commands", cmds);
}

} // namespace

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(PrimUpdaterManager);

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(PrimUpdaterManager);

PrimUpdaterManager::PrimUpdaterManager()
#ifdef HAS_ORPHANED_NODES_MANAGER
    : _orphanedNodesManager(std::make_shared<OrphanedNodesManager>())
#endif
{
    TfSingleton<PrimUpdaterManager>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<PrimUpdaterManager>();

    TfWeakPtr<PrimUpdaterManager> me(this);
    TfNotice::Register(me, &PrimUpdaterManager::onProxyContentChanged);

#ifdef HAS_ORPHANED_NODES_MANAGER
    beginLoadSaveCallbacks();
#endif
}

PrimUpdaterManager::~PrimUpdaterManager()
{
#ifdef HAS_ORPHANED_NODES_MANAGER
    endLoadSaveCallbacks();
    endManagePulledPrims();
#endif
}

bool PrimUpdaterManager::mergeToUsd(
    const MFnDependencyNode& depNodeFn,
    const Ufe::Path&         pulledPath,
    const VtDictionary&      userArgs)
{
    MayaUsdProxyShapeBase* proxyShape = MayaUsd::ufe::getProxyShape(pulledPath);
    if (!proxyShape) {
        return false;
    }

    auto pulledPrim = MayaUsd::ufe::ufePathToPrim(pulledPath);
    if (!pulledPrim) {
        return false;
    }

    // Are we doing a merge or cache?
    MString progStr(
        VtDictionaryIsHolding<std::string>(userArgs, MayaUsdEditRoutingTokens->DestinationPrimName)
            ? "Caching to USD"
            : "Merging to USD");
    MayaUsd::ProgressBarScope progressBar(11, progStr);
    PushPullScope             scopeIt(_inPushPull);

    auto ctxArgs = VtDictionaryOver(userArgs, UsdMayaJobExportArgs::GetDefaultDictionary());

    auto       updaterArgs = UsdMayaPrimUpdaterArgs::createFromDictionary(ctxArgs);
    auto       mayaPath = usdToMaya(pulledPath);
    auto       mayaDagPath = MayaUsd::ufe::ufeToDagPath(mayaPath);
    MDagPath   pullParentPath;
    const bool isCopy = updaterArgs._copyOperation;
    if (!isCopy) {
        // The pull parent is simply the parent of the pulled path.
        pullParentPath = MayaUsd::ufe::ufeToDagPath(mayaPath.pop());
        if (!TF_VERIFY(pullParentPath.isValid())) {
            return false;
        }
        if (!LockNodesUndoItem::lock("Merge to USD node unlocking", pullParentPath, false)) {
            return false;
        }
    }
    progressBar.advance();

    // If the user-provided argument does *not* contain an animation key, then
    // automatically infer if we should merge animations.
    if (!VtDictionaryIsHolding<bool>(userArgs, UsdMayaJobExportArgsTokens->animation)) {
        const bool isAnimated = PXR_NS::UsdMayaPrimUpdater::isAnimated(mayaDagPath);
        GfInterval timeInterval = isAnimated
            ? GfInterval(MAnimControl::minTime().value(), MAnimControl::maxTime().value())
            : GfInterval();

        ctxArgs[UsdMayaJobExportArgsTokens->animation] = isAnimated;
        ctxArgs[UsdMayaJobExportArgsTokens->frameStride] = 1.0;
        ctxArgs[UsdMayaJobExportArgsTokens->startTime] = timeInterval.GetMin();
        ctxArgs[UsdMayaJobExportArgsTokens->endTime] = timeInterval.GetMax();
    } else if (ctxArgs[UsdMayaJobExportArgsTokens->animation] == true) {
        // If user asked for animation but there is no animation, skip the exportation of animation.
        const bool isAnimated = PXR_NS::UsdMayaPrimUpdater::isAnimated(mayaDagPath);
        if (!isAnimated)
            ctxArgs[UsdMayaJobExportArgsTokens->animation] = false;
    }
    progressBar.advance();

    // Reset the selection, otherwise it will keep a reference to a deleted node
    // and crash later on.
    if (!UfeSelectionUndoItem::clear("Merge to USD selection reset")) {
        TF_WARN("Cannot reset the selection.");
        return false;
    }

    UsdStageRefPtr            proxyStage = proxyShape->usdPrim().GetStage();
    UsdMayaPrimUpdaterContext context(proxyShape->getTime(), proxyStage, ctxArgs);

    auto  ufeMayaItem = Ufe::Hierarchy::createItem(mayaPath);
    auto& scene = Ufe::Scene::instance();
    if (!isCopy && TF_VERIFY(ufeMayaItem))
        scene.notify(Ufe::ObjectPreDelete(ufeMayaItem));
    progressBar.advance();

    // Remove the pulled path from the orphan node manager *before* exporting
    // and merging into the original USD. Otherwise, the orphan manager can
    // receive notification mid-way through the merge process, while the variants
    // have not all been authored and think the variant set has changed back
    // to the correct variant and thus decide to deactivate the USD prim again,
    // thinking the Maya data shoudl be shown again...
#ifdef HAS_ORPHANED_NODES_MANAGER
    if (_orphanedNodesManager) {
        if (!TF_VERIFY(RemovePullVariantInfoUndoItem::execute(
                _orphanedNodesManager, pulledPath, mayaDagPath))) {
            return false;
        }
    }
#endif

    // Record all USD modifications in an undo block and item.
    UsdUfe::UsdUndoBlock undoBlock(
        &UsdUndoableItemUndoItem::create("Merge to Maya USD data modifications"));

    // The push is done in two stages:
    // 1) Perform the export into a temporary layer.
    // 2) Traverse the layer and call the prim updater for each prim, for
    //    per-prim customization.

    // 1) Perform the export to the temporary layer.
    auto pushCustomizeSrc = pushExport(pulledPath, depNodeFn.object(), context);
    progressBar.advance();

    // 2) Traverse the in-memory layer, creating a prim updater for each prim,
    // and call Push for each updater.  Build a new context with the USD path
    // to Maya path mapping information.
    context.SetUsdPathToDagPathMap(std::get<UsdPathToDagPathMapPtr>(pushCustomizeSrc));

    if (!isCopy) {
        if (!FunctionUndoItem::execute(
                "Merge to Maya rendering inclusion",
                [pulledPath]() {
                    removeExcludeFromRendering(pulledPath);
                    return true;
                },
                [pulledPath]() { return addExcludeFromRendering(pulledPath); })) {
            TF_WARN("Cannot re-enable original USD data in viewport rendering.");
            return false;
        }
    }
    progressBar.advance();

    if (!pushCustomize(pulledPath, pushCustomizeSrc, context)) {
        return false;
    }
    progressBar.advance();

    if (!isCopy) {
        if (!FunctionUndoItem::execute(
                "Merge to Maya pull info removal",
                [pulledPath]() {
                    removeAllPullInformation(pulledPath);
                    return true;
                },
                [pulledPath, mayaDagPath]() {
                    return writeAllPullInformation(pulledPath, mayaDagPath);
                })) {
            TF_WARN("Cannot remove pull information metadata.");
            return false;
        }
    }
    progressBar.advance();

    // Discard all pulled Maya nodes.
    std::vector<MDagPath> toApplyOn = UsdMayaUtil::getDescendantsStartingWithChildren(mayaDagPath);
    MayaUsd::ProgressBarLoopScope toApplyOnLoop(toApplyOn.size());
    for (const MDagPath& curDagPath : toApplyOn) {
        MStatus status = NodeDeletionUndoItem::deleteNode(
            "Merge to USD Maya scene cleanup", curDagPath.fullPathName(), curDagPath.node());
        if (status != MS::kSuccess) {
            TF_WARN(
                "Merge to USD Maya scene cleanup: cannot delete node \"%s\".",
                curDagPath.fullPathName().asChar());
            return false;
        }
        toApplyOnLoop.loopAdvance();
    }

    if (!isCopy) {
        if (!TF_VERIFY(removePullParent(pullParentPath, pulledPath))) {
            return false;
        }
    }
    progressBar.advance();

    context._pushExtras.finalize(MayaUsd::ufe::stagePath(context.GetUsdStage()), {});
    progressBar.advance();

    discardPullSetIfEmpty();

    // Some updaters (like MayaReference) may be writing and changing the variant during merge.
    // This will change the hierarchy around pulled prim. Grab hierarchy from the parent.
    auto ufeUsdItem = Ufe::Hierarchy::createItem(pulledPath.pop());
    auto hier = Ufe::Hierarchy::hierarchy(ufeUsdItem);
    if (TF_VERIFY(hier)) {
        scene.notify(Ufe::SubtreeInvalidate(hier->parent()));
    }
    progressBar.advance();

    scopeIt.end();
    executeAdditionalCommands(context);
    progressBar.advance();

    return true;
}

bool PrimUpdaterManager::editAsMaya(const Ufe::Path& path, const VtDictionary& userArgs)
{
    if (hasEditedDescendant(path)) {
        TF_WARN("Cannot edit an ancestor (%s) of an already edited node.", path.string().c_str());
        return false;
    }

    MayaUsdProxyShapeBase* proxyShape = MayaUsd::ufe::getProxyShape(path);
    if (!proxyShape) {
        return false;
    }

    auto pulledPrim = MayaUsd::ufe::ufePathToPrim(path);
    if (!pulledPrim) {
        return false;
    }

    if (pulledPrim.IsInstanceProxy()) {
        TF_WARN("Cannot edit a USD instance proxy.");
        return false;
    }

    MayaUsd::ProgressBarScope progressBar(7, "Converting to Maya Data");

    PushPullScope scopeIt(_inPushPull);

    auto ctxArgs = VtDictionaryOver(userArgs, UsdMayaJobImportArgs::GetDefaultDictionary());
    auto updaterArgs = UsdMayaPrimUpdaterArgs::createFromDictionary(ctxArgs);

    MDagPath pullParentPath;
    if (!updaterArgs._copyOperation
        && !(pullParentPath = setupPullParent(path, ctxArgs)).isValid()) {
        TF_WARN("Cannot setup the edit parent node.");
        return false;
    }
    progressBar.advance();

    UsdMayaPrimUpdaterContext context(proxyShape->getTime(), pulledPrim.GetStage(), ctxArgs);

    auto& scene = Ufe::Scene::instance();
    auto  ufeItem = Ufe::Hierarchy::createItem(path);
    context._pullExtras.initRecursive(ufeItem);
    if (!updaterArgs._copyOperation && TF_VERIFY(ufeItem))
        scene.notify(Ufe::ObjectPreDelete(ufeItem));

    progressBar.advance();

    // The pull is done in two stages:
    // 1) Perform the import into Maya.
    // 2) Iterate over all imported Dag paths and call the prim updater on
    //    each, for per-prim customization.

    // 1) Perform the import
    PullImportPaths importedPaths = pullImport(path, pulledPrim, context);
    if (importedPaths.empty()) {
        return false;
    }
    progressBar.advance();

    // 2) Iterate over all imported Dag paths.
    if (!pullCustomize(importedPaths, context)) {
        TF_WARN("Failed to customize the edited nodes.");
        return false;
    }
    progressBar.advance();

#ifdef HAS_ORPHANED_NODES_MANAGER
    if (_orphanedNodesManager) {
        RecordPullVariantInfoUndoItem::execute(_orphanedNodesManager, path, importedPaths[0].first);
    }
#endif

    if (!updaterArgs._copyOperation) {
        // Lock pulled nodes starting at the pull parent.
        if (!LockNodesUndoItem::lock("Edit as Maya node locking", pullParentPath, true))
            return false;

        // Allow editing topology, which gets turned of by locking.
        if (!allowTopologyModifications(pullParentPath))
            return false;
    }
    progressBar.advance();

    // We must recreate the UFE item because it has changed data models (USD -> Maya).
    ufeItem = Ufe::Hierarchy::createItem(usdToMaya(path));
    if (TF_VERIFY(ufeItem))
        scene.notify(Ufe::ObjectAdd(ufeItem));
    progressBar.advance();

    scopeIt.end();
    executeAdditionalCommands(context);
    progressBar.advance();

    return true;
}

bool PrimUpdaterManager::canEditAsMaya(const Ufe::Path& path) const
{
    // Verify if the prim is an ancestor of an edited prim.
    if (hasEditedDescendant(path))
        return false;

    // Create a prim updater for the path, and ask it if the prim can be edited
    // as Maya.
    auto prim = MayaUsd::ufe::ufePathToPrim(path);
    if (!prim) {
        return false;
    }

    // USD refuses that we modify point instance proxies, so detect that.
    if (prim.IsInstanceProxy()) {
        return false;
    }

    VtDictionary              userArgs;
    UsdMayaPrimUpdaterContext context(UsdTimeCode::Default(), prim.GetStage(), userArgs);

    auto typeName = prim.GetTypeName();
    auto regItem = UsdMayaPrimUpdaterRegistry::FindOrFallback(typeName);
    auto factory = std::get<UpdaterFactoryFn>(regItem);
    // No Maya Dag path for the prim updater, so pass in a null MObject.
    auto updater = factory(context, MFnDependencyNode(MObject()), path);
    return updater ? updater->canEditAsMaya() : false;
}

bool PrimUpdaterManager::discardEdits(const MDagPath& dagPath)
{
    Ufe::Path pulledPath;
    if (!readPullInformation(dagPath, pulledPath))
        return false;

    MayaUsd::ProgressBarScope progressBar(1, "Discarding Converted Maya Data");

    auto usdPrim = MayaUsd::ufe::ufePathToPrim(pulledPath);

#ifdef HAS_ORPHANED_NODES_MANAGER
    auto ret = _orphanedNodesManager->isOrphaned(pulledPath, dagPath)
        ? discardOrphanedEdits(dagPath, pulledPath)
        : discardPrimEdits(pulledPath);
#else
    // The following is incorrect: because of pull information in the session
    // layer stored as overs, the usdPrim will never be invalid: a prim that
    // exists only because of over opinions is valid, but is typeless.
    // Therefore, the conditional will always succeed, and
    // discardOrphanedEdits() is never called.  PPT, 30-Sep-2022.
    auto ret = usdPrim ? discardPrimEdits(pulledPath) : discardOrphanedEdits(dagPath, pulledPath);
#endif
    progressBar.advance();
    return ret;
}

bool PrimUpdaterManager::discardPrimEdits(const Ufe::Path& pulledPath)
{
    MayaUsdProxyShapeBase* proxyShape = MayaUsd::ufe::getProxyShape(pulledPath);
    if (!proxyShape) {
        return false;
    }

    MayaUsd::ProgressBarScope progressBar(6);
    PushPullScope             scopeIt(_inPushPull);

    // Record all USD modifications in an undo block and item.
    UsdUfe::UsdUndoBlock undoBlock(
        &UsdUndoableItemUndoItem::create("Discard edits USD data modifications"));

    auto mayaPath = usdToMaya(pulledPath);
    auto mayaDagPath = MayaUsd::ufe::ufeToDagPath(mayaPath);

    UsdMayaPrimUpdaterContext context(
        proxyShape->getTime(), proxyShape->usdPrim().GetStage(), VtDictionary());

    auto  ufeMayaItem = Ufe::Hierarchy::createItem(mayaPath);
    auto& scene = Ufe::Scene::instance();
    if (TF_VERIFY(ufeMayaItem))
        scene.notify(Ufe::ObjectPreDelete(ufeMayaItem));
    progressBar.advance();

    // Unlock the pulled hierarchy, clear the pull information, and remove the
    // pull parent, which is simply the parent of the pulled path.
    auto pullParent = mayaDagPath;
    pullParent.pop();
    if (!TF_VERIFY(pullParent.isValid())) {
        return false;
    }
    if (!LockNodesUndoItem::lock("Discard edits node unlocking", pullParent, false)) {
        return false;
    }
    progressBar.advance();

    // Reset the selection, otherwise it will keep a reference to a deleted node
    // and crash later on.
    if (!UfeSelectionUndoItem::clear("Discard edits selection reset")) {
        TF_WARN("Cannot reset the selection.");
        return false;
    }
    progressBar.advance();

    // Discard all pulled Maya nodes.
    std::vector<MDagPath> toApplyOn = UsdMayaUtil::getDescendantsStartingWithChildren(mayaDagPath);
    MayaUsd::ProgressBarLoopScope toApplyOnLoop(toApplyOn.size());
    for (const MDagPath& curDagPath : toApplyOn) {
        MFnDependencyNode dgNodeFn(curDagPath.node());

        const Ufe::Path path = MayaUsd::ufe::dagPathToPathSegment(curDagPath);

        auto registryItem = getUpdaterItem(dgNodeFn);
        auto factory = std::get<UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn>(registryItem);
        auto updater = factory(context, dgNodeFn, path);

        updater->discardEdits();
        toApplyOnLoop.loopAdvance();
    }

#ifdef HAS_ORPHANED_NODES_MANAGER
    if (_orphanedNodesManager) {
        if (!TF_VERIFY(RemovePullVariantInfoUndoItem::execute(
                _orphanedNodesManager, pulledPath, mayaDagPath))) {
            return false;
        }
    }
#endif

    if (!FunctionUndoItem::execute(
            "Discard edits pull info removal",
            [pulledPath]() {
                removeAllPullInformation(pulledPath);
                return true;
            },
            [pulledPath, mayaDagPath]() {
                return writeAllPullInformation(pulledPath, mayaDagPath);
            })) {
        TF_WARN("Cannot remove pull information metadata.");
        return false;
    }

    if (!FunctionUndoItem::execute(
            "Discard edits rendering inclusion",
            [pulledPath]() {
                removeExcludeFromRendering(pulledPath);
                return true;
            },
            [pulledPath]() { return addExcludeFromRendering(pulledPath); })) {
        TF_WARN("Cannot re-enable original USD data in viewport rendering.");
        return false;
    }

    if (!TF_VERIFY(removePullParent(pullParent, pulledPath))) {
        return false;
    }
    progressBar.advance();

    discardPullSetIfEmpty();

    auto ufeUsdItem = Ufe::Hierarchy::createItem(pulledPath);
    auto hier = Ufe::Hierarchy::hierarchy(ufeUsdItem);
    if (TF_VERIFY(hier)) {
        scene.notify(Ufe::SubtreeInvalidate(hier->parent()));
    }
    progressBar.advance();

    scopeIt.end();
    executeAdditionalCommands(context);
    progressBar.advance();

    return true;
}

bool PrimUpdaterManager::discardOrphanedEdits(const MDagPath& dagPath, const Ufe::Path& pulledPath)
{
    MayaUsd::ProgressBarScope progressBar(3);
    PushPullScope             scopeIt(_inPushPull);

    // Unlock the pulled hierarchy, clear the pull information, and remove the
    // pull parent, which is simply the parent of the pulled path.
    auto pullParent = dagPath;
    pullParent.pop();

    if (!LockNodesUndoItem::lock("Discard orphaned edits node unlocking", pullParent, false)) {
        return false;
    }

    // Reset the selection, otherwise it will keep a reference to a deleted node
    // and crash later on.
    if (!UfeSelectionUndoItem::clear("Discard orphaned edits selection reset")) {
        TF_WARN("Cannot reset the selection.");
        return false;
    }

    UsdMayaPrimUpdaterContext context(UsdTimeCode(), nullptr, VtDictionary());
    progressBar.advance();

    // Discard all pulled Maya nodes.
    std::vector<MDagPath> toApplyOn = UsdMayaUtil::getDescendantsStartingWithChildren(dagPath);
    MayaUsd::ProgressBarLoopScope toApplyOnLoop(toApplyOn.size());
    for (const MDagPath& curDagPath : toApplyOn) {
        MFnDependencyNode dgNodeFn(curDagPath.node());

        auto registryItem = getUpdaterItem(dgNodeFn);
        auto factory = std::get<UsdMayaPrimUpdaterRegistry::UpdaterFactoryFn>(registryItem);
        auto updater = factory(context, dgNodeFn, Ufe::Path());

        updater->discardEdits();
        toApplyOnLoop.loopAdvance();
    }

#ifdef HAS_ORPHANED_NODES_MANAGER
    if (_orphanedNodesManager) {
        if (!TF_VERIFY(RemovePullVariantInfoUndoItem::execute(
                _orphanedNodesManager, pulledPath, dagPath))) {
            return false;
        }
    }
#endif

    if (!TF_VERIFY(removePullParent(pullParent, pulledPath))) {
        return false;
    }
    progressBar.advance();

    scopeIt.end();
    executeAdditionalCommands(context);
    progressBar.advance();

    return true;
}

void PrimUpdaterManager::discardPullSetIfEmpty()
{
    // Discard of the pull set if it is empty.
    //
    // Note: do not use the MFnSet API to discard it as it clears the redo stack
    // and thus prevents redo.
    MObject pullSetObj;
    MStatus status = UsdMayaUtil::GetMObjectByName(kPullSetName, pullSetObj);
    if (status == MStatus::kSuccess) {
        MFnSet         fnPullSet(pullSetObj);
        MSelectionList members;
        const bool     flatten = true;
        fnPullSet.getMembers(members, flatten);

        if (members.length() == 0) {
            MString deleteSetCmd;
            deleteSetCmd.format(
                "lockNode -lock off \"^1s\";delete \"^1s\";", kPullSetName.asChar());
            MDGModifier& dgMod = MDGModifierUndoItem::create("Discard edits pull set removal");
            dgMod.commandToExecute(deleteSetCmd);
            dgMod.doIt();
        }
    }
}

bool PrimUpdaterManager::duplicate(
    const Ufe::Path&    srcPath,
    const Ufe::Path&    dstPath,
    const VtDictionary& userArgs)
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

        MayaUsd::ProgressBarScope progressBar(3, "Duplicating to Maya Data");

        auto ctxArgs = VtDictionaryOver(userArgs, UsdMayaJobImportArgs::GetDefaultDictionary());

        // We will only do copy between two data models, setting this in arguments
        // to configure the updater
        ctxArgs[UsdMayaPrimUpdaterArgsTokens->copyOperation] = true;

        // Set destination of duplicate. The Maya world MDagPath is not valid,
        // so don't try to validate the path if it is the world root.
        MDagPath pullParentPath;
        if (!MayaUsd::ufe::isMayaWorldPath(dstPath)) {
            pullParentPath = MayaUsd::ufe::ufeToDagPath(dstPath);
            if (!pullParentPath.isValid()) {
                return false;
            }
        }
        ctxArgs[kPullParentPathKey] = VtValue(std::string(pullParentPath.fullPathName().asChar()));

        UsdMayaPrimUpdaterContext context(
            srcProxyShape->getTime(), srcProxyShape->getUsdStage(), ctxArgs);
        context._pullExtras.initRecursive(Ufe::Hierarchy::createItem(srcPath));
        progressBar.advance();

        pullImport(srcPath, srcPrim, context);
        progressBar.advance();

        scopeIt.end();
        executeAdditionalCommands(context);
        progressBar.advance();

        return true;
    }
    // Copy from DG to USD
    else if (srcProxyShape == nullptr && dstProxyShape) {
        MDagPath dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(Ufe::PathString::string(srcPath));
        if (!dagPath.isValid()) {
            return false;
        }

        MayaUsd::ProgressBarScope progressBar(6, "Duplicating to USD");

        auto ctxArgs = VtDictionaryOver(userArgs, UsdMayaJobExportArgs::GetDefaultDictionary());

        const UsdStageRefPtr  dstStage = dstProxyShape->getUsdStage();
        const SdfLayerHandle& layer = dstStage->GetEditTarget().GetLayer();
        if (!layer->IsAnonymous())
            fillUserArgsFileIfEmpty(ctxArgs, layer->GetIdentifier());

        // Record all USD modifications in an undo block and item.
        UsdUfe::UsdUndoBlock undoBlock(
            &UsdUndoableItemUndoItem::create("Duplicate USD data modifications"));
        progressBar.advance();

        // We will only do copy between two data models, setting this in arguments
        // to configure the updater
        ctxArgs[UsdMayaPrimUpdaterArgsTokens->copyOperation] = true;
        UsdMayaPrimUpdaterContext context(dstProxyShape->getTime(), dstStage, ctxArgs);

        // Export out to a temporary layer.
        auto        pushExportOutput = pushExport(srcPath, dagPath.node(), context);
        const auto& srcRootPath = std::get<SdfPath>(pushExportOutput);
        if (srcRootPath.IsEmpty()) {
            return false;
        }
        progressBar.advance();

        // Copy the temporary layer contents out to the proper destination.
        const auto& srcStage = std::get<UsdStageRefPtr>(pushExportOutput);
        const auto& srcLayer = std::get<SdfLayerRefPtr>(pushExportOutput);
        const auto& editTarget = dstStage->GetEditTarget();
        const auto& dstLayer = editTarget.GetLayer();

        // Validate that the destination parent prim is valid.
        UsdPrim dstParentPrim = MayaUsd::ufe::ufePathToPrim(dstPath);
        if (!dstParentPrim.IsValid()) {
            return false;
        }
        progressBar.advance();

        // We need the parent path of the source and destination to
        // fixup the paths of the source prims we copy to their
        // destination paths.
        const SdfPath srcParentPath = srcRootPath.GetParentPath();
        const SdfPath dstParentPath = dstParentPrim.GetPath();

        CopyLayerPrimsOptions options;
        options.progressBar = &progressBar;

        CopyLayerPrimsResult copyResult = copyLayerPrims(
            srcStage,
            srcLayer,
            srcParentPath,
            dstStage,
            dstLayer,
            dstParentPath,
            { srcRootPath },
            options);

        context._pushExtras.finalize(MayaUsd::ufe::stagePath(dstStage), copyResult.renamedPaths);

        auto ufeItem = Ufe::Hierarchy::createItem(dstPath);
        if (TF_VERIFY(ufeItem)) {
            Ufe::Scene::instance().notify(Ufe::SubtreeInvalidate(ufeItem));
        }
        progressBar.advance();

        scopeIt.end();
        executeAdditionalCommands(context);
        progressBar.advance();

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

    if (LayerManager::isSaving()) {
        return;
    }

    auto proxyShapeUfePath = proxyNotice.GetProxyShape().ufePath();

    auto autoEditFn = [this, proxyShapeUfePath](
                          const UsdMayaPrimUpdaterContext& context, const UsdPrim& prim) -> bool {
        TfToken typeName = prim.GetTypeName();

        auto registryItem = UsdMayaPrimUpdaterRegistry::FindOrFallback(typeName);
        auto supports = std::get<UsdMayaPrimUpdater::Supports>(registryItem);

        if ((supports & UsdMayaPrimUpdater::Supports::AutoPull)
            != UsdMayaPrimUpdater::Supports::AutoPull)
            return false;

        const Ufe::PathSegment pathSegment = UsdUfe::usdPathToUfePathSegment(prim.GetPath());
        const Ufe::Path        path = proxyShapeUfePath + pathSegment;

        auto factory = std::get<UpdaterFactoryFn>(registryItem);
        auto updater = factory(context, MFnDependencyNode(MObject()), path);

        if (updater && updater->shouldAutoEdit()) {
            // TODO UNDO: is it okay to throw away the undo info in the change notification?
            // What could we do with it anyway?
            OpUndoItemMuting muting;
            this->editAsMaya(path);

            return true;
        }
        return false;
    };

    const UsdNotice::ObjectsChanged& notice = proxyNotice.GetNotice();

    Usd_PrimFlagsPredicate predicate = UsdPrimDefaultPredicate;

    auto stage = notice.GetStage();

    VtDictionary              userArgs;
    UsdMayaPrimUpdaterContext context(UsdTimeCode::Default(), stage, userArgs);

    for (const auto& changedPath : notice.GetResyncedPaths()) {
        UsdPrim resyncPrim = (changedPath != SdfPath::AbsoluteRootPath())
            ? stage->GetPrimAtPath(changedPath)
            : stage->GetPseudoRoot();

        UsdPrimRange range(resyncPrim, predicate);

        for (auto it = range.begin(); it != range.end(); it++) {
            const UsdPrim& prim = *it;
            if (autoEditFn(context, prim)) {
                it.PruneChildren();
            }
        }
    }

    auto changedInfoOnlyPaths = notice.GetChangedInfoOnlyPaths();
    for (auto it = changedInfoOnlyPaths.begin(), end = changedInfoOnlyPaths.end(); it != end;
         ++it) {
        const auto& changedPath = *it;
        if (changedPath.IsPrimPropertyPath()) {
            UsdPrim valueChangedPrim = stage->GetPrimAtPath(changedPath.GetPrimPath());
            if (valueChangedPrim) {
                autoEditFn(context, valueChangedPrim);
            }
        }
    }
}

PrimUpdaterManager& PrimUpdaterManager::getInstance()
{
    return TfSingleton<PrimUpdaterManager>::GetInstance();
}

MObject PrimUpdaterManager::findOrCreatePullRoot()
{
    MayaUsd::ProgressBarScope progressBar(5);

    MObject pullRoot = findPullRoot();
    if (!pullRoot.isNull()) {
        return pullRoot;
    }
    progressBar.advance();

    // No pull root in the scene, so create one.
    MDagModifier& dagMod = MDagModifierUndoItem::create("Create pull root");
    MStatus       status;
    MObject       pullRootObj = dagMod.createNode(MString("transform"), MObject::kNullObj, &status);
    if (status != MStatus::kSuccess) {
        return MObject();
    }
    progressBar.advance();
    status = dagMod.renameNode(pullRootObj, kPullRootName);
    if (status != MStatus::kSuccess) {
        return MObject();
    }
    progressBar.advance();

    if (dagMod.doIt() != MStatus::kSuccess) {
        return MObject();
    }
    progressBar.advance();

    // Hide all objects under the pull root in the Outliner so only the pulled
    // objects under a proxy shape will be shown.
    //
    // TODO UNDO: make this redoable? Pull is always redone from scratch for now, so it does not
    // look necessary.
    MFnDependencyNode pullRootFn(pullRootObj);
    UsdMayaUtil::SetHiddenInOutliner(pullRootFn, true);

    progressBar.advance();

    // As soon as we've pulled something, we must observe the scene for
    // structural changes.
#ifdef HAS_ORPHANED_NODES_MANAGER
    beginManagePulledPrims();
#endif

    return pullRootObj;
}

MObject PrimUpdaterManager::createPullParent(const Ufe::Path& pulledPath, MObject pullRoot)
{
    MayaUsd::ProgressBarScope progressBar(2);

    MDagModifier& dagMod = MDagModifierUndoItem::create("Create pull parent node");
    MStatus       status;
    MObject       pullParentObj = dagMod.createNode(MString("transform"), pullRoot, &status);
    if (status != MStatus::kSuccess) {
        return MObject::kNullObj;
    }

    // Rename the pull parent to be the name of the node plus a "Parent" suffix.
    status = dagMod.renameNode(
        pullParentObj, MString(pulledPath.back().string().c_str()) + MString("Parent"));
    progressBar.advance();

    auto ret = dagMod.doIt();
    progressBar.advance();
    return (ret == MStatus::kSuccess) ? pullParentObj : MObject::kNullObj;
}

bool PrimUpdaterManager::removePullParent(
    const MDagPath&  parentDagPath,
    const Ufe::Path& pulledPath)
{
    if (!TF_VERIFY(parentDagPath.isValid())) {
        return false;
    }

    MayaUsd::ProgressBarScope progressBar(2);
    MStatus                   status = NodeDeletionUndoItem::deleteNode(
        "Delete pull parent node", parentDagPath.fullPathName(), parentDagPath.node());
    if (status != MStatus::kSuccess)
        return false;
    progressBar.advance();

    // If the pull parent was the last child of the pull root, remove the pull
    // root as well, and null out our pull root cache.
    MObject pullRoot = findPullRoot();
    if (!pullRoot.isNull()) {
        MFnDagNode pullRootNode(pullRoot);
        auto       nbPullRootChildren = pullRootNode.childCount();
        if (nbPullRootChildren == 0) {
            status = NodeDeletionUndoItem::deleteNode(
                "Delete pull root", pullRootNode.absoluteName(), pullRoot);
            if (status != MStatus::kSuccess) {
                return false;
            }
#ifdef HAS_ORPHANED_NODES_MANAGER
            if (!TF_VERIFY(FunctionUndoItem::execute(
                    "Remove orphaned nodes manager, pulled prims flag reset",
                    [&]() {
                        endManagePulledPrims();
                        return true;
                    },
                    [&]() {
                        beginManagePulledPrims();
                        return true;
                    }))) {
                return false;
            }
#endif
        }
    }
    progressBar.advance();

    return true;
}

MDagPath PrimUpdaterManager::setupPullParent(const Ufe::Path& pulledPath, VtDictionary& args)
{
    MayaUsd::ProgressBarScope progressBar(3);

    // Record all USD modifications in an undo block and item.
    UsdUfe::UsdUndoBlock undoBlock(
        &UsdUndoableItemUndoItem::create("Setup pull parent USD data modification"));

    MObject pullRoot = findOrCreatePullRoot();
    if (pullRoot.isNull()) {
        return MDagPath();
    }
    progressBar.advance();

    auto pullParent = createPullParent(pulledPath, pullRoot);
    if (pullParent == MObject::kNullObj) {
        return MDagPath();
    }
    progressBar.advance();

    // Pull parent is not instanced, so use first path found.
    MDagPath pullParentPath;
    if (MDagPath::getAPathTo(pullParent, pullParentPath) != MStatus::kSuccess) {
        return MDagPath();
    }

    progressBar.advance();

    // Add pull parent path to import args as a string.
    args[kPullParentPathKey] = VtValue(std::string(pullParentPath.fullPathName().asChar()));

    return pullParentPath;
}

bool PrimUpdaterManager::hasPulledPrims() const
{
    MObject pullRoot = findPullRoot();
    return !pullRoot.isNull();
}

PrimUpdaterManager::PulledPrimPaths PrimUpdaterManager::getPulledPrimPaths() const
{
    PulledPrimPaths pulledPaths;

#ifdef HAS_ORPHANED_NODES_MANAGER

    if (!_orphanedNodesManager)
        return pulledPaths;

    const OrphanedNodesManager::PulledPrims& pulledPrims = _orphanedNodesManager->getPulledPrims();
    MayaUsd::TrieVisitor<OrphanedNodesManager::PullVariantInfos>::visit(
        pulledPrims,
        [&pulledPaths](const Ufe::Path& path, const OrphanedNodesManager::PulledPrimNode& node) {
            for (const OrphanedNodesManager::PullVariantInfo& info : node.data()) {
                pulledPaths.emplace_back(path, info.editedAsMayaRoot);
            }
        });

#endif

    return pulledPaths;
}

#ifdef HAS_ORPHANED_NODES_MANAGER

void PrimUpdaterManager::beginManagePulledPrims()
{
    TF_VERIFY(_orphanedNodesManager->empty());
    Ufe::Scene::instance().addObserver(_orphanedNodesManager);

    // Observe Maya so we can stop scene observation on file new or open.
    MStatus                status;
    MSceneMessage::Message msgs[] = { MSceneMessage::kBeforeNew, MSceneMessage::kBeforeOpen };
    for (auto msg : msgs) {
        _fileCbs.append(MSceneMessage::addCallback(msg, beforeNewOrOpenCallback, this, &status));
        CHECK_MSTATUS(status);
    }
}

void PrimUpdaterManager::endManagePulledPrims()
{
    TF_VERIFY(Ufe::Scene::instance().removeObserver(_orphanedNodesManager));
    auto status = MMessage::removeCallbacks(_fileCbs);
    CHECK_MSTATUS(status);
    _fileCbs.clear();
    _orphanedNodesManager->clear();
}

/*static*/
void PrimUpdaterManager::beforeNewOrOpenCallback(void* clientData)
{
    auto* pum = static_cast<PrimUpdaterManager*>(clientData);
    pum->endManagePulledPrims();
}

void PrimUpdaterManager::beginLoadSaveCallbacks()
{
    MStatus                status;
    MSceneMessage::Message msgs[] = { MSceneMessage::kAfterNew, MSceneMessage::kAfterOpen };
    for (auto msg : msgs) {
        _openSaveCbs.append(MSceneMessage::addCallback(msg, afterNewOrOpenCallback, this, &status));
        CHECK_MSTATUS(status);
    }

    _openSaveCbs.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeSave, beforeSaveCallback, this, &status));
    CHECK_MSTATUS(status);
}

void PrimUpdaterManager::endLoadSaveCallbacks()
{
    auto status = MMessage::removeCallbacks(_openSaveCbs);
    CHECK_MSTATUS(status);
    _openSaveCbs.clear();
}

/*static*/
void PrimUpdaterManager::afterNewOrOpenCallback(void* clientData)
{
    auto* pum = static_cast<PrimUpdaterManager*>(clientData);
    pum->loadOrphanedNodesManagerData();
}

/*static*/
void PrimUpdaterManager::beforeSaveCallback(void* clientData)
{
    auto* pum = static_cast<PrimUpdaterManager*>(clientData);
    pum->saveOrphanedNodesManagerData();
}

static const char* orphanedNodesManagerDynAttrName = "orphanedNodeManagerState";

void PrimUpdaterManager::loadOrphanedNodesManagerData()
{
    MObject pullRoot = findPullRoot();
    if (pullRoot.isNull())
        return;

    beginManagePulledPrims();

    if (!hasDynamicAttribute(pullRoot, orphanedNodesManagerDynAttrName))
        return;

    MString json;
    if (!getDynamicAttribute(pullRoot, orphanedNodesManagerDynAttrName, json))
        return;

    _orphanedNodesManager->restore(OrphanedNodesManager::Memento::convertFromJson(json.asChar()));
}

void PrimUpdaterManager::saveOrphanedNodesManagerData()
{
    MObject pullRoot = findPullRoot();
    if (pullRoot.isNull())
        return;

    OrphanedNodesManager::Memento memento = _orphanedNodesManager->preserve();
    const std::string             json = OrphanedNodesManager::Memento::convertToJson(memento);

    MFnDependencyNode pullRootDepNode(pullRoot);
    MStatus           status
        = setDynamicAttribute(pullRootDepNode, orphanedNodesManagerDynAttrName, json.c_str());
    CHECK_MSTATUS(status);
}

#endif

PXR_NAMESPACE_CLOSE_SCOPE
