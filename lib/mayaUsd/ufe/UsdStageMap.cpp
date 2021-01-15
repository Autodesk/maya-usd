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

#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <maya/MFnDagNode.h>

#include <cassert>

namespace {

MObjectHandle proxyShapeHandle(const Ufe::Path& path)
{
    // Get the MObjectHandle from the tail of the MDagPath.	 Remove the leading
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

void UsdStageMap::addItem(const Ufe::Path& path, UsdStageWeakPtr stage)
{
    // We expect a path to the proxy shape node, therefore a single segment.
    auto nbSegments =
#ifdef UFE_V2_FEATURES_AVAILABLE
        path.nbSegments();
#else
        path.getSegments().size();
#endif
    if (nbSegments != 1) {
        TF_CODING_ERROR(
            "A proxy shape node path can have only one segment, path '%s' has %lu",
            path.string().c_str(),
            nbSegments);
        return;
    }

    // Convert the tail of the UFE path to an MObjectHandle.
    auto proxyShape = proxyShapeHandle(path);
    if (!proxyShape.isValid()) {
        return;
    }

    // Could get the stage from the proxy shape object in the stage() method,
    // but since it's given here, simply store it.
    fObjectToStage[proxyShape] = stage;
    fStageToObject[stage] = proxyShape;
}

UsdStageWeakPtr UsdStageMap::stage(const Ufe::Path& path)
{
    rebuildIfDirty();

    auto proxyShape = proxyShapeHandle(path);
    if (!proxyShape.isValid()) {
        return nullptr;
    }

    // A stage is bound to a single Dag proxy shape.
    auto iter = fObjectToStage.find(proxyShape);
    if (iter != std::end(fObjectToStage))
        return iter->second;
    return nullptr;
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

void UsdStageMap::setDirty()
{
    fObjectToStage.clear();
    fStageToObject.clear();
    fDirty = true;
}

void UsdStageMap::rebuildIfDirty()
{
    if (!fDirty)
        return;

    auto proxyShapeNames = ProxyShapeHandler::getAllNames();
    for (const auto& psn : proxyShapeNames) {
        MDagPath  dag = UsdMayaUtil::nameToDagPath(psn);
        Ufe::Path ufePath = dagPathToUfe(dag);
        auto      stage = ProxyShapeHandler::dagPathToStage(psn);
        addItem(ufePath, stage);
    }
    fDirty = false;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
