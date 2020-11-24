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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MDagPath.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/types.h>
#else
#include <ufe/transform3d.h>
#endif

#include <cstring> // memcpy

PXR_NAMESPACE_USING_DIRECTIVE

UFE_NS_DEF { class PathSegment; }

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

//! Get USD stage corresponding to argument Maya Dag path.
MAYAUSD_CORE_PUBLIC
UsdStageWeakPtr getStage(const Ufe::Path& path);

//! Return the ProxyShape node UFE path for the argument stage.
MAYAUSD_CORE_PUBLIC
Ufe::Path stagePath(UsdStageWeakPtr stage);

//! Get the UFE path segment corresponding to the argument USD path.
//! If an instanceIndex is provided, the path segment for a point instance with
//! that USD path and index is returned.
MAYAUSD_CORE_PUBLIC
Ufe::PathSegment usdPathToUfePathSegment(
    const SdfPath& usdPath,
    int            instanceIndex = UsdImagingDelegate::ALL_INSTANCES);

//! Get the UFE path representing just the USD prim for the argument UFE path.
//! Any instance index component at the tail of the given path is removed from
//! the returned path.
MAYAUSD_CORE_PUBLIC
Ufe::Path stripInstanceIndexFromUfePath(const Ufe::Path& path);

//! Return the USD prim corresponding to the argument UFE path.
MAYAUSD_CORE_PUBLIC
UsdPrim ufePathToPrim(const Ufe::Path& path);

MAYAUSD_CORE_PUBLIC
bool isRootChild(const Ufe::Path& path);

MAYAUSD_CORE_PUBLIC
UsdSceneItem::Ptr
createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName);

//! Split the source name into a base name and a numerical suffix (set to
//! 1 if absent).  Increment the numerical suffix until name is unique.
MAYAUSD_CORE_PUBLIC
std::string uniqueName(const TfToken::HashSet& existingNames, std::string srcName);

//! Return a unique child name.
MAYAUSD_CORE_PUBLIC
std::string uniqueChildName(const UsdPrim& parent, const std::string& name);

//! Return if a Maya node type is derived from the gateway node type.
MAYAUSD_CORE_PUBLIC
bool isAGatewayType(const std::string& mayaNodeType);

MAYAUSD_CORE_PUBLIC
Ufe::Path dagPathToUfe(const MDagPath& dagPath);

MAYAUSD_CORE_PUBLIC
Ufe::PathSegment dagPathToPathSegment(const MDagPath& dagPath);

//! Get the time along the argument path.  A gateway node (i.e. proxy shape)
//! along the path can transform Maya's time (e.g. with scale and offset).
MAYAUSD_CORE_PUBLIC
UsdTimeCode getTime(const Ufe::Path& path);

//! Send notification for data model changes
template <class T>
void sendNotification(const Ufe::SceneItem::Ptr& item, const Ufe::Path& previousPath)
{
    T notification(item, previousPath);
#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::Scene::instance().notify(notification);
#else
    Ufe::Scene::notifyObjectPathChange(notification);
#endif
}

//! Readability function to downcast a SceneItem::Ptr to a UsdSceneItem::Ptr.
inline UsdSceneItem::Ptr downcast(const Ufe::SceneItem::Ptr& item)
{
    return std::dynamic_pointer_cast<UsdSceneItem>(item);
}

//! Copy the argument matrix into the return matrix.
inline Ufe::Matrix4d toUfe(const GfMatrix4d& src)
{
    Ufe::Matrix4d dst;
    std::memcpy(&dst.matrix[0][0], src.GetArray(), sizeof(double) * 16);
    return dst;
}

//! Copy the argument matrix into the return matrix.
inline GfMatrix4d toUsd(const Ufe::Matrix4d& src)
{
    GfMatrix4d dst;
    std::memcpy(dst.GetArray(), &src.matrix[0][0], sizeof(double) * 16);
    return dst;
}

//! Copy the argument vector into the return vector.
inline Ufe::Vector3d toUfe(const GfVec3d& src) { return Ufe::Vector3d(src[0], src[1], src[2]); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
