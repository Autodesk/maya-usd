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

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

GfMatrix4d computeLocalInclusiveTransform(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time);

GfMatrix4d computeLocalInclusiveTransform(
    const UsdPrim&        prim,
    const UsdGeomXformOp& op,
    const UsdTimeCode&    time);

GfMatrix4d computeLocalExclusiveTransform(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time);

GfMatrix4d computeLocalExclusiveTransform(
    const UsdPrim&        prim,
    const UsdGeomXformOp& op,
    const UsdTimeCode&    time);

std::vector<UsdGeomXformOp> getOrderedXformOps(const UsdPrim& prim);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
