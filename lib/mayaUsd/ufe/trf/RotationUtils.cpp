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

#include "RotationUtils.h"

namespace {

inline double TO_DEG(double a) { return a * 180.0 / 3.141592654; }
inline double TO_RAD(double a) { return a * 3.141592654 / 180.0; }

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

template <MEulerRotation::RotationOrder SRC_ROT_ORDER>
Ufe::Vector3d fromRot(const PXR_NS::VtValue& value)
{
    auto v = value.Get<PXR_NS::GfVec3f>();

    MEulerRotation eulerRot(TO_RAD(v[0]), TO_RAD(v[1]), TO_RAD(v[2]), SRC_ROT_ORDER);
    eulerRot.reorderIt(MEulerRotation::kXYZ);
    return Ufe::Vector3d(TO_DEG(eulerRot.x), TO_DEG(eulerRot.y), TO_DEG(eulerRot.z));
}

template <MEulerRotation::RotationOrder DST_ROT_ORDER>
PXR_NS::VtValue toRot(double x, double y, double z)
{
    MEulerRotation eulerRot(TO_RAD(x), TO_RAD(y), TO_RAD(z), MEulerRotation::kXYZ);
    eulerRot.reorderIt(DST_ROT_ORDER);
    PXR_NS::VtValue v;
    v = PXR_NS::GfVec3f(TO_DEG(eulerRot.x), TO_DEG(eulerRot.y), TO_DEG(eulerRot.z));
    return v;
}

template Ufe::Vector3d fromRot<MEulerRotation::kXZY>(const PXR_NS::VtValue&);
template Ufe::Vector3d fromRot<MEulerRotation::kYXZ>(const PXR_NS::VtValue&);
template Ufe::Vector3d fromRot<MEulerRotation::kYZX>(const PXR_NS::VtValue&);
template Ufe::Vector3d fromRot<MEulerRotation::kZXY>(const PXR_NS::VtValue&);
template Ufe::Vector3d fromRot<MEulerRotation::kZYX>(const PXR_NS::VtValue&);

template PXR_NS::VtValue toRot<MEulerRotation::kXZY>(double, double, double);
template PXR_NS::VtValue toRot<MEulerRotation::kYXZ>(double, double, double);
template PXR_NS::VtValue toRot<MEulerRotation::kYZX>(double, double, double);
template PXR_NS::VtValue toRot<MEulerRotation::kZXY>(double, double, double);
template PXR_NS::VtValue toRot<MEulerRotation::kZYX>(double, double, double);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
