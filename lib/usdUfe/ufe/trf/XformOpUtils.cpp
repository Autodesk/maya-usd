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

#include <usdUfe/ufe/Utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/transform.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <cstring>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

namespace {
template <bool INCLUSIVE>
GfMatrix4d computeLocalTransformWithIterator(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time)
{
    // If we want the op to be included, increment the end op iterator.
    if (INCLUSIVE && endOp != ops.end()) {
        ++endOp;
    }

    // GetLocalTransformation() interface does not allow passing a begin and
    // end iterator, so copy into an argument vector.
    std::vector<UsdGeomXformOp> argOps(std::distance(ops.begin(), endOp));
    argOps.assign(ops.begin(), endOp);

    GfMatrix4d m(1);
    if (!UsdGeomXformable::GetLocalTransformation(&m, argOps, time)) {
        throw std::runtime_error("Local transformation computation failed.");
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
        std::string msg
            = TfStringPrintf("Matrix op %s not found in transform ops.", op.GetOpName().GetText());
        throw std::runtime_error(msg.c_str());
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

Ufe::Vector3d getTranslation(const Ufe::Matrix4d& m)
{
    Ufe::Vector3d t;
    extractTRS(m, &t, nullptr, nullptr);
    return t;
}

Ufe::Vector3d getRotation(const Ufe::Matrix4d& m)
{
    Ufe::Vector3d r;
    extractTRS(m, nullptr, &r, nullptr);
    return r;
}

Ufe::Vector3d getScale(const Ufe::Matrix4d& m)
{
    Ufe::Vector3d s;
    extractTRS(m, nullptr, nullptr, &s);
    return s;
}

namespace internal {

void getTRS(const Ufe::Matrix4d& m, Ufe::Vector3d* t, Ufe::Vector3d* r, Ufe::Vector3d* s)
{
    // Decompose new matrix to extract TRS.
    PXR_NS::GfMatrix4d usdMatrix;
    std::memcpy(usdMatrix[0], &m.matrix[0][0], sizeof(double) * 16);

    PXR_NS::GfTransform usdXform(usdMatrix);

    if (nullptr != t) {
        auto usdT = usdXform.GetTranslation();
        *t = Ufe::Vector3d(usdT[0], usdT[1], usdT[2]);
    }

    if (nullptr != s) {
        auto usdS = usdXform.GetScale();
        *s = Ufe::Vector3d(usdS[0], usdS[1], usdS[2]);
    }

    if (nullptr != r) {
        // Convert the rotation to Euler XYZ (when we decompose we use ZYX).
        auto gfRot = usdXform.GetRotation();
        auto eulerXYZ = gfRot.Decompose(GfVec3d::ZAxis(), GfVec3d::YAxis(), GfVec3d::XAxis());
        *r = Ufe::Vector3d(eulerXYZ[2], eulerXYZ[1], eulerXYZ[0]);
    }
}

} // namespace internal

} // namespace USDUFE_NS_DEF
