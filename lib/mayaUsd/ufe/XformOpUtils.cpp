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

#include "XformOpUtils.h"

#include <pxr/usd/usdGeom/xformable.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {
template <bool INCLUSIVE>
GfMatrix4d computeLocalTransformWithIterator(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time)
{
    // If we want the op to be included, increment the end op iterator.
    if (INCLUSIVE) {
        TF_AXIOM(endOp != ops.end());
        ++endOp;
    }

    // GetLocalTransformation() interface does not allow passing a begin and
    // end iterator, so copy into an argument vector.
    std::vector<UsdGeomXformOp> argOps(std::distance(ops.begin(), endOp));
    argOps.assign(ops.begin(), endOp);

    GfMatrix4d m(1);
    if (!UsdGeomXformable::GetLocalTransformation(&m, argOps, time)) {
        TF_FATAL_ERROR("Local transformation computation failed.");
    }

    return m;
}

template <bool INCLUSIVE>
GfMatrix4d
computeLocalTransformWithOp(const UsdPrim& prim, const UsdGeomXformOp& op, const UsdTimeCode& time)
{
    UsdGeomXformable xformable(prim);
    bool             unused;
    auto             ops = xformable.GetOrderedXformOps(&unused);

    auto i = std::find(ops.begin(), ops.end(), op);

    if (i == ops.end()) {
        TF_FATAL_ERROR("Matrix op %s not found in transform ops.", op.GetOpName().GetText());
    }

    return computeLocalTransformWithIterator<INCLUSIVE>(ops, i, time);
}
} // namespace

GfMatrix4d computeLocalInclusiveTransform(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time)
{
    return computeLocalTransformWithIterator<true>(ops, endOp, time);
}

GfMatrix4d computeLocalInclusiveTransform(
    const UsdPrim&        prim,
    const UsdGeomXformOp& op,
    const UsdTimeCode&    time)
{
    return computeLocalTransformWithOp<true>(prim, op, time);
}

GfMatrix4d computeLocalExclusiveTransform(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time)
{
    return computeLocalTransformWithIterator<false>(ops, endOp, time);
}

GfMatrix4d computeLocalExclusiveTransform(
    const UsdPrim&        prim,
    const UsdGeomXformOp& op,
    const UsdTimeCode&    time)
{
    return computeLocalTransformWithOp<false>(prim, op, time);
}

std::vector<UsdGeomXformOp> getOrderedXformOps(const UsdPrim& prim)
{
    UsdGeomXformable xformable(prim);
    bool             unused;
    return xformable.GetOrderedXformOps(&unused);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
