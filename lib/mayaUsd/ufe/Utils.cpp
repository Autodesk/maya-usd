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
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/fileio/primUpdaterManager.h>
#endif

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/tokens.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#ifdef MAYA_HAS_DISPLAY_LAYER_API
#include <maya/MFnDisplayLayer.h>
#include <maya/MFnDisplayLayerManager.h>
#endif

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObjectHandle.h>
#include <ufe/hierarchy.h>
#include <ufe/object3d.h>
#include <ufe/object3dHandler.h>
#include <ufe/pathSegment.h>
#include <ufe/pathString.h>
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>
#include <ufe/selection.h>
#include <ufe/types.h>

#include <cassert>
#include <cctype>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

constexpr auto kIllegalUFEPath = "Illegal UFE run-time path %s.";

} // anonymous namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables & macros
//------------------------------------------------------------------------------

extern Ufe::Rtid g_MayaRtid;

// Cache of Maya node types we've queried before for inheritance from the
// gateway node type.
std::unordered_map<std::string, bool> g_GatewayType;

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

UsdStageWeakPtr getStage(const Ufe::Path& path, bool rebuildCacheIfNeeded)
{
    return UsdStageMap::getInstance().stage(path, rebuildCacheIfNeeded);
}

Ufe::Path stagePath(UsdStageWeakPtr stage) { return UsdStageMap::getInstance().path(stage); }

TfHashSet<UsdStageWeakPtr, TfHash> getAllStages() { return UsdStageMap::getInstance().allStages(); }

std::vector<Ufe::Path> getAllStagesPaths() { return UsdStageMap::getInstance().allStagesPaths(); }

bool isInStagesCache(const Ufe::Path& path)
{
    return UsdStageMap::getInstance().isInStagesCache(path);
}

UsdPrim ufePathToPrim(const Ufe::Path& path)
{
    // When called we do not make any assumption on whether or not the
    // input path is valid.

    const Ufe::Path ufePrimPath = UsdUfe::stripInstanceIndexFromUfePath(path);

    const Ufe::Path::Segments& segments = ufePrimPath.getSegments();
    if (!TF_VERIFY(!segments.empty(), kIllegalUFEPath, path.string().c_str())) {
        return UsdPrim();
    }
    auto stage = getStage(Ufe::Path(segments[0]));
    if (!stage) {
        // Do not output any TF message here (such as TF_WARN). A low-level function
        // like this should not be outputting any warnings messages. It is allowed to
        // call this method with a properly composed Ufe path, but one that doesn't
        // actually point to any valid prim.
        return UsdPrim();
    }

    // If there is only a single segment in the path, it must point to the
    // proxy shape, otherwise we would not have retrieved a valid stage.
    // The second path segment is the USD path.
    return (segments.size() == 1u)
        ? stage->GetPseudoRoot()
        : stage->GetPrimAtPath(SdfPath(segments[1].string()).GetPrimPath());
}

std::string uniqueChildNameMayaStandard(
    const PXR_NS::UsdPrim& usdParent,
    const std::string&     name,
    const std::string*     excludeName)
{
    if (!usdParent.IsValid())
        return std::string();

    // See uniqueChildNameDefault() in lib\usdUfe\ufe\Utils.cpp for details.
    // Note: removed 'UsdPrimIsAbstract' from the predicate since the Maya
    //       Outliner can show class prims now.
    TfToken::HashSet allChildrenNames;
    for (auto child : usdParent.GetFilteredChildren(UsdTraverseInstanceProxies(UsdPrimIsDefined))) {
        if (excludeName != nullptr && child.GetName().GetString() == *excludeName)
            continue;
        allChildrenNames.insert(child.GetName());
    }

    // When setting unique name Maya will look at the numerical suffix of all
    // matching names and set the unique name to +1 on the greatest suffix.
    // Example: with siblings Capsule001 & Capsule006, duplicating Capsule001
    //          will set new unique name to Capsule007.
    std::string childName { name };
    if (allChildrenNames.find(TfToken(childName)) != allChildrenNames.end()) {
        childName = UsdUfe::uniqueNameMaxSuffix(allChildrenNames, childName);
    } else {
        if (excludeName == nullptr) {
            // Not renaming, check for identical bases
            std::string baseName, suffix;
            UsdUfe::splitNumericalSuffix(childName, baseName, suffix);

            bool hasMatchingBase = false;
            for (const auto& child : allChildrenNames) {
                std::string childBaseName, childSuffix;
                if (UsdUfe::splitNumericalSuffix(child.GetString(), childBaseName, childSuffix)
                    && baseName == childBaseName) {
                    hasMatchingBase = true;
                    break;
                }
            }

            if (hasMatchingBase) {
                childName = UsdUfe::uniqueNameMaxSuffix(allChildrenNames, childName);
            }
        }
    }

    return childName;
}

bool isAGatewayType(const std::string& mayaNodeType)
{
    // If we've seen this node type before, return the cached value.
    auto iter = g_GatewayType.find(mayaNodeType);
    if (iter != std::end(g_GatewayType)) {
        return iter->second;
    }

    // Note: we are calling the MEL interpreter to determine the inherited types,
    //		 but we are then caching the result. So MEL will only be called once
    //		 for each node type.
    // Not seen before, so ask Maya.
    // When the inherited flag is used, the command returns a string array containing
    // the names of all the base node types inherited by the the given node.
    MString      cmd;
    MStringArray inherited;
    bool         isInherited = false;
    cmd.format("nodeType -inherited -isTypeName ^1s", mayaNodeType.c_str());
    if (MS::kSuccess == MGlobal::executeCommand(cmd, inherited)) {
        MString gatewayNodeType(ProxyShapeHandler::gatewayNodeType().c_str());
        isInherited = inherited.indexOf(gatewayNodeType) != -1;
        g_GatewayType[mayaNodeType] = isInherited;
    }
    return isInherited;
}

Ufe::Path dagPathToUfe(const MDagPath& dagPath)
{
    // This function can only create UFE Maya scene items with a single
    // segment, as it is only given a Dag path as input.
    return Ufe::Path(dagPathToPathSegment(dagPath));
}

Ufe::PathSegment dagPathToPathSegment(const MDagPath& dagPath)
{
    MStatus status;
    // The Ufe path includes a prepended "world" that the dag path doesn't have
    size_t                       numUfeComponents = dagPath.length(&status) + 1;
    Ufe::PathSegment::Components components;
    components.resize(numUfeComponents);
    components[0] = Ufe::PathComponent("world");
    MDagPath path = dagPath; // make an editable copy

    // Pop nodes off the path string one by one, adding them to the correct
    // position in the components vector as we go. Use i>0 as the stopping
    // condition because we've already written to element 0 of the components
    // vector.
    for (int i = numUfeComponents - 1; i > 0; i--) {
        MObject node = path.node(&status);

        if (MS::kSuccess != status)
            return Ufe::PathSegment("", g_MayaRtid, '|');

        std::string componentString(MFnDependencyNode(node).name(&status).asChar());

        if (MS::kSuccess != status)
            return Ufe::PathSegment("", g_MayaRtid, '|');

        components[i] = componentString;
        path.pop(1);
    }

    return Ufe::PathSegment(std::move(components), g_MayaRtid, '|');
}

MDagPath ufeToDagPath(const Ufe::Path& ufePath)
{
    if (ufePath.runTimeId() != g_MayaRtid || ufePath.nbSegments() > 1) {
        return MDagPath();
    }
    return UsdMayaUtil::nameToDagPath(Ufe::PathString::string(ufePath));
}

bool isMayaWorldPath(const Ufe::Path& ufePath)
{
    return (ufePath.runTimeId() == g_MayaRtid && ufePath.size() == 1);
}

MayaUsdProxyShapeBase* getProxyShape(const Ufe::Path& path, bool rebuildCacheIfNeeded /*= true*/)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return nullptr;
    }

    MayaUsdProxyShapeBase* result
        = UsdStageMap::getInstance().proxyShapeNode(path, rebuildCacheIfNeeded);
    return result;
}

SdfPath getProxyShapePrimPath(const Ufe::Path& path)
{
    if (auto proxyShape = getProxyShape(path)) {
        return proxyShape->getPrimPath();
    }
    // No proxy shape.  Just default to the empty path.
    return SdfPath::AbsoluteRootPath();
}

UsdTimeCode getTime(const Ufe::Path& path)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return UsdTimeCode::Default();
    }

    // Proxy shape node should not be null.
    auto proxyShape = UsdStageMap::getInstance().proxyShapeNode(path);
    if (proxyShape == nullptr) {
        return UsdTimeCode::Default();
    }

    return proxyShape->getTime();
}

TfTokenVector getProxyShapePurposes(const Ufe::Path& path)
{
    // Path should not be empty.
    if (!TF_VERIFY(!path.empty())) {
        return TfTokenVector();
    }

    // Proxy shape node should not be null.
    auto proxyShape = UsdStageMap::getInstance().proxyShapeNode(path);
    if (!TF_VERIFY(proxyShape)) {
        return TfTokenVector();
    }

    bool renderPurpose, proxyPurpose, guidePurpose;
    proxyShape->getDrawPurposeToggles(&renderPurpose, &proxyPurpose, &guidePurpose);
    TfTokenVector purposes;
    if (renderPurpose) {
        purposes.emplace_back(UsdGeomTokens->render);
    }
    if (proxyPurpose) {
        purposes.emplace_back(UsdGeomTokens->proxy);
    }
    if (guidePurpose) {
        purposes.emplace_back(UsdGeomTokens->guide);
    }

    return purposes;
}

#ifdef MAYA_HAS_DISPLAY_LAYER_API
template <typename PathType>
void handleDisplayLayer(
    const PathType&                                    displayLayerPath,
    const std::function<void(const MFnDisplayLayer&)>& handler)
{
    MFnDisplayLayerManager displayLayerManager(
        MFnDisplayLayerManager::currentDisplayLayerManager());

    MObject displayLayerObj = displayLayerManager.getLayer(displayLayerPath);
    if (displayLayerObj.hasFn(MFn::kDisplayLayer)) {
        MFnDisplayLayer displayLayer(displayLayerObj);
        // UFE display layers coming from referenced files are not yet supported in Maya
        // and their usage leads to a crash, so skip those for the time being
        if (!displayLayer.isFromReferencedFile()) {
            handler(displayLayer);
        }
    }
}
#endif

void ReplicateExtrasFromUSD::initRecursive(Ufe::SceneItem::Ptr ufeItem) const
{
    auto hier = Ufe::Hierarchy::hierarchy(ufeItem);
    if (hier) {
        // Go through the entire hierarchy
        for (auto child : hier->children()) {
            initRecursive(child);
        }
    }

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    MString displayLayerPath(Ufe::PathString::string(ufeItem->path()).c_str());
    handleDisplayLayer(displayLayerPath, [this, &ufeItem](const MFnDisplayLayer& displayLayer) {
        if (displayLayer.name() != "defaultLayer") {
            _displayLayerMap[ufeItem->path()] = displayLayer.object();
        }
    });
#endif
}

void ReplicateExtrasFromUSD::processItem(const Ufe::Path& path, const MObject& mayaObject) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Replicate display layer membership
    auto it = _displayLayerMap.find(path);
    if (it != _displayLayerMap.end() && it->second.hasFn(MFn::kDisplayLayer)) {
        MDagPath dagPath;
        if (MDagPath::getAPathTo(mayaObject, dagPath) == MStatus::kSuccess) {
            MFnDisplayLayer displayLayer(it->second);
            displayLayer.add(dagPath);

            // In case display layer membership was removed from the USD prim that we are
            // replicating, we want to restore it here to make sure that the prim will stay in
            // its display layer on DiscardEdits
            displayLayer.add(Ufe::PathString::string(path).c_str());
        }
    }
#endif
}

void ReplicateExtrasToUSD::processItem(const MDagPath& dagPath, const SdfPath& usdPath) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Populate display layer membership map

    // Since multiple dag paths may lead to a single USD path (like transform and node),
    // we have to make sure we don't overwrite a non-default layer with a default one
    bool displayLayerAssigned = false;
    auto entry = _primToLayerMap.find(usdPath);
    if (entry != _primToLayerMap.end() && entry->second.hasFn(MFn::kDisplayLayer)) {
        MFnDisplayLayer displayLayer(entry->second);
        displayLayerAssigned = (displayLayer.name() != "defaultLayer");
    }

    if (!displayLayerAssigned) {
        handleDisplayLayer(dagPath, [this, &usdPath](const MFnDisplayLayer& displayLayer) {
            _primToLayerMap[usdPath] = displayLayer.object();
        });
    }
#endif
}

void ReplicateExtrasToUSD::initRecursive(const Ufe::SceneItem::Ptr& item) const
{
    auto hier = Ufe::Hierarchy::hierarchy(item);
    if (hier) {
        // Go through the entire hierarchy.
        for (auto child : hier->children()) {
            initRecursive(child);
        }
    }

#ifdef MAYA_HAS_DISPLAY_LAYER_API
    MString displayLayerPath(Ufe::PathString::string(item->path()).c_str());
    handleDisplayLayer(displayLayerPath, [this, &item](const MFnDisplayLayer& displayLayer) {
        if (displayLayer.name() != "defaultLayer") {
            auto usdItem = downcast(item);
            if (usdItem) {
                auto prim = usdItem->prim();
                _primToLayerMap[prim.GetPath()] = displayLayer.object();
            }
        }
    });
#endif
}

void ReplicateExtrasToUSD::finalize(const Ufe::Path& stagePath, const RenamedPaths& renamed) const
{
#ifdef MAYA_HAS_DISPLAY_LAYER_API
    // Replicate display layer membership
    for (const auto& entry : _primToLayerMap) {
        if (entry.second.hasFn(MFn::kDisplayLayer)) {
            SdfPath usdPrimPath = entry.first;
            for (const auto& oldAndNew : renamed) {
                const PXR_NS::SdfPath& oldPrefix = oldAndNew.first;
                if (!usdPrimPath.HasPrefix(oldPrefix))
                    continue;

                const PXR_NS::SdfPath& newPrefix = oldAndNew.second;
                usdPrimPath = usdPrimPath.ReplacePrefix(oldPrefix, newPrefix);
            }

            // Avoid trying to manipulate the virtual absolute root. It will trigger
            // exceptions which can affect Python scripts and testing.
            if (usdPrimPath.IsAbsoluteRootPath())
                continue;

            auto                primPath = UsdUfe::usdPathToUfePathSegment(usdPrimPath);
            Ufe::Path::Segments segments { stagePath.getSegments()[0], primPath };
            Ufe::Path           ufePath(std::move(segments));

            MFnDisplayLayer displayLayer(entry.second);
            displayLayer.add(Ufe::PathString::string(ufePath).c_str());
        }
    }
#endif
}

#ifdef HAS_ORPHANED_NODES_MANAGER

static Ufe::BBox3d transformBBox(const PXR_NS::GfMatrix4d& matrix, const Ufe::BBox3d& bbox)
{
    Ufe::BBox3d transformed(bbox);

    transformed.min = UsdUfe::toUfe(matrix.Transform(UsdUfe::toUsd(bbox.min)));
    transformed.max = UsdUfe::toUfe(matrix.Transform(UsdUfe::toUsd(bbox.max)));

    return transformed;
}

static Ufe::BBox3d transformBBox(const Ufe::Matrix4d& matrix, const Ufe::BBox3d& bbox)
{
    return transformBBox(UsdUfe::toUsd(matrix), bbox);
}

static Ufe::BBox3d transformBBox(Ufe::SceneItem::Ptr& item, const Ufe::BBox3d& bbox)
{
    Ufe::Transform3d::Ptr t3d = Ufe::Transform3d::transform3d(item);
    if (!t3d)
        return bbox;

    return transformBBox(t3d->matrix(), bbox);
}

static Ufe::BBox3d getTransformedBBox(const MDagPath& path)
{
    MFnDagNode node(path);

    MBoundingBox mayaBBox = node.boundingBox();
    return Ufe::BBox3d(
        Ufe::Vector3d(mayaBBox.min().x, mayaBBox.min().y, mayaBBox.min().z),
        Ufe::Vector3d(mayaBBox.max().x, mayaBBox.max().y, mayaBBox.max().z));
}

#endif /* HAS_ORPHANED_NODES_MANAGER */

Ufe::BBox3d getPulledPrimsBoundingBox(const Ufe::Path& path)
{
    Ufe::BBox3d ufeBBox;

#ifdef HAS_ORPHANED_NODES_MANAGER
    const auto& updaterMgr = PXR_NS::PrimUpdaterManager::getInstance();
    PXR_NS::PrimUpdaterManager::PulledPrimPaths pulledPaths = updaterMgr.getPulledPrimPaths();
    for (const auto& paths : pulledPaths) {
        const Ufe::Path& pulledPath = paths.first;

        if (pulledPath == path)
            continue;

        if (!pulledPath.startsWith(path))
            continue;

        // Note: Maya implementation of the Object3d UFE interface does not
        //       implement the boundingBox() function. So we ask the DAG instead.
        const MDagPath& mayaPath = paths.second;
        Ufe::BBox3d     pulledBBox = getTransformedBBox(mayaPath);

        for (auto parentPath = pulledPath.pop(); parentPath != path;
             parentPath = parentPath.pop()) {
            Ufe::SceneItem::Ptr parentItem = Ufe::Hierarchy::createItem(parentPath);
            if (!parentItem)
                continue;
            pulledBBox = transformBBox(parentItem, pulledBBox);
        }

        ufeBBox = UsdUfe::combineUfeBBox(ufeBBox, pulledBBox);
    }
#endif /* HAS_ORPHANED_NODES_MANAGER */

    return ufeBBox;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
