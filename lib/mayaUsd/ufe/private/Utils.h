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

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <ufe/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Private helper functions
//------------------------------------------------------------------------------

//! Extended support for the xform operations.
UsdGeomXformCommonAPI convertToCompatibleCommonAPI(const UsdPrim& prim);

//! Apply restriction rules on the given prim
void applyCommandRestriction(const UsdPrim& prim, const std::string& commandName);

//! Apply restriction rules on the given property
bool isAttributeEditAllowed(const PXR_NS::UsdAttribute& attr);

//------------------------------------------------------------------------------
// Operations: translate, rotate, scale, pivot
//------------------------------------------------------------------------------

//! Absolute translation of the given prim.
void translateOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute rotation (degrees) of the given prim.
void rotateOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute scale of the given prim.
void scaleOp(const UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute translation of the given prim's pivot point.
void rotatePivotTranslateOp(
    const UsdPrim&   prim,
    const Ufe::Path& path,
    double           x,
    double           y,
    double           z);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
