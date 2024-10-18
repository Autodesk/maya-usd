//
// Copyright 2023 Autodesk
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
#ifndef USDUFE_UFE_TRF_UTILS_H
#define USDUFE_UFE_TRF_UTILS_H

#include <usdUfe/base/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <ufe/path.h>

namespace USDUFE_NS_DEF {

//------------------------------------------------------------------------------
// Private helper functions
//------------------------------------------------------------------------------

//! Extended support for the xform operations.
PXR_NS::UsdGeomXformCommonAPI convertToCompatibleCommonAPI(const PXR_NS::UsdPrim& prim);

//------------------------------------------------------------------------------
// Operations: translate, rotate, scale, pivot
//------------------------------------------------------------------------------

//! Absolute translation of the given prim.
void translateOp(const PXR_NS::UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute rotation (degrees) of the given prim.
void rotateOp(const PXR_NS::UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute scale of the given prim.
void scaleOp(const PXR_NS::UsdPrim& prim, const Ufe::Path& path, double x, double y, double z);

//! Absolute translation of the given prim's pivot point.
void rotatePivotTranslateOp(
    const PXR_NS::UsdPrim& prim,
    const Ufe::Path&       path,
    double                 x,
    double                 y,
    double                 z);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UFE_TRF_UTILS_H
