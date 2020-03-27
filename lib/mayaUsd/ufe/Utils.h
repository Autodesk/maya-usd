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

#include "../base/api.h"

#include "UsdSceneItem.h"

#include <ufe/path.h>
#include <ufe/pathSegment.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/base/tf/token.h>

#include <maya/MDagPath.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
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

//! Return the highest-priority layer where the prim has a def primSpec.
MAYAUSD_CORE_PUBLIC
SdfLayerHandle defPrimSpecLayer(const UsdPrim& prim);

MAYAUSD_CORE_PUBLIC
UsdSceneItem::Ptr createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName);

//! Split the source name into a base name and a numerical suffix (set to
//! 1 if absent).  Increment the numerical suffix until name is unique.
MAYAUSD_CORE_PUBLIC
std::string uniqueName(const TfToken::HashSet& existingNames, std::string srcName);

//! Return a unique child name. Parent must be a UsdSceneItem.
MAYAUSD_CORE_PUBLIC
std::string uniqueChildName(const Ufe::SceneItem::Ptr& parent, const Ufe::Path& childPath);

//! Return if a Maya node type is derived from the gateway node type.
MAYAUSD_CORE_PUBLIC
bool isAGatewayType(const std::string& mayaNodeType);

MAYAUSD_CORE_PUBLIC
Ufe::Path dagPathToUfe(const MDagPath& dagPath);

MAYAUSD_CORE_PUBLIC
Ufe::PathSegment dagPathToPathSegment(const MDagPath& dagPath);

MAYAUSD_CORE_PUBLIC
MDagPath nameToDagPath(const std::string& name);

//! Get the time along the argument path.  A gateway node (i.e. proxy shape)
//! along the path can transform Maya's time (e.g. with scale and offset).
MAYAUSD_CORE_PUBLIC
UsdTimeCode getTime(const Ufe::Path& path);

} // namespace ufe
} // namespace MayaUsd
