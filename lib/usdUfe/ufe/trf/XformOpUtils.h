//
// Copyright 2024 Autodesk
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
#ifndef USDUFE_XFORMOPUTILS_H
#define USDUFE_XFORMOPUTILS_H

#include <usdUfe/base/api.h>
#include <usdUfe/usdUfe.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformOp.h>

// Ufe::Vector3d is a typedef.
#include <ufe/types.h>

#include <vector>

namespace USDUFE_NS_DEF {

USDUFE_PUBLIC
PXR_NS::GfMatrix4d computeLocalInclusiveTransform(
    const std::vector<PXR_NS::UsdGeomXformOp>&          ops,
    std::vector<PXR_NS::UsdGeomXformOp>::const_iterator endOp,
    const PXR_NS::UsdTimeCode&                          time);

USDUFE_PUBLIC
PXR_NS::GfMatrix4d computeLocalInclusiveTransform(
    const PXR_NS::UsdPrim&        prim,
    const PXR_NS::UsdGeomXformOp& op,
    const PXR_NS::UsdTimeCode&    time);

USDUFE_PUBLIC
PXR_NS::GfMatrix4d computeLocalExclusiveTransform(
    const std::vector<PXR_NS::UsdGeomXformOp>&          ops,
    std::vector<PXR_NS::UsdGeomXformOp>::const_iterator endOp,
    const PXR_NS::UsdTimeCode&                          time);

USDUFE_PUBLIC
PXR_NS::GfMatrix4d computeLocalExclusiveTransform(
    const PXR_NS::UsdPrim&        prim,
    const PXR_NS::UsdGeomXformOp& op,
    const PXR_NS::UsdTimeCode&    time);

USDUFE_PUBLIC
std::vector<PXR_NS::UsdGeomXformOp> getOrderedXformOps(const PXR_NS::UsdPrim& prim);

USDUFE_PUBLIC
Ufe::Vector3d getTranslation(const Ufe::Matrix4d& m);

// Rotation order is XYZ, as per UFE convention.
USDUFE_PUBLIC
Ufe::Vector3d getRotation(const Ufe::Matrix4d& m);

USDUFE_PUBLIC
Ufe::Vector3d getScale(const Ufe::Matrix4d& m);

namespace internal {

/*! Decompose the argument matrix m into translation, rotation and scale
    components using the USD API.

    \note Should not directly call this method as its the default implementation
          which can be overridden by DCC. Instead use the method "extractTRS".

    \param m Input matrix.
    \param[out] t Output translation.  If null, will be ignored.
    \param[out] r Output rotation, in XYZ order.  If null, will be ignored.
    \param[out] s Output scale.  If null, will be ignored.
*/
void getTRS(const Ufe::Matrix4d& m, Ufe::Vector3d* t, Ufe::Vector3d* r, Ufe::Vector3d* s);

} // namespace internal

} // namespace USDUFE_NS_DEF

#endif // USDUFE_XFORMOPUTILS_H
