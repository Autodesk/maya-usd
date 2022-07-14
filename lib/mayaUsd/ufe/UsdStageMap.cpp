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

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <maya/MFnDagNode.h>

#include <cassert>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

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
    if (!TF_VERIFY(handle.isValid(), "Cannot get path from invalid object handle")) {
        return Ufe::Path();
    }
    return firstPath(handle.object());
}

UsdStageWeakPtr objToStage(MObject& obj)
{
    if (obj.isNull()) {
        return nullptr;
    }

    // Get the stage from the proxy shape.
    MFnDependencyNode fn(obj);
    auto              ps = dynamic_cast<MayaUsdProxyShapeBase*>(fn.userNode());
    TF_VERIFY(ps);

    return ps->getUsdStage();
}

inline Ufe::Path::Segments::size_type nbPathSegments(const Ufe::Path& path)
{
    return
#ifdef UFE_V2_FEATURES_AVAILABLE
        path.nbSegments();
#else
        path.getSegments().size();
#endif
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------

UsdStageMap g_StageMap;

//------------------------------------------------------------------------------
// UsdStageMap
//------------------------------------------------------------------------------

void UsdStageMap::addItem(const Ufe::Path& path)
{
    // We expect a path to the proxy shape node, therefore a single segment.
    auto nbSegments = nbPathSegments(path);
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

    fPathToObject[path] = proxyShape;
    fStageToObject[stage] = proxyShape;
}

UsdStageWeakPtr UsdStageMap::stage(const Ufe::Path& path)
{
    // Non-const MObject& requires an lvalue.
    auto obj = proxyShape(path);
    return objToStage(obj);
}

void UsdStageMap::fixByDoingNothing()
{
    // The first fix is to do nothing and just use the cache as-is.
}

void UsdStageMap::fixByDetectingRenamedStageProxies()
{
    // The second fix is to try to detect renamed proxy shapes and adjust
    // only those entries.

    // MObjects stay valid even when re-parented or re-named, so we can
    // scan through all the entries in the cache and validate that the current
    // DAG path to the MObject matches the key Ufe::Path for the MObject in
    // the cache. When the don't match, update fPathToObject so that the key and
    // the MObject are in sync again.
    auto pathToObject = fPathToObject;
    for (const auto& entry : pathToObject) {
        const auto& cachedPath = entry.first;
        const auto& cachedObject = entry.second;
        // Get the UFE path from the map value.
        auto newPath = cachedObject.isValid() ? firstPath(cachedObject) : Ufe::Path();
        if (newPath != cachedPath) {
            // Key is stale.  Remove it from our cache, and add the new entry.
            auto count = fPathToObject.erase(cachedPath);
            TF_AXIOM(count);
            if (!newPath.empty())
                fPathToObject[newPath] = cachedObject;
        }
    }
}

void UsdStageMap::fixByRebuildingCache()
{
    // Final attempt is to just rebuild the entire cache.
    rebuildCache();
}

MObject UsdStageMap::proxyShape(const Ufe::Path& path)
{
    // In additional to the explicit dirty system it is possible that
    // the cache is in an invalid state and needs to be refreshed. See
    // the class comment in UsdStageMap.h for details.
    // There are multiple scenarios which signal a cache refresh is required:
    // 1. A path is searched for which cannot be found. This indicates that
    //    the stage has been reparented and the new path has been used to search
    //    for the stage and the stage is not present in the cache.
    // 2. The DAG path of a cached MObject doesn't match the key path used to
    //    find that MObject. This indicates that the stage has been reparented
    //    and the old path has been used to search for the stage. In this case
    //    there is a cache hit when there should not be.
    // 3. Any case where the cached data might be indirectly invalidated.
    //    For example undoing or redoing a proxy shape deletion.
    //
    // All these scenarios happen during transient periods when notifications
    // following a change (node renaming, deletion undo or redo, etc) are
    // processed in some arbitrary order (the order depends on the  order
    // observer were registered and how their hash values). Once the dust
    // settles after the scene change, the cache will be in steady-state
    // again. In my testing, the cache gets rebuilt once, sometimes twice.
    // Since we would mark the cache dirty anyway during these event if we
    // registered callback on node deletion or undo, the performance is
    // about the same.

    const auto& singleSegmentPath
        = nbPathSegments(path) == 1 ? path : Ufe::Path(path.getSegments()[0]);

    // The various cache healing fixes in order of costs.
    // The first one does nothing.
    //
    // We did it this way with a first no-op fix to simplify the structure
    // of the for loop, if we had applied the fix at the end of the loop,
    // we would have needed a final lookup after the final element has been
    // used, which would preclude a simple for loop and required a harder
    // to follow do/while with manual iteration over the fixes.
    static const std::function<void()> fixAttempts[]
        = { [this]() { fixByDoingNothing(); },
            [this]() { fixByDetectingRenamedStageProxies(); },
            [this]() { fixByRebuildingCache(); } };

    for (const auto& fix : fixAttempts) {
        // Apply fix, the first one tried does nothing and we thus use the cache as-is.
        fix();

        auto iter = fPathToObject.find(singleSegmentPath);

        // No entry found, try the next fix and lookup again.
        if (iter == std::end(fPathToObject))
            continue;

        auto object = iter->second;

        // If the cached object itself is invalid then remove it from the map
        // and try the next fix and lookup again.
        if (!object.isValid()) {
            fPathToObject.erase(singleSegmentPath);
            continue;
        }

        auto objectPath = firstPath(object);

        // When we hit the cache and the key path doesn't match the current object path
        // we are in scenerio 2. Update the entry in fPathToObject so that the key path
        // is the current object path, then try the next fix and lookup again.
        if (objectPath != iter->first) {
            fPathToObject.erase(singleSegmentPath);
            fPathToObject[objectPath] = object;
            TF_VERIFY(std::end(fPathToObject) == fPathToObject.find(singleSegmentPath));
            continue;
        }

        // Everything is fine, return the found object.
        return object.object();
    }

    // At this point lookup failure means the object doesn't exist.
    return MObject();
}

MayaUsdProxyShapeBase* UsdStageMap::proxyShapeNode(const Ufe::Path& path)
{
    auto obj = proxyShape(path);
    if (obj.isNull()) {
        return nullptr;
    }

    MFnDependencyNode fn(obj);
    return dynamic_cast<MayaUsdProxyShapeBase*>(fn.userNode());
}

Ufe::Path UsdStageMap::path(UsdStageWeakPtr stage)
{
    rebuildIfDirty();

    // A stage is bound to a single Dag proxy shape.
    auto iter = fStageToObject.find(stage);
    if (iter != std::end(fStageToObject))
        return firstPath(iter->second);
    return Ufe::Path();
}

UsdStageMap::StageSet UsdStageMap::allStages()
{
    rebuildIfDirty();

    StageSet stages;
    for (auto it = fPathToObject.begin(); it != fPathToObject.end();
         /* no ++it here, we manually move it in the loop */) {
        const auto&      pair = *it;
        const Ufe::Path& path = pair.first;
        // Calling UsdStageMap::stage may erase the entry in fPathToObject at path.
        // Advance the iterator so that the iterator stays valid if erasure occurs.
        it++;
        // Now handle path.
        PXR_NS::UsdStageWeakPtr matchingStage = stage(path);
        // After this point pair and path cannot be safely used, they may have been erased.
        // If the object cached in fPathToObject was invalid we'll get back a nullptr.
        // Don't add nullptr to the returned StageSet.
        if (matchingStage)
            stages.insert(matchingStage);
    }
    return stages;
}

void UsdStageMap::setDirty()
{
    fPathToObject.clear();
    fStageToObject.clear();
    fDirty = true;
}

void UsdStageMap::rebuildCache()
{
    setDirty();
    rebuildIfDirty();
}

void UsdStageMap::rebuildIfDirty()
{
    if (!fDirty)
        return;

    for (const auto& psn : ProxyShapeHandler::getAllNames()) {
        addItem(Ufe::Path(Ufe::PathSegment("|world" + psn, getMayaRunTimeId(), '|')));
    }
    fDirty = false;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
