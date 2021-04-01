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

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/gf/quath.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/pointInstancer.h>

#include <maya/MAngle.h>
#include <maya/MEulerRotation.h>
#include <maya/MTransformationMatrix.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/types.h>
#else
#include <ufe/transform3d.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

/* override */
GfQuath UsdPointInstanceOrientationModifier::convertValueToUsd(const Ufe::Vector3d& ufeValue) const
{
    const MEulerRotation eulerRotation(
        MAngle(ufeValue.x(), MAngle::kDegrees).asRadians(),
        MAngle(ufeValue.y(), MAngle::kDegrees).asRadians(),
        MAngle(ufeValue.z(), MAngle::kDegrees).asRadians());

    MTransformationMatrix transformMatrix(MTransformationMatrix::identity);
    transformMatrix.rotateTo(eulerRotation);

    double real;
    double i, j, k;
    transformMatrix.getRotationQuaternion(i, j, k, real);

    return GfQuath(
        static_cast<float>(real),
        static_cast<float>(i),
        static_cast<float>(j),
        static_cast<float>(k));
}

/* override */
Ufe::Vector3d UsdPointInstanceOrientationModifier::convertValueToUfe(const GfQuath& usdValue) const
{
    MTransformationMatrix transformMatrix(MTransformationMatrix::identity);

    const GfVec3h& imaginary = usdValue.GetImaginary();
    transformMatrix.setRotationQuaternion(
        imaginary[0u], imaginary[1u], imaginary[2u], usdValue.GetReal());

    const MEulerRotation eulerRotation = transformMatrix.eulerRotation();

    return Ufe::Vector3d(
        MAngle(eulerRotation.x, MAngle::kRadians).asDegrees(),
        MAngle(eulerRotation.y, MAngle::kRadians).asDegrees(),
        MAngle(eulerRotation.z, MAngle::kRadians).asDegrees());
}

/* override */
UsdAttribute UsdPointInstanceOrientationModifier::_getAttribute() const
{
    UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return UsdAttribute();
    }

    return pointInstancer.GetOrientationsAttr();
}

/* override */
UsdAttribute UsdPointInstanceOrientationModifier::_createAttribute()
{
    UsdGeomPointInstancer pointInstancer = getPointInstancer();
    if (!pointInstancer) {
        return UsdAttribute();
    }

    return pointInstancer.CreateOrientationsAttr();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
