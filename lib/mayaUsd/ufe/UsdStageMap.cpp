//
// Copyright 2019 Autodesk
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
#include "UsdStageMap.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MProfiler.h>
#include <ufe/pathString.h>

#include <cassert>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

const int kUsdStageMapProfilerCategory = MProfiler::addCategory("USDStages", "USDStages");

MObjectHandle nameLookup(const Ufe::Path& path)
{
    // Get the MObjectHandle from the tail of the MDagPath.  Remove the leading
    // '|world' component.
    auto          noWorld = path.popHead().string();
    auto          dagPath = UsdMayaUtil::nameToDagPath(noWorld);
    MObjectHandle handle(dagPath.node());
    if (!handle.isValid()) {
        TF_CODING_ERROR("'%s' is not a path to a proxy shape node.", noWorld.c_str());
    }
    return handle;
}

Ufe::Path firstPath(const MObject& object)
{
    MDagPath dagPath;
    auto     status = MFnDagNode(object).getPath(dagPath);
    CHECK_MSTATUS(status);
    return MayaUsd::ufe::dagPathToUfe(dagPath);
}

// Assuming proxy shape nodes cannot be instanced, simply return the first path.
Ufe::Path firstPath(const MObjectHandle& handle)
{
    if (!handle.isValid()) {
        return Ufe::Path();
    }
    return firstPath(handle.object());
}

MayaUsdProxyShapeBase* objToProxyShape(MObject& obj)
{
    if (obj.isNull()) {
        return nullptr;
    }

    // Get the stage from the proxy shape.
    MFnDependencyNode fn(obj);
    return dynamic_cast<MayaUsdProxyShapeBase*>(fn.userNode());
}

UsdStageWeakPtr objToStage(MObject& obj)
{
    MayaUsdProxyShapeBase* ps = objToProxyShape(obj);
    if (!ps)
        return {};

    return ps->getUsdStage();
}

inline Ufe::Path toPath(const std::string& mayaPathString)
{
    return Ufe::Path(
        Ufe::PathSegment("|world" + mayaPathString, MayaUsd::ufe::getMayaRunTimeId(), '|'));
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Ensure that UsdStageMap is properly setup.
static_assert(
    !std::has_virtual_destructor<UsdStageMap>::value,
    "Class should not have virtual destructor");
MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdStageMap);

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

/* static */
UsdStageMap& UsdStageMap::getInstance()
{
    // Note: C++ guarantees thread-safe initialization of static variables
    //       declared inside functions.
    static UsdStageMap stageMap;
    return stageMap;
}

//------------------------------------------------------------------------------
// UsdStageMap
//------------------------------------------------------------------------------

UsdStageMap::UsdStageMap()
{
    MayaUsd::MayaNodeTypeObserver& shapeObserver = MayaUsdProxyShapeBase::getProxyShapesObserver();
    shapeObserver.addTypeListener(*this);
    shapeObserver.addNodeListener(*this);
}

UsdStageMap::~UsdStageMap()
{
    MayaUsd::MayaNodeTypeObserver& shapeObserver = MayaUsdProxyShapeBase::getProxyShapesObserver();
    shapeObserver.removeTypeListener(*this);
    shapeObserver.removeNodeListener(*this);
}

void UsdStageMap::addItem(const Ufe::Path& path)
{
    // We expect a path to the proxy shape node, therefore a single segment.
    auto nbSegments = path.nbSegments();
    if (nbSegments != 1) {
        TF_CODING_ERROR(
            "A proxy shape node path can have only one segment, path '%s' has %lu",
            path.string().c_str(),
            nbSegments);
        return;
    }

    // Convert the UFE path to an MObjectHandle.
    auto proxyShape = nameLookup(path);
    if (!proxyShape.isValid()) {
        return;
    }

    // If a proxy shape doesn't yet have a stage, don't add it.
    // We will add it later, when the stage is initialized
    auto obj = proxyShape.object();
    auto stage = objToStage(obj);
    if (!stage) {
        return;
    }

    _pathToObject[path] = proxyShape;
    _stageToObject[stage] = proxyShape;
}

UsdStageWeakPtr UsdStageMap::stage(const Ufe::Path& path, bool rebuildCacheIfNeeded)
{
    MProfilingScope profilingScope(
        kUsdStageMapProfilerCategory, MProfiler::kColorB_L1, "UsdStageMap::stage()");

    // Non-const MObject& requires an lvalue.
    auto obj = proxyShape(path, rebuildCacheIfNeeded);
    return objToStage(obj);
}

MObject UsdStageMap::proxyShape(const Ufe::Path& path, bool rebuildCacheIfNeeded)
{
    MProfilingScope profilingScope(
        kUsdStageMapProfilerCategory, MProfiler::kColorB_L1, "UsdStageMap::proxyShape()");

    // Optimization: if there are not proxy shape instances,
    // there is nothing that can be mapped.
    if (MayaUsdProxyShapeBase::countProxyShapeInstances() == 0)
        return MObject();

    const bool wasRebuilt = rebuildIfDirty();

    const auto& singleSegmentPath
        = path.nbSegments() == 1 ? path : Ufe::Path(path.getSegments()[0]);

    auto iter = _pathToObject.find(singleSegmentPath);

    if (rebuildCacheIfNeeded && !wasRebuilt) {
        if (iter == std::end(_pathToObject)) {
            for (const auto& psn : ProxyShapeHandler::getAllNames()) {
                auto psPath = toPath(psn);
                if (_pathToObject.find(psPath) == std::end(_pathToObject)) {
                    addItem(psPath);
                }
            }
            iter = _pathToObject.find(singleSegmentPath);
        }
    }

    if (iter == std::end(_pathToObject)) {
        TF_DEBUG(MAYAUSD_STAGEMAP).Msg("Failed to find %s\n", path.string().c_str());
        return MObject();
    }

    MObjectHandle object = iter->second;

    // If the cached object itself is invalid then remove it from the map.
    if (!object.isValid()) {
        TF_DEBUG(MAYAUSD_STAGEMAP).Msg("Found invalid object for %s\n", path.string().c_str());
        _pathToObject.erase(singleSegmentPath);
        return MObject();
    }

    Ufe::Path objectPath = firstPath(object);
    if (objectPath != iter->first) {
        // When we hit the cache but the key UFE path doesn't match the object
        // current UFE path, this indicates that the stage has been reparented
        // but the notification to update stage map has not been received yet
        // and the old path has been used to search for the stage. In this case
        // there is a cache hit when there should not be. Update the entry in
        // fPathToObject so that the key path is the current object path and
        // return an invalidobject to signify we did not find the proxy shape.
        _pathToObject.erase(singleSegmentPath);
        if (!objectPath.empty())
            _pathToObject[objectPath] = object;
        TF_VERIFY(std::end(_pathToObject) == _pathToObject.find(singleSegmentPath));
        TF_DEBUG(MAYAUSD_STAGEMAP)
            .Msg(
                "Found non-matching path %s vs %s for UFE %s\n",
                objectPath.string().c_str(),
                iter->first.string().c_str(),
                path.string().c_str());
        return MObject();
    }

    return iter->second.object();
}

MayaUsdProxyShapeBase* UsdStageMap::proxyShapeNode(const Ufe::Path& path, bool rebuildCacheIfNeeded)
{
    MProfilingScope profilingScope(
        kUsdStageMapProfilerCategory, MProfiler::kColorB_L1, "UsdStageMap::proxyShapeNode()");
    auto obj = proxyShape(path, rebuildCacheIfNeeded);
    if (obj.isNull()) {
        return nullptr;
    }

    return objToProxyShape(obj);
}

Ufe::Path UsdStageMap::path(UsdStageWeakPtr stage)
{
    MProfilingScope profilingScope(
        kUsdStageMapProfilerCategory, MProfiler::kColorB_L1, "UsdStageMap::path()");
    rebuildIfDirty();

    // A stage is bound to a single Dag proxy shape.
    auto iter = _stageToObject.find(stage);
    if (iter != std::end(_stageToObject))
        return firstPath(iter->second);
    return Ufe::Path();
}

UsdStageMap::StageSet UsdStageMap::allStages()
{
    MProfilingScope profilingScope(
        kUsdStageMapProfilerCategory, MProfiler::kColorB_L1, "UsdStageMap::allStages()");

    rebuildIfDirty();

    // We can't rely on using the cached paths to find all the stages.
    // There might have been changes made to the stages, but we might not
    // yet have received the notification(s) required to update the cache,
    // and so the cache might not have been dirtied just yet.
    // Therefore, directly query the Maya data model to get the most up-to-date
    // info. This will add any missing stages in the cache. If there are outdated
    // stages in the cache, they will be cleared on the next cache rebuild, or
    // the next time anyone queries such an outdated stage.
    StageSet stages;
    for (const auto& proxyShapeName : ProxyShapeHandler::getAllNames()) {
        auto proxyShapePath = toPath(proxyShapeName);
        auto foundStage = stage(proxyShapePath);
        if (foundStage) {
            stages.insert(foundStage);
        }
    }
    return stages;
}

std::vector<Ufe::Path> UsdStageMap::allStagesPaths()
{
    MProfilingScope profilingScope(
        kUsdStageMapProfilerCategory, MProfiler::kColorB_L1, "UsdStageMap::allStagesPaths()");

    rebuildIfDirty();

    std::vector<Ufe::Path> _stagePaths;
    for (auto it = _pathToObject.begin(); it != _pathToObject.end(); ++it) {
        const auto&      pair = *it;
        const Ufe::Path& path = pair.first;
        _stagePaths.push_back(path);
    }
    return _stagePaths;
}

bool UsdStageMap::isInStagesCache(const Ufe::Path& path)
{
    return _pathToObject.find(path) != _pathToObject.end();
}

void UsdStageMap::setDirty()
{
    _pathToObject.clear();
    _stageToObject.clear();
    _dirty = true;
}

bool UsdStageMap::rebuildIfDirty()
{
    MProfilingScope profilingScope(
        kUsdStageMapProfilerCategory, MProfiler::kColorB_L1, "UsdStageMap::rebuildIfDirty()");

    if (!_dirty)
        return false;

    for (const auto& psn : ProxyShapeHandler::getAllNames()) {
        addItem(toPath(psn));
    }

    TF_DEBUG(MAYAUSD_STAGEMAP)
        .Msg("Rebuilt stage map, found %d proxy shapes\n", int(_stageToObject.size()));
    _dirty = false;
    return true;
}

void UsdStageMap::updateProxyShapeName(
    const MayaUsdProxyShapeBase& proxyShape,
    const MString&               oldName,
    const MString&               newName)
{
    TF_DEBUG(MAYAUSD_STAGEMAP)
        .Msg("ProxyShape rename %s to %s\n", oldName.asChar(), newName.asChar());
    // Note: we could try to do more precise updates than just setting dirty,
    //       but this way we make eh cache self-correcting and rely less on
    //       which notification comes first.
    setDirty();
}

void UsdStageMap::updateProxyShapePath(
    const MayaUsdProxyShapeBase& proxyShape,
    const MDagPath&              newParentPath)
{
    TF_DEBUG(MAYAUSD_STAGEMAP)
        .Msg("ProxyShape new parent %s\n", newParentPath.partialPathName().asChar());
    // Note: we could try to do more precise updates than just setting dirty,
    //       but this way we make eh cache self-correcting and rely less on
    //       which notification comes first.
    setDirty();
}

void UsdStageMap::addProxyShapeNode(const MayaUsdProxyShapeBase& proxyShape, MObject& node)
{
    TF_DEBUG(MAYAUSD_STAGEMAP).Msg("MayaUsd proxy shape added\n");
    // Note: we could try to do more precise updates than just setting dirty,
    //       but this way we make eh cache self-correcting and rely less on
    //       which notification comes first.
    setDirty();
}

void UsdStageMap::removeProxyShapeNode(const MayaUsdProxyShapeBase& proxyShape, MObject& node)
{
    TF_DEBUG(MAYAUSD_STAGEMAP).Msg("MayaUsd proxy shape removed\n");
    // Note: we could try to do more precise updates than just setting dirty,
    //       but this way we make eh cache self-correcting and rely less on
    //       which notification comes first.
    setDirty();
}

void UsdStageMap::processNodeAdded(MObject& node)
{
    MayaUsdProxyShapeBase* proxyShape = objToProxyShape(node);
    if (!proxyShape)
        return;

    MayaUsd::MayaNodeTypeObserver& shapeObserver = MayaUsdProxyShapeBase::getProxyShapesObserver();
    MayaNodeObserver*              observer = shapeObserver.addObservedNode(node);
    if (observer)
        observer->addListener(*this);

    addProxyShapeNode(*proxyShape, node);
}

void UsdStageMap::processNodeRemoved(MObject& node)
{
    MayaUsdProxyShapeBase* proxyShape = objToProxyShape(node);
    if (!proxyShape)
        return;

    // Note: we do *not* remove the node from the set of observed node.
    //       We rely on the MayaUsdProxyShapeBase to remove itself at
    //       the right time.
    MayaNodeTypeObserver& shapeObserver = MayaUsdProxyShapeBase::getProxyShapesObserver();
    MayaNodeObserver*     observer = shapeObserver.getNodeObserver(node);
    if (observer)
        observer->removeListener(*this);

    removeProxyShapeNode(*proxyShape, node);
}

void UsdStageMap::processNodeRenamed(MObject& node, const MString& oldName)
{
    MayaUsdProxyShapeBase* proxyShape = objToProxyShape(node);
    if (!proxyShape)
        return;

    MFnDependencyNode depNode(node);
    updateProxyShapeName(*proxyShape, oldName, depNode.name());
}

void UsdStageMap::processParentAdded(MObject& node, MDagPath& /*childPath*/, MDagPath& parentPath)
{
    MayaUsdProxyShapeBase* proxyShape = objToProxyShape(node);
    if (!proxyShape)
        return;

    updateProxyShapePath(*proxyShape, parentPath);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
