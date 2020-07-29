//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/utils/AttributeType.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>

#include <unordered_map>

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
static const std::unordered_map<size_t, UsdDataType> usdTypeHashToEnum {
    { SdfValueTypeNames->Bool.GetHash(), UsdDataType::kBool },
    { SdfValueTypeNames->UChar.GetHash(), UsdDataType::kUChar },
    { SdfValueTypeNames->Int.GetHash(), UsdDataType::kInt },
    { SdfValueTypeNames->UInt.GetHash(), UsdDataType::kUInt },
    { SdfValueTypeNames->Int64.GetHash(), UsdDataType::kInt64 },
    { SdfValueTypeNames->UInt64.GetHash(), UsdDataType::kUInt64 },
    { SdfValueTypeNames->Half.GetHash(), UsdDataType::kHalf },
    { SdfValueTypeNames->Float.GetHash(), UsdDataType::kFloat },
    { SdfValueTypeNames->Double.GetHash(), UsdDataType::kDouble },
    { SdfValueTypeNames->String.GetHash(), UsdDataType::kString },
    { SdfValueTypeNames->Token.GetHash(), UsdDataType::kToken },
    { SdfValueTypeNames->Asset.GetHash(), UsdDataType::kAsset },
    { SdfValueTypeNames->Int2.GetHash(), UsdDataType::kVec2i },
    { SdfValueTypeNames->Int3.GetHash(), UsdDataType::kVec3i },
    { SdfValueTypeNames->Int4.GetHash(), UsdDataType::kVec4i },
    { SdfValueTypeNames->Half2.GetHash(), UsdDataType::kVec2h },
    { SdfValueTypeNames->Half3.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Half4.GetHash(), UsdDataType::kVec4h },
    { SdfValueTypeNames->Float2.GetHash(), UsdDataType::kVec2f },
    { SdfValueTypeNames->Float3.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Float4.GetHash(), UsdDataType::kVec4f },
    { SdfValueTypeNames->Double2.GetHash(), UsdDataType::kVec2d },
    { SdfValueTypeNames->Double3.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Double4.GetHash(), UsdDataType::kVec4d },
    { SdfValueTypeNames->Point3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Point3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Point3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Vector3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Vector3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Vector3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Normal3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Normal3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Normal3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Color3h.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Color3f.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Color3d.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Quath.GetHash(), UsdDataType::kQuath },
    { SdfValueTypeNames->Quatf.GetHash(), UsdDataType::kQuatf },
    { SdfValueTypeNames->Quatd.GetHash(), UsdDataType::kQuatd },
    { SdfValueTypeNames->Matrix2d.GetHash(), UsdDataType::kMatrix2d },
    { SdfValueTypeNames->Matrix3d.GetHash(), UsdDataType::kMatrix3d },
    { SdfValueTypeNames->Matrix4d.GetHash(), UsdDataType::kMatrix4d },
    { SdfValueTypeNames->Frame4d.GetHash(), UsdDataType::kFrame4d },
    { SdfValueTypeNames->BoolArray.GetHash(), UsdDataType::kBool },
    { SdfValueTypeNames->UCharArray.GetHash(), UsdDataType::kUChar },
    { SdfValueTypeNames->IntArray.GetHash(), UsdDataType::kInt },
    { SdfValueTypeNames->UIntArray.GetHash(), UsdDataType::kUInt },
    { SdfValueTypeNames->Int64Array.GetHash(), UsdDataType::kInt64 },
    { SdfValueTypeNames->UInt64Array.GetHash(), UsdDataType::kUInt64 },
    { SdfValueTypeNames->HalfArray.GetHash(), UsdDataType::kHalf },
    { SdfValueTypeNames->FloatArray.GetHash(), UsdDataType::kFloat },
    { SdfValueTypeNames->DoubleArray.GetHash(), UsdDataType::kDouble },
    { SdfValueTypeNames->StringArray.GetHash(), UsdDataType::kString },
    { SdfValueTypeNames->TokenArray.GetHash(), UsdDataType::kToken },
    { SdfValueTypeNames->AssetArray.GetHash(), UsdDataType::kAsset },
    { SdfValueTypeNames->Int2Array.GetHash(), UsdDataType::kVec2i },
    { SdfValueTypeNames->Int3Array.GetHash(), UsdDataType::kVec3i },
    { SdfValueTypeNames->Int4Array.GetHash(), UsdDataType::kVec4i },
    { SdfValueTypeNames->Half2Array.GetHash(), UsdDataType::kVec2h },
    { SdfValueTypeNames->Half3Array.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Half4Array.GetHash(), UsdDataType::kVec4h },
    { SdfValueTypeNames->Float2Array.GetHash(), UsdDataType::kVec2f },
    { SdfValueTypeNames->Float3Array.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Float4Array.GetHash(), UsdDataType::kVec4f },
    { SdfValueTypeNames->Double2Array.GetHash(), UsdDataType::kVec2d },
    { SdfValueTypeNames->Double3Array.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Double4Array.GetHash(), UsdDataType::kVec4d },
    { SdfValueTypeNames->Point3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Point3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Point3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Vector3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Vector3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Vector3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Normal3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Normal3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Normal3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->Color3hArray.GetHash(), UsdDataType::kVec3h },
    { SdfValueTypeNames->Color3fArray.GetHash(), UsdDataType::kVec3f },
    { SdfValueTypeNames->Color3dArray.GetHash(), UsdDataType::kVec3d },
    { SdfValueTypeNames->QuathArray.GetHash(), UsdDataType::kQuath },
    { SdfValueTypeNames->QuatfArray.GetHash(), UsdDataType::kQuatf },
    { SdfValueTypeNames->QuatdArray.GetHash(), UsdDataType::kQuatd },
    { SdfValueTypeNames->Matrix2dArray.GetHash(), UsdDataType::kMatrix2d },
    { SdfValueTypeNames->Matrix3dArray.GetHash(), UsdDataType::kMatrix3d },
    { SdfValueTypeNames->Matrix4dArray.GetHash(), UsdDataType::kMatrix4d },
    { SdfValueTypeNames->Frame4dArray.GetHash(), UsdDataType::kFrame4d }
};

//----------------------------------------------------------------------------------------------------------------------
UsdDataType getAttributeType(const SdfValueTypeName& typeName)
{
    const auto it = usdTypeHashToEnum.find(typeName.GetHash());
    if (it == usdTypeHashToEnum.end()) {
        return UsdDataType::kUnknown;
    }
    return it->second;
}

//----------------------------------------------------------------------------------------------------------------------
UsdDataType getAttributeType(const UsdAttribute& usdAttr)
{
    if (!usdAttr.IsValid()) {
        return UsdDataType::kUnknown;
    }
    const SdfValueTypeName typeName = usdAttr.GetTypeName();
    const auto             it = usdTypeHashToEnum.find(typeName.GetHash());
    if (it == usdTypeHashToEnum.end()) {
        return UsdDataType::kUnknown;
    }
    return it->second;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
