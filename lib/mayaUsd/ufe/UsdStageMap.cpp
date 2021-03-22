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

// Assuming proxy shape nodes cannot be instanced, simply return the first path.
Ufe::Path firstPath(const MObjectHandle& handle)
{
    if (!TF_VERIFY(handle.isValid(), "Cannot get path from invalid object handle")) {
        return Ufe::Path();
    }

    MDagPath dagPath;
    auto     status = MFnDagNode(handle.object()).getPath(dagPath);
    CHECK_MSTATUS(status);
    return MayaUsd::ufe::dagPathToUfe(dagPath);
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

    // Non-const MObject& requires an lvalue.  We've just done a name-based
    // lookup of the proxy shape, so the stage cannot be null.
    auto obj = proxyShape.object();
    auto stage = objToStage(obj);
    TF_AXIOM(stage);

    fPathToObject[path] = proxyShape;
    fStageToObject[stage] = proxyShape;
}

UsdStageWeakPtr UsdStageMap::stage(const Ufe::Path& path)
{
    // Non-const MObject& requires an lvalue.
    auto obj = proxyShape(path);
    return objToStage(obj);
}

MObject UsdStageMap::proxyShape(const Ufe::Path& path)
{
    rebuildIfDirty();

    const auto& singleSegmentPath
        = nbPathSegments(path) == 1 ? path : Ufe::Path(path.getSegments()[0]);

    auto iter = fPathToObject.find(singleSegmentPath);
    if (iter != std::end(fPathToObject)) {
        return iter->second.object();
    }

    // Caches are invalidated by DG dirtying, but not by renaming or
    // reparenting.  We have not found the object in the PathToObject cache,
    // either because the cache is stale, or because the object does not exist.
    // MObjectHandle values in the caches are stable against rename or
    // reparent, so StageToObject is unchanged.  We can iterate on the
    // MObjectHandle values and refresh the stale Ufe::Path in PathToObject.
    // Iterate on a copy of the map, as we may be updating some entries.
    auto pathToObject = fPathToObject;
    for (const auto& entry : pathToObject) {
        const auto& cachedPath = entry.first;
        const auto& cachedObject = entry.second;
        // Get the UFE path from the map value.
        auto newPath = firstPath(cachedObject);
        if (newPath != cachedPath) {
            // Key is stale.  Remove it from our cache, and add the new entry.
            auto count = fPathToObject.erase(cachedPath);
            TF_AXIOM(count);
            fPathToObject[newPath] = cachedObject;
        }
    }

    // At this point the cache is rebuilt, so lookup failure means the object
    // doesn't exist.
    iter = fPathToObject.find(singleSegmentPath);
    return iter == std::end(fPathToObject) ? MObject() : iter->second.object();
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
    for (const auto& pair : fPathToObject) {
        stages.insert(stage(pair.first));
    }
    return stages;
}

void UsdStageMap::setDirty()
{
    fPathToObject.clear();
    fStageToObject.clear();
    fDirty = true;
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
