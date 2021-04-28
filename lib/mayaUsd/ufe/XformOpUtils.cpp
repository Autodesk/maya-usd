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

#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>

#include <cstring>

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

    // The UsdGeomXformOp::operator==() was only added in v20.05, so
    // prior to that we need to find using a predicate. In USD
    // they define equality to be the same underlying UsdAttribute.
#if PXR_VERSION < 2005
    auto find_XFormOp = [op](const UsdGeomXformOp& x) { return x.GetAttr() == op.GetAttr(); };
    auto i = std::find_if(ops.begin(), ops.end(), find_XFormOp);
#else
    auto i = std::find(ops.begin(), ops.end(), op);
#endif

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

Ufe::Vector3d getTranslation(const Ufe::Matrix4d& m)
{
    Ufe::Vector3d t;
    getTRS(m, &t, nullptr, nullptr);
    return t;
}

Ufe::Vector3d getRotation(const Ufe::Matrix4d& m)
{
    Ufe::Vector3d r;
    getTRS(m, nullptr, &r, nullptr);
    return r;
}

Ufe::Vector3d getScale(const Ufe::Matrix4d& m)
{
    Ufe::Vector3d s;
    getTRS(m, nullptr, nullptr, &s);
    return s;
}

void getTRS(const Ufe::Matrix4d& m, Ufe::Vector3d* t, Ufe::Vector3d* r, Ufe::Vector3d* s)
{
    // Decompose new matrix to extract TRS.  Neither GfMatrix4d::Factor
    // nor GfTransform decomposition provide results that match Maya,
    // so use MTransformationMatrix.
    MMatrix mm;
    std::memcpy(mm[0], &m.matrix[0][0], sizeof(double) * 16);
    MTransformationMatrix xformM(mm);
    if (t) {
        auto tv = xformM.getTranslation(MSpace::kTransform);
        *t = Ufe::Vector3d(tv[0], tv[1], tv[2]);
    }
    if (r) {
        double rv[3];
        // We created the MTransformationMatrix with the default XYZ rotation
        // order, so rotOrder parameter is unused.
        MTransformationMatrix::RotationOrder rotOrder;
        xformM.getRotation(rv, rotOrder);
        constexpr double radToDeg = 57.295779506;
        *r = Ufe::Vector3d(rv[0] * radToDeg, rv[1] * radToDeg, rv[2] * radToDeg);
    }
    if (s) {
        double sv[3];
        xformM.getScale(sv, MSpace::kTransform);
        *s = Ufe::Vector3d(sv[0], sv[1], sv[2]);
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
