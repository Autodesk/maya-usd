//
// Copyright 2021 Autodesk
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

#include "UsdPointInstanceOrientationModifier.h"

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/pointInstancer.h>

#include <ufe/types.h>

namespace {
// Map of point instance batches under construction, keyed on instancer path.
UsdUfe::UsdPointInstanceOrientationModifier::Batches sBatches;
} // namespace

namespace USDUFE_NS_DEF {

// Ensure that UsdPointInstanceOrientationModifier is properly setup.
using base = UsdPointInstanceModifierBase<Ufe::Vector3d, PXR_NS::GfQuath>;
USDUFE_VERIFY_CLASS_BASE(base, UsdPointInstanceOrientationModifier);
USDUFE_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(UsdPointInstanceOrientationModifier);
USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdPointInstanceOrientationModifier);

/* override */
GfQuath UsdPointInstanceOrientationModifier::convertValueToUsd(const Ufe::Vector3d& ufeValue) const
{
    // Input vector from Ufe is in degrees.
    PXR_NS::GfVec3f eulerXYZ(ufeValue.x(), ufeValue.y(), ufeValue.z());

    // Create a rotation (XYZ) from the Euler angles.
    auto rotD = PXR_NS::GfRotation(GfVec3d::XAxis(), eulerXYZ[0])
        * PXR_NS::GfRotation(GfVec3d::YAxis(), eulerXYZ[1])
        * PXR_NS::GfRotation(GfVec3d::ZAxis(), eulerXYZ[2]);

    PXR_NS::GfQuath usdQuatD(rotD.GetQuat());
    return usdQuatD;
}

/* override */
Ufe::Vector3d UsdPointInstanceOrientationModifier::convertValueToUfe(const GfQuath& usdValue) const
{
    // First create a rotation from the quaternion.
    PXR_NS::GfRotation gfRot(usdValue);

    // Convert the rotation to Euler XYZ (when we decompose we use ZYX).
    auto          eulerXYZ = gfRot.Decompose(GfVec3d::ZAxis(), GfVec3d::YAxis(), GfVec3d::XAxis());
    Ufe::Vector3d newRet(eulerXYZ[2], eulerXYZ[1], eulerXYZ[0]);
    return newRet;
}

/* override */
PXR_NS::UsdAttribute UsdPointInstanceOrientationModifier::getAttribute() const
{
    UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return PXR_NS::UsdAttribute();
    }

    return pointInstancer.GetOrientationsAttr();
}

/* override */
PXR_NS::UsdAttribute UsdPointInstanceOrientationModifier::_createAttribute()
{
    UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return PXR_NS::UsdAttribute();
    }

    return pointInstancer.CreateOrientationsAttr();
}

UsdPointInstanceOrientationModifier::Batches& UsdPointInstanceOrientationModifier::batches()
{
    return sBatches;
}

} // namespace USDUFE_NS_DEF
