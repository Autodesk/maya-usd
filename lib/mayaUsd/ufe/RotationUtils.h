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

#include <pxr/base/gf/vec3d.h>
#include <pxr/base/vt/value.h>

#include <maya/MEulerRotation.h>
#include <ufe/types.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Helper functions for dealing with rotations.
//------------------------------------------------------------------------------

inline double TO_DEG(double a) { return a * 180.0 / 3.141592654; }
inline double TO_RAD(double a) { return a * 3.141592654 / 180.0; }

//----------------------------------------------------------------------
// Conversion functions from RotXYZ to all supported rotation attributes.
//----------------------------------------------------------------------

inline VtValue toXYZ(double x, double y, double z)
{
    // No rotation order conversion
    VtValue v;
    v = GfVec3f(x, y, z);
    return v;
}

// Reorder argument RotXYZ rotation.
template <MEulerRotation::RotationOrder DST_ROT_ORDER> VtValue toRot(double x, double y, double z);

constexpr auto toXZY = toRot<MEulerRotation::kXZY>;
constexpr auto toYXZ = toRot<MEulerRotation::kYXZ>;
constexpr auto toYZX = toRot<MEulerRotation::kYZX>;
constexpr auto toZXY = toRot<MEulerRotation::kZXY>;
constexpr auto toZYX = toRot<MEulerRotation::kZYX>;

// Scalar float is the proper type for single-axis rotations.
inline VtValue toX(double x, double, double)
{
    VtValue v;
    v = float(x);
    return v;
}

inline VtValue toY(double, double y, double)
{
    VtValue v;
    v = float(y);
    return v;
}

inline VtValue toZ(double, double, double z)
{
    VtValue v;
    v = float(z);
    return v;
}

//----------------------------------------------------------------------
// Conversion functions from all supported rotation attributes to RotXYZ.
//----------------------------------------------------------------------

inline Ufe::Vector3d fromXYZ(const VtValue& value)
{
    // No rotation order conversion
    auto v = value.Get<GfVec3f>();
    return Ufe::Vector3d(v[0], v[1], v[2]);
}

template <MEulerRotation::RotationOrder SRC_ROT_ORDER> Ufe::Vector3d fromRot(const VtValue& value);

constexpr auto fromXZY = fromRot<MEulerRotation::kXZY>;
constexpr auto fromYXZ = fromRot<MEulerRotation::kYXZ>;
constexpr auto fromYZX = fromRot<MEulerRotation::kYZX>;
constexpr auto fromZXY = fromRot<MEulerRotation::kZXY>;
constexpr auto fromZYX = fromRot<MEulerRotation::kZYX>;

inline Ufe::Vector3d fromX(const VtValue& value) { return Ufe::Vector3d(value.Get<float>(), 0, 0); }

inline Ufe::Vector3d fromY(const VtValue& value) { return Ufe::Vector3d(0, value.Get<float>(), 0); }

inline Ufe::Vector3d fromZ(const VtValue& value) { return Ufe::Vector3d(0, 0, value.Get<float>()); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
