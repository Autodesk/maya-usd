//
// Copyright 2020 Autodesk
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

#include <mayaUsd/mayaUsd.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <vector>

namespace MAYAUSD_NS_DEF {
namespace ufe {

PXR_NS::GfMatrix4d computeLocalInclusiveTransform(
    const std::vector<PXR_NS::UsdGeomXformOp>&          ops,
    std::vector<PXR_NS::UsdGeomXformOp>::const_iterator endOp,
    const PXR_NS::UsdTimeCode&                          time);

PXR_NS::GfMatrix4d computeLocalInclusiveTransform(
    const PXR_NS::UsdPrim&        prim,
    const PXR_NS::UsdGeomXformOp& op,
    const PXR_NS::UsdTimeCode&    time);

PXR_NS::GfMatrix4d computeLocalExclusiveTransform(
    const std::vector<PXR_NS::UsdGeomXformOp>&          ops,
    std::vector<PXR_NS::UsdGeomXformOp>::const_iterator endOp,
    const PXR_NS::UsdTimeCode&                          time);

PXR_NS::GfMatrix4d computeLocalExclusiveTransform(
    const PXR_NS::UsdPrim&        prim,
    const PXR_NS::UsdGeomXformOp& op,
    const PXR_NS::UsdTimeCode&    time);

std::vector<PXR_NS::UsdGeomXformOp> getOrderedXformOps(const PXR_NS::UsdPrim& prim);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
