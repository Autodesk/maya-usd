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
#ifndef MAYAUSD_UFE_UTILS_H
#define MAYAUSD_UFE_UTILS_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdAttribute.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/hashset.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MDagPath.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/types.h>

#include <cstring> // memcpy
#include <vector>

UFE_NS_DEF
{
    class PathSegment;
    class Selection;
}

PXR_NAMESPACE_OPEN_SCOPE
class MayaUsdProxyShapeBase;
PXR_NAMESPACE_CLOSE_SCOPE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

//! Get USD stage corresponding to argument UFE path.
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdStageWeakPtr getStage(const Ufe::Path& path, bool rebuildCacheIfNeeded = true);

//! Return the ProxyShape node UFE path for the argument stage.
MAYAUSD_CORE_PUBLIC
Ufe::Path stagePath(PXR_NS::UsdStageWeakPtr stage);

//! Return all the USD stages.
MAYAUSD_CORE_PUBLIC
PXR_NS::TfHashSet<PXR_NS::UsdStageWeakPtr, PXR_NS::TfHash> getAllStages();

//! Return all the USD stages' paths.
MAYAUSD_CORE_PUBLIC
std::vector<Ufe::Path> getAllStagesPaths();

//! Returns true if the a stage with the given path exists, returns false otherwise.
MAYAUSD_CORE_PUBLIC
bool isInStagesCache(const Ufe::Path& path);

//! Return the USD prim corresponding to the argument UFE path.
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdPrim ufePathToPrim(const Ufe::Path& path);

//! Returns a unique child name following the Maya standard naming rules.
MAYAUSD_CORE_PUBLIC
std::string uniqueChildNameMayaStandard(const PXR_NS::UsdPrim& usdParent, const std::string& name);

//! Return if a Maya node type is derived from the gateway node type.
MAYAUSD_CORE_PUBLIC
bool isAGatewayType(const std::string& mayaNodeType);

MAYAUSD_CORE_PUBLIC
Ufe::Path dagPathToUfe(const MDagPath& dagPath);

MAYAUSD_CORE_PUBLIC
Ufe::PathSegment dagPathToPathSegment(const MDagPath& dagPath);

//! Convert a single-segment Maya UFE path to a Dag path.  If the argument path
//! is not for a Maya object, or if it has more than one segment, returns an
//! invalid MDagPath.
MAYAUSD_CORE_PUBLIC
MDagPath ufeToDagPath(const Ufe::Path& ufePath);

//! Verify if the UFE path is the Maya world (root) path.
MAYAUSD_CORE_PUBLIC
bool isMayaWorldPath(const Ufe::Path& ufePath);

//! Return the gateway node (i.e. proxy shape)
MAYAUSD_CORE_PUBLIC
PXR_NS::MayaUsdProxyShapeBase*
getProxyShape(const Ufe::Path& path, bool rebuildCacheIfNeeded = true);

//! Return the primPath for the gateway node.
//! The gateway node may specify a path that UFE should be restricted to.
//! An empty path is invalid indicates that the path is invalid (i.e. the primPath
//! string has an error or is not a prim path).
//! Otherwise, UFE should only represent paths that are ancestors or descendants
//! of the returned path.  Note, if the primPath string is empty, this will
//! SdfPath::AbsoluteRootPath() (instead of an empty SdfPath).
MAYAUSD_CORE_PUBLIC
PXR_NS::SdfPath getProxyShapePrimPath(const Ufe::Path& path);

//! Get the time along the argument path.  A gateway node (i.e. proxy shape)
//! along the path can transform Maya's time (e.g. with scale and offset).
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdTimeCode getTime(const Ufe::Path& path);

//! Return the non-default purposes of the gateway node (i.e. proxy shape)
//! along the argument path.  Only those purposes that are true are returned.
//! The default purpose is not returned, and is considered implicit.
MAYAUSD_CORE_PUBLIC
PXR_NS::TfTokenVector getProxyShapePurposes(const Ufe::Path& path);

//! Readability function to downcast a SceneItem::Ptr to a UsdSceneItem::Ptr.
inline UsdUfe::UsdSceneItem::Ptr downcast(const Ufe::SceneItem::Ptr& item)
{
    return std::dynamic_pointer_cast<UsdUfe::UsdSceneItem>(item);
}

class ReplicateExtrasFromUSD
{
public:
    // Prepares the replication operation for the subtree starting with the given scene item
    void initRecursive(Ufe::SceneItem::Ptr) const;

    // Replicates extra features from the USD item defined by 'path' to the maya object
    void processItem(const Ufe::Path& path, const MObject& mayaObject) const;

private:
    mutable std::unordered_map<Ufe::Path, MObject> _displayLayerMap;
};

class ReplicateExtrasToUSD
{
public:
    // Processes replication from a maya object defined by 'dagPath'
    // to the usd item defined by 'usdPath'
    void processItem(const MDagPath& dagPath, const PXR_NS::SdfPath& usdPath) const;

    // Prepares the replication operation for the subtree starting at the given scene item.
    void initRecursive(const Ufe::SceneItem::Ptr& item) const;

    // Finalizes the replication operation to the USD stage defined by 'stagePath'
    // with a possibility to rename the old usd prefix to a new one
    using RenamedPaths = std::map<PXR_NS::SdfPath, PXR_NS::SdfPath>;
    void finalize(const Ufe::Path& stagePath, const RenamedPaths& renamed) const;

private:
    mutable std::map<PXR_NS::SdfPath, MObject> _primToLayerMap;
};

//! Return the UFE bounding-box of all prims that are pulled for edit-as-Maya
//! under the given UFE path.
MAYAUSD_CORE_PUBLIC
Ufe::BBox3d getPulledPrimsBoundingBox(const Ufe::Path& path);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UFE_UTILS_H
