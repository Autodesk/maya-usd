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
#pragma once

#include <mayaUsd/base/api.h>

#include <ufe/ufe.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdAttribute.h>
#endif
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/base/tf/hashset.h>
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
PXR_NS::UsdStageWeakPtr getStage(const Ufe::Path& path);

//! Return the ProxyShape node UFE path for the argument stage.
MAYAUSD_CORE_PUBLIC
Ufe::Path stagePath(PXR_NS::UsdStageWeakPtr stage);

//! Return all the USD stages.
MAYAUSD_CORE_PUBLIC
PXR_NS::TfHashSet<PXR_NS::UsdStageWeakPtr, PXR_NS::TfHash> getAllStages();

//! Get the UFE path segment corresponding to the argument USD path.
//! If an instanceIndex is provided, the path segment for a point instance with
//! that USD path and index is returned.
MAYAUSD_CORE_PUBLIC
Ufe::PathSegment usdPathToUfePathSegment(
    const PXR_NS::SdfPath& usdPath,
    int                    instanceIndex = PXR_NS::UsdImagingDelegate::ALL_INSTANCES);

//! Get the UFE path representing just the USD prim for the argument UFE path.
//! Any instance index component at the tail of the given path is removed from
//! the returned path.
MAYAUSD_CORE_PUBLIC
Ufe::Path stripInstanceIndexFromUfePath(const Ufe::Path& path);

//! Return the USD prim corresponding to the argument UFE path.
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdPrim ufePathToPrim(const Ufe::Path& path);

//! Return the instance index corresponding to the argument UFE path if it
//! represents a point instance.
//! If the given path does not represent a point instance,
//! UsdImagingDelegate::ALL_INSTANCES (-1) will be returned.
//! If the optional argument prim pointer is non-null, the USD prim
//! corresponding to the argument UFE path is returned, as per ufePathToPrim().
MAYAUSD_CORE_PUBLIC
int ufePathToInstanceIndex(const Ufe::Path& path, PXR_NS::UsdPrim* prim = nullptr);

MAYAUSD_CORE_PUBLIC
bool isRootChild(const Ufe::Path& path);

MAYAUSD_CORE_PUBLIC
UsdSceneItem::Ptr
createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName);

//! Split the source name into a base name and a numerical suffix (set to
//! 1 if absent).  Increment the numerical suffix until name is unique.
MAYAUSD_CORE_PUBLIC
std::string uniqueName(const PXR_NS::TfToken::HashSet& existingNames, std::string srcName);

//! Return a unique child name.
MAYAUSD_CORE_PUBLIC
std::string uniqueChildName(const PXR_NS::UsdPrim& parent, const std::string& name);

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
PXR_NS::MayaUsdProxyShapeBase* getProxyShape(const Ufe::Path& path);

//! Get the time along the argument path.  A gateway node (i.e. proxy shape)
//! along the path can transform Maya's time (e.g. with scale and offset).
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdTimeCode getTime(const Ufe::Path& path);

//! Return the non-default purposes of the gateway node (i.e. proxy shape)
//! along the argument path.  Only those purposes that are true are returned.
//! The default purpose is not returned, and is considered implicit.
MAYAUSD_CORE_PUBLIC
PXR_NS::TfTokenVector getProxyShapePurposes(const Ufe::Path& path);

//! Check if an attribute value is allowed to be changed.
//! \return True, if the attribute value is allowed to be edited in the stage's local Layer Stack.
MAYAUSD_CORE_PUBLIC
bool isAttributeEditAllowed(const PXR_NS::UsdAttribute& attr, std::string* errMsg = nullptr);

MAYAUSD_CORE_PUBLIC
bool isAttributeEditAllowed(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName);

#ifdef UFE_V2_FEATURES_AVAILABLE
MAYAUSD_CORE_PUBLIC
Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::SdfValueTypeName& usdType);
Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::SdrShaderPropertyConstPtr& shaderProperty);

bool vtValueFromString(
    const std::string& typeName,
    const std::string& strValue,
    PXR_NS::VtValue*   value);

bool stringFromVtValue(
    const std::string&     typeName,
    const PXR_NS::VtValue& value,
    std::string*           strValue);
#endif

//! Check if the edit target in the stage is allowed to be changed.
//! \return True, if the edit target layer in the stage is allowed to be changed
MAYAUSD_CORE_PUBLIC
bool isEditTargetLayerModifiable(
    const PXR_NS::UsdStageWeakPtr stage,
    std::string*                  errMsg = nullptr);

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
inline Ufe::Matrix4d toUfe(const PXR_NS::GfMatrix4d& src)
{
    Ufe::Matrix4d dst;
    std::memcpy(&dst.matrix[0][0], src.GetArray(), sizeof(double) * 16);
    return dst;
}

//! Copy the argument matrix into the return matrix.
inline PXR_NS::GfMatrix4d toUsd(const Ufe::Matrix4d& src)
{
    PXR_NS::GfMatrix4d dst;
    std::memcpy(dst.GetArray(), &src.matrix[0][0], sizeof(double) * 16);
    return dst;
}

//! Copy the argument vector into the return vector.
inline Ufe::Vector3d toUfe(const PXR_NS::GfVec3d& src)
{
    return Ufe::Vector3d(src[0], src[1], src[2]);
}

//! Filter a source selection by removing descendants of filterPath.
Ufe::Selection removeDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath);

//! Re-build a source selection by copying scene items that are not descendants
//! of filterPath to the destination, and re-creating the others into the
//! destination using the source scene item path.
Ufe::Selection recreateDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath);

std::vector<std::string> splitString(const std::string& str, const std::string& separators);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
