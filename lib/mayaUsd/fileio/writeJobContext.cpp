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
#include "writeJobContext.h"

#include <mayaUsd/fileio/instancedNodeWriter.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/transformWriter.h>
#include <mayaUsd/fileio/translators/skelBindingsProcessor.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/resolverContext.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usd/usdFileFormat.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <sstream>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

inline SdfPath _GetRootOverridePath(
    const UsdMayaJobExportArgs& args,
    const SdfPath&              path,
    bool                        modelRootOverride = true,
    bool                        rootMap = true)
{
    if (!path.IsEmpty()) {
        if (modelRootOverride && !args.usdModelRootOverridePath.IsEmpty()) {
            return path.ReplacePrefix(path.GetPrefixes()[0], args.usdModelRootOverridePath);
        }

        if (rootMap && !args.rootMapFunction.IsNull()) {
            return args.rootMapFunction.MapSourceToTarget(path);
        }
    }

    return path;
}

} // anonymous namespace

/*static*/
const SdfPath& UsdMayaWriteJobContext::GetInstanceMasterBasePath()
{
    static const SdfPath INSTANCES_SCOPE_PATH("/MayaExportedInstanceSources");
    return INSTANCES_SCOPE_PATH;
}

UsdMayaWriteJobContext::UsdMayaWriteJobContext(const UsdMayaJobExportArgs& args)
    : mArgs(args)
    , _skelBindingsProcessor(new UsdMaya_SkelBindingsProcessor())
{
}

UsdMayaWriteJobContext::~UsdMayaWriteJobContext() = default;

static bool _ShouldCreatePrim(const MDagPath& dagPath, bool isMerged)
{
    MDagPath shapeDagPath = dagPath;
    if (isMerged) {
        // if we're merging transforms, then we need to look at the shape.
        shapeDagPath.extendToShape();
    }

    MStatus                 status;
    MObject                 obj = shapeDagPath.node();
    const MFnDependencyNode depFn(obj, &status);
    if (!status) {
        return false;
    }

    const std::string mayaTypeName(depFn.typeName().asChar());
    return UsdMayaPrimWriterRegistry::IsPrimless(mayaTypeName);
}

bool UsdMayaWriteJobContext::IsMergedTransform(const MDagPath& path) const
{
    if (!mArgs.mergeTransformAndShape) {
        return false;
    }

    MStatus    status;
    const bool isDagPathValid = path.isValid(&status);
    if (status != MS::kSuccess || !isDagPathValid) {
        return false;
    }

    // Only transforms are mergeable.
    if (!path.hasFn(MFn::kTransform)) {
        return false;
    }

    // If we're instancing, and the transform is instanced, then we want it
    // to stay a plain old Xform at the root of the master. Thus, we disallow
    // merging as a special case.
    // (See also: in _FindOrCreateInstanceMaster, we insert a dummy prim
    // before any bare gprims, which we can avoid for transforms by not merging
    // here.)
    if (mArgs.exportInstances) {
        MFnDagNode dagNode(path);
        if (dagNode.isInstanced(/*indirect*/ false)) {
            return false;
        }
    }

    // Any transform with multiple (non-intermediate) shapes below is
    // non-mergeable.
    // Expanded out the implementation of MDagPath::numberOfShapesDirectlyBelow() to cut down
    // the time by half.
    unsigned int numShapes = 0;
    const auto   childCount = path.childCount();
    for (auto child = decltype(childCount) { 0 }; child < childCount; ++child) {
        const auto dagObj = path.child(child);
        MFnDagNode dagNode(dagObj);
        if (dagNode.isIntermediateObject() || dagNode.isIntermediateObject()) {
            continue;
        }
        if (dagObj.hasFn(MFn::kShape)) {
            numShapes++;
            if (numShapes > 1)
                return false;
        }
    }
    if (numShapes != 1) {
        return false;
    }

    // If the node has more than one exportable child, then it is
    // non-mergeable. (I.e., we still want to collapse if it has two shapes
    // below, but one of them is an intermediate object.)
    // For efficiency reasons, since (# exportable children <= # children),
    // check the total child count first before checking whether they're
    // exportable.
    if (childCount != 1) {
        MDagPath     childDag(path);
        unsigned int numExportableChildren = 0u;
        for (unsigned int i = 0u; i < childCount; ++i) {
            childDag.push(path.child(i));
            if (_NeedToTraverse(childDag)) {
                ++numExportableChildren;
                if (numExportableChildren > 1) {
                    return false;
                }
            }
            childDag.pop();
        }
    }

    return true;
}

SdfPath UsdMayaWriteJobContext::ConvertDagToUsdPath(const MDagPath& dagPath) const
{
    SdfPath path = UsdMayaUtil::MDagPathToUsdPath(dagPath, false, mArgs.stripNamespaces);

    // If we're merging transforms and shapes and this is a shape node, then
    // write to the parent (transform) path instead.
    MDagPath parentDag(dagPath);
    parentDag.pop();

    if (IsMergedTransform(parentDag) && UsdMayaUtil::isShape(dagPath)) {
        path = path.GetParentPath();
    }

    path = _GetRootOverridePath(mArgs, path, /* modelRootOverride = */ false, /* rootMap = */ true);

    if (!mRootPrimPath.IsEmpty()) {
        // Since path is from MDagPathToUsdPath, it will always be
        // an absolute path...
        path = path.ReplacePrefix(SdfPath::AbsoluteRootPath(), mRootPrimPath);
    }
    return _GetRootOverridePath(mArgs, path, /* modelRootOverride = */ true, /* rootMap = */ false);
}

std::vector<SdfPath> UsdMayaWriteJobContext::GetAlInstanceMasterPaths() const
{
    std::vector<SdfPath> masterPaths;

    for (const auto& objToMasters : _objectsToMasterPaths) {
        // Note: the second element of the iteration value is the pair of master paths,
        //       the first of which is the true path in the stage of the instance master.
        //       See the documentation about the _ExportAndRefPaths type.
        masterPaths.emplace_back(objToMasters.second.first);
    }
    return masterPaths;
}

UsdMayaWriteJobContext::_ExportAndRefPaths
UsdMayaWriteJobContext::_GetInstanceMasterPaths(const MDagPath& instancePath) const
{
    if (!TF_VERIFY(mInstancesPrim)) {
        return _ExportAndRefPaths();
    }

    std::string fullName = instancePath.fullPathName().asChar();
    if (mArgs.stripNamespaces) {
        fullName = UsdMayaUtil::stripNamespaces(fullName);
    }

    // Trim leading pipe; it's superfluous because all Maya full paths have it.
    fullName = fullName.substr(1);
    // Avoid name conflicts with other special chars (like |:/), since
    // TfMakeValidIdentifier replaces them with underscores also.
    fullName = TfStringReplace(fullName, "_", "__");
    // This should make a valid path component.
    fullName = TfMakeValidIdentifier(fullName);

    const SdfPath path
        = _GetRootOverridePath(mArgs, mInstancesPrim.GetPath().AppendChild(TfToken(fullName)));

    // In Maya, you can directly instance gprims or transforms, but
    // UsdImaging really wants you to instance at the transform level.
    // So if this is a directly-instanced gprim, we export it one level down
    // by creating a dummy scope.
    // (See also: in IsMergedTransform, we avoid merging directly-instanced
    // transforms in order to avoid having to add the dummy scope below.)
    SdfPath exportPath;
    if (instancePath.hasFn(MFn::kTransform)) {
        // Can directly instance transforms.
        return std::make_pair(path, path);
    } else {
        // Cannot directly instance gprims, so this must be exported underneath
        // a fake scope using the gprim name.
        MFnDagNode  primNode(instancePath.node());
        std::string gprimScopeName(primNode.name().asChar());
        if (mArgs.stripNamespaces) {
            gprimScopeName = UsdMayaUtil::stripNamespaces(gprimScopeName);
        }
        gprimScopeName = TfStringReplace(gprimScopeName, "_", "__");
        gprimScopeName = TfMakeValidIdentifier(gprimScopeName);
        return std::make_pair(path.AppendChild(TfToken(gprimScopeName)), path);
    }
}

UsdMayaWriteJobContext::_ExportAndRefPaths
UsdMayaWriteJobContext::_FindOrCreateInstanceMaster(const MDagPath& instancePath)
{
    const MObjectHandle handle(instancePath.node());
    const auto          it = _objectsToMasterPaths.find(handle);
    if (it != _objectsToMasterPaths.end()) {
        return it->second;
    } else {
        MDagPathArray allInstances;
        if (!MDagPath::getAllPathsTo(instancePath.node(), allInstances)
            || (allInstances.length() == 0)) {
            TF_RUNTIME_ERROR(
                "Could not find any instances for '%s'", instancePath.fullPathName().asChar());
            _objectsToMasterPaths[handle] = _ExportAndRefPaths();
            return _ExportAndRefPaths();
        }

        // We use the DAG path of the first instance to construct the name of
        // the master.
        const _ExportAndRefPaths masterPaths = _GetInstanceMasterPaths(allInstances[0]);
        const SdfPath&           exportPath = masterPaths.first;
        const SdfPath&           referencePath = masterPaths.second;

        if (exportPath.IsEmpty()) {
            _objectsToMasterPaths[handle] = _ExportAndRefPaths();
            return _ExportAndRefPaths();
        }

        // Export the master's hierarchy.
        // Force un-instancing when exporting to avoid an infinite loop (we've
        // got to actually export the prims un-instanced somewhere at least
        // once).
        std::vector<UsdMayaPrimWriterSharedPtr> primWriters;
        CreatePrimWriterHierarchy(
            allInstances[0],
            exportPath,
            /*forceUninstance*/ true,
            /*exportRootVisibility*/ true,
            &primWriters);

        if (primWriters.empty()) {
            _objectsToMasterPaths[handle] = _ExportAndRefPaths();
            return _ExportAndRefPaths();
        }

        for (UsdMayaPrimWriterSharedPtr& primWriter : primWriters) {
            primWriter->Write(UsdTimeCode::Default());
        }

        // For proper instancing, ensure that none of the prims from
        // referencePath down to exportPath have empty type names by converting
        // prims to Xforms if necessary.
        for (UsdPrim prim = mStage->GetPrimAtPath(exportPath);
             prim && prim.GetPath().HasPrefix(referencePath);
             prim = prim.GetParent()) {
            if (prim.GetTypeName().IsEmpty()) {
                UsdGeomXform::Define(mStage, prim.GetPath());
            }
        }

        _objectsToMasterPaths[handle] = masterPaths;
        _objectsToMasterWriters[handle] = std::make_pair(
            mMayaPrimWriterList.size(), mMayaPrimWriterList.size() + primWriters.size());
        mMayaPrimWriterList.insert(
            mMayaPrimWriterList.end(), primWriters.begin(), primWriters.end());

        return masterPaths;
    }
}

bool UsdMayaWriteJobContext::_GetInstanceMasterPrimWriters(
    const MDagPath&                                          instancePath,
    std::vector<UsdMayaPrimWriterSharedPtr>::const_iterator* begin,
    std::vector<UsdMayaPrimWriterSharedPtr>::const_iterator* end) const
{
    const MObjectHandle handle(instancePath.node());
    const auto          it = _objectsToMasterWriters.find(handle);
    if (it != _objectsToMasterWriters.end()) {
        std::pair<size_t, size_t> range = it->second;
        if (range.first < mMayaPrimWriterList.size()
            && range.second <= mMayaPrimWriterList.size()) {
            *begin = mMayaPrimWriterList.cbegin() + range.first;
            *end = mMayaPrimWriterList.cbegin() + range.second;
            return true;
        }
    }

    return false;
}

bool UsdMayaWriteJobContext::_NeedToTraverse(const MDagPath& curDag) const
{
    MObject ob = curDag.node();
    // NOTE: Already skipping all intermediate objects
    // skip all intermediate nodes (and their children)
    if (UsdMayaUtil::isIntermediate(ob)) {
        return false;
    }

    if (!UsdMayaUtil::isWritable(ob)) {
        return false;
    }

    // skip nodes that aren't renderable (and their children)

    if (mArgs.excludeInvisible && !UsdMayaUtil::isRenderable(ob)) {
        return false;
    }

    if (!mArgs.exportDefaultCameras && ob.hasFn(MFn::kTransform) && curDag.length() == 1) {
        // Ignore transforms of default cameras
        MString fullPathName = curDag.fullPathName();
        if (fullPathName == "|persp" || fullPathName == "|top" || fullPathName == "|front"
            || fullPathName == "|side") {
            return false;
        }
    }

    if (!mArgs.filteredTypeIds.empty()) {
        MFnDependencyNode mfnNode(ob);
        if (mArgs.filteredTypeIds.find(mfnNode.typeId().id()) != mArgs.filteredTypeIds.end()) {
            return false;
        }
    }

    if (!_ShouldCreatePrim(curDag, mArgs.mergeTransformAndShape)) {
        // If we're not going to create a prim at curDag, then we do not need to
        // traverse.
        return false;
    }

    // In addition to check for primless, we check for user selection of export types
    if (!mArgs.excludeExportTypes.empty()) {
        MDagPath shapeDagPath = curDag;
        if (mArgs.mergeTransformAndShape) {
            // if we're merging transforms, then we need to look at the shape.
            shapeDagPath.extendToShape();
        }

        MStatus                 status;
        MObject                 obj = shapeDagPath.node();
        const MFnDependencyNode depFn(obj, &status);
        if (!status) {
            return false;
        }

        const std::string mayaTypeName(depFn.typeName().asChar());

        if (!mArgs.isExportingMeshes()) {
            if (mayaTypeName == "mesh")
                return false;
        }
        if (!mArgs.isExportingCameras()) {
            if (mayaTypeName.find("camera") != std::string::npos)
                return false;
        }
        if (!mArgs.isExportingLights()) {
            if (mayaTypeName.find("Light") != std::string::npos)
                return false;
        }
    }

    return true;
}

bool UsdMayaWriteJobContext::_OpenFile(const std::string& filename, bool append)
{
    SdfLayerRefPtr    layer;
    ArResolverContext resolverCtx = ArGetResolver().GetCurrentContext();
    if (append) {
        layer = SdfLayer::FindOrOpen(filename);
        if (!layer) {
            TF_RUNTIME_ERROR("Failed to open layer '%s' for append", filename.c_str());
            return false;
        }
    } else {
        // If we're exporting over a file that was previously imported, there
        // may still be stages in the stage cache that have that file as a root
        // layer. Overwriting that layer will trigger potentially-unnecessary
        // recomposition on those stages, so we try to clear the layer from the
        // registry by erasing any stages in the stage cache with that root
        // layer.
        UsdMayaStageCache::EraseAllStagesWithRootLayerPath(filename);

        if (SdfLayerRefPtr existingLayer = SdfLayer::Find(filename)) {
            TF_STATUS("Writing to already-open layer '%s'", filename.c_str());
            existingLayer->Clear();
            layer = existingLayer;
        } else {
            SdfLayer::FileFormatArguments args;
            args[UsdUsdFileFormatTokens->FormatArg] = mArgs.defaultUSDFormat.GetString();
            layer = SdfLayer::CreateNew(filename, args);
        }
        if (!layer) {
            TF_RUNTIME_ERROR("Failed to create layer '%s'", filename.c_str());
            return false;
        }
    }

    mStage = UsdStage::Open(layer, resolverCtx);
    if (!mStage) {
        TF_RUNTIME_ERROR("Error opening stage for '%s'", filename.c_str());
        return false;
    }

    if (!mArgs.parentScope.IsEmpty() || !mArgs.rootPrim.IsEmpty()) {
        if (!mArgs.parentScope.IsEmpty()) {
            MGlobal::displayWarning("Flag parentScope is deprecated. Please use rootPrim instead.");
            mRootPrimPath = mArgs.parentScope;
        } else {
            mRootPrimPath = mArgs.rootPrim;
        }
        // Note that we only need to create the parentScope prim if we're not
        // using a usdModelRootOverridePath - if we ARE using
        // usdModelRootOverridePath, then IT will take the name of our parent
        // scope, and will be created when we write out the model variants
        if (mArgs.usdModelRootOverridePath.IsEmpty()) {
            if (mArgs.rootPrimType == TfToken("xform") || mArgs.rootPrimType == TfToken("Xform")) {
                mRootPrimPath = UsdGeomXform::Define(mStage, mRootPrimPath).GetPrim().GetPrimPath();
            } else {
                mRootPrimPath = UsdGeomScope::Define(mStage, mRootPrimPath).GetPrim().GetPrimPath();
            }
        }
    }

    if (mArgs.exportInstances) {
        mInstancesPrim = mStage->OverridePrim(GetInstanceMasterBasePath());
    }

    return true;
}

bool UsdMayaWriteJobContext::_PostProcess()
{
    if (mArgs.exportInstances) {
        if (_objectsToMasterWriters.empty()) {
            mStage->RemovePrim(mInstancesPrim.GetPrimPath());
        } else {
            // The MayaExportedInstanceSources group should be an over and moved to the
            // end of the layer.
            mInstancesPrim.SetSpecifier(SdfSpecifierOver);

            // We need to drop down to the Sdf level to reorder root prims.
            // (Note that we want to change the actual order in the layer, and
            // not just author a reorder statement.)
            const SdfPath           instancesPrimPath = mInstancesPrim.GetPrimPath();
            SdfPrimSpecHandleVector newRootPrims;
            SdfPrimSpecHandle       instancesPrimSpec;
            for (const SdfPrimSpecHandle& rootPrimSpec : mStage->GetRootLayer()->GetRootPrims()) {
                if (rootPrimSpec->GetPath() == instancesPrimPath) {
                    instancesPrimSpec = rootPrimSpec;
                } else {
                    newRootPrims.push_back(rootPrimSpec);
                }
            }
            if (instancesPrimSpec) {
                newRootPrims.push_back(instancesPrimSpec);
                mStage->GetRootLayer()->SetRootPrims(newRootPrims);
            } else {
                TF_CODING_ERROR(
                    "Expected to find <%s> in the root prims; "
                    "was it moved or removed?",
                    instancesPrimPath.GetText());
            }
        }
    }

    if (!_skelBindingsProcessor->PostProcessSkelBindings(mStage)) {
        return false;
    }

    return true;
}

UsdMayaPrimWriterSharedPtr UsdMayaWriteJobContext::CreatePrimWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    const bool               forceUninstance)
{
    SdfPath writePath = usdPath;

    const MDagPath dagPath = UsdMayaUtil::getDagPath(depNodeFn, /* reportError = */ false);
    if (!dagPath.isValid()) {
        // This must be a DG node. usdPath must be supplied for DG nodes.
        if (writePath.IsEmpty()) {
            TF_CODING_ERROR(
                "No usdPath supplied for DG node '%s'.",
                UsdMayaUtil::GetMayaNodeName(depNodeFn.object()).c_str());
            return nullptr;
        }
    } else {
        if (dagPath.length() == 0u) {
            // This is the world root node. It can't have a prim writer.
            return nullptr;
        }

        if (writePath.IsEmpty()) {
            writePath = ConvertDagToUsdPath(dagPath);

            if (writePath.IsEmpty()) {
                if (mArgs.rootMapFunction.IsNull()) {
                    TF_CODING_ERROR(
                        "When root mapping is not set we should always have a valid write path");
                }

                return nullptr;
            }
        }

        const MFnDagNode dagNodeFn(dagPath);
        const bool       instanced = dagNodeFn.isInstanced(/* indirect = */ false);
        if (mArgs.exportInstances && instanced && !forceUninstance) {
            // Deal with instances -- we use a special internal writer for them.
            return std::make_shared<UsdMaya_InstancedNodeWriter>(dagNodeFn, writePath, *this);
        }
    }

    // This is either a DG node or a non-instanced DAG node, so try to look up
    // a writer plugin. We search through the node's type ancestors, working
    // backwards until we find a prim writer plugin.
    if (UsdMayaPrimWriterRegistry::WriterFactoryFn primWriterFactory = _FindWriter(depNodeFn)) {
        if (UsdMayaPrimWriterSharedPtr primPtr = primWriterFactory(depNodeFn, writePath, *this)) {
            // We found a registered user prim writer that handles this node
            // type, so return now.
            return primPtr;
        }
    }

    // Could not create a writer for this node.
    return nullptr;
}

UsdMayaPrimWriterRegistry::WriterFactoryFn
UsdMayaWriteJobContext::_FindWriter(const MFnDependencyNode& mayaNode)
{
    const std::string mayaNodeType(mayaNode.typeName().asChar());

    // Check if type is already cached locally.
    // If this type has multiple writers, we need to call CanExport again to determine which writer
    // to use
    auto iter = mWriterFactoryCache.find(mayaNodeType);
    if (iter != mWriterFactoryCache.end()
        && !UsdMayaPrimWriterRegistry::HasMultipleWriters(mayaNodeType)) {
        return iter->second;
    }

    // Search up the ancestor hierarchy for a writer plugin.
    const std::vector<std::string> ancestorTypes
        = UsdMayaUtil::GetAllAncestorMayaNodeTypes(mayaNodeType);
    for (auto i = ancestorTypes.rbegin(); i != ancestorTypes.rend(); ++i) {
        if (UsdMayaPrimWriterRegistry::WriterFactoryFn primWriterFactory
            = UsdMayaPrimWriterRegistry::Find(*i, mArgs, mayaNode.object())) {
            mWriterFactoryCache[mayaNodeType] = primWriterFactory;
            return primWriterFactory;
        }
    }

    // No writer found, so mark the type as unknown in the local cache.
    mWriterFactoryCache[mayaNodeType] = nullptr;
    return nullptr;
}

void UsdMayaWriteJobContext::CreatePrimWriterHierarchy(
    const MDagPath&                          rootDag,
    const SdfPath&                           rootUsdPath,
    const bool                               forceUninstance,
    const bool                               exportRootVisibility,
    std::vector<UsdMayaPrimWriterSharedPtr>* primWritersOut)
{
    if (!primWritersOut) {
        TF_CODING_ERROR("primWritersOut is null");
        return;
    }

    // The USD path of the prototype root if it were exported at its current
    // Maya location.
    const SdfPath rootComputedUsdPath = this->ConvertDagToUsdPath(rootDag);

    MItDag itDag(MItDag::kDepthFirst, MFn::kInvalid);
    itDag.reset(rootDag);
    for (; !itDag.isDone(); itDag.next()) {
        MDagPath curDagPath;
        itDag.getPath(curDagPath);

        if (!this->_NeedToTraverse(curDagPath)) {
            itDag.prune();
            continue;
        }

        // The USD path of this prototype descendant prim if it were exported
        // at its current Maya location.
        const SdfPath curComputedUsdPath = this->ConvertDagToUsdPath(curDagPath);

        SdfPath curActualUsdPath;
        if (rootUsdPath.IsEmpty()) {
            // Just use the actual computed current path.
            curActualUsdPath = curComputedUsdPath;
        } else {
            // Compute the current prim's relative path w/r/t the prototype
            // root, and use this to re-anchor it under the USD stage location
            // where we want to write out the prototype.
            const SdfPath curRelPath = curComputedUsdPath.MakeRelativePath(rootComputedUsdPath);
            curActualUsdPath = rootUsdPath.AppendPath(curRelPath);
        }

        const MFnDagNode dagNodeFn(curDagPath);

        // Currently, forceUninstance only applies to the root DAG path but not
        // to descendant nodes (i.e. nested instancing will always occur).
        // Its purpose is to allow us to do the actual write of the master.
        UsdMayaPrimWriterSharedPtr writer = this->CreatePrimWriter(
            dagNodeFn, curActualUsdPath, curDagPath == rootDag ? forceUninstance : false);
        if (!writer) {
            continue;
        }

        if (!exportRootVisibility && writer->GetUsdPath() == rootUsdPath) {
            writer->SetExportVisibility(false);
        }

        if (writer->ShouldPruneChildren()) {
            itDag.prune();
        }

        primWritersOut->push_back(writer);
    }
}

void UsdMayaWriteJobContext::MarkSkelBindings(
    const SdfPath& path,
    const SdfPath& skelPath,
    const TfToken& config)
{
    _skelBindingsProcessor->MarkBindings(path, skelPath, config);
    _skelBindingsProcessor->SetRootPrimPath(mRootPrimPath);
}

bool UsdMayaWriteJobContext::UpdateSkelBindingsWithExtent(
    const UsdStagePtr&  stage,
    const VtVec3fArray& bbox,
    const UsdTimeCode&  timeSample)
{
    return _skelBindingsProcessor->UpdateSkelRootsWithExtent(stage, bbox, timeSample);
}

PXR_NAMESPACE_CLOSE_SCOPE
