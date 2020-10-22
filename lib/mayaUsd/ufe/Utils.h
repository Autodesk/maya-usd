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

#include <ufe/path.h>
#include <ufe/pathSegment.h>
#include <ufe/scene.h>

#include <maya/MDagPath.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/base/tf/token.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

PXR_NAMESPACE_USING_DIRECTIVE

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

//! Return the USD prim corresponding to the argument UFE path.
MAYAUSD_CORE_PUBLIC
UsdPrim ufePathToPrim(const Ufe::Path& path);

MAYAUSD_CORE_PUBLIC
bool isRootChild(const Ufe::Path& path);

MAYAUSD_CORE_PUBLIC
UsdSceneItem::Ptr createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName);

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
inline
UsdSceneItem::Ptr downcast(const Ufe::SceneItem::Ptr& item) {
    return std::dynamic_pointer_cast<UsdSceneItem>(item);
}

} // namespace ufe
} // namespace MayaUsd
