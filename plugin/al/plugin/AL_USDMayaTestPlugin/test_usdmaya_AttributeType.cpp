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
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test USD to attribute enum mappings
//----------------------------------------------------------------------------------------------------------------------
TEST(usdmaya_AttributeType, enums)
{
    // value types
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kBool,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Bool));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUChar,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->UChar));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->UInt));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt64,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int64));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt64,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->UInt64));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kHalf,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Half));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFloat,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Float));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kDouble,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Double));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kString,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->String));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix2d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Matrix2d));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Matrix3d));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix4d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Matrix4d));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuath,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Quath));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatf,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Quatf));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatd,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Quatd));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Half2));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Half3));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Half4));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Float2));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Float3));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Float4));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Double2));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Double3));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Double4));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2i,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int2));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3i,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int3));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4i,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int4));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Point3h));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Point3f));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Point3d));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Normal3h));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Normal3f));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Normal3d));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Color3h));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Color3f));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Color3d));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFrame4d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Frame4d));

    // array types
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kBool,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->BoolArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUChar,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->UCharArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->IntArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->UIntArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt64,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int64Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt64,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->UInt64Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kHalf,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->HalfArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFloat,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->FloatArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kDouble,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->DoubleArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kString,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->StringArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix2d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Matrix2dArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Matrix3dArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix4d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Matrix4dArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuath,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->QuathArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatf,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->QuatfArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatd,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->QuatdArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Half2Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Half3Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Half4Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Float2Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Float3Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Float4Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Double2Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Double3Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Double4Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2i,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int2Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3i,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int3Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4i,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Int4Array));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Point3hArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Point3fArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Point3dArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Normal3hArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Normal3fArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Normal3dArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Color3hArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Color3fArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Color3dArray));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFrame4d,
        AL::usdmaya::utils::getAttributeType(SdfValueTypeNames->Frame4dArray));
}

TEST(usdmaya_AttributeType, attributes)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdGeomXform   xform = UsdGeomXform::Define(stage, SdfPath("/hello"));
    UsdPrim        prim = xform.GetPrim();

    // value types
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kBool,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr1"), SdfValueTypeNames->Bool)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUChar,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr2"), SdfValueTypeNames->UChar)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr3"), SdfValueTypeNames->Int)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr4"), SdfValueTypeNames->UInt)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt64,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr5"), SdfValueTypeNames->Int64)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt64,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr6"), SdfValueTypeNames->UInt64)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kHalf,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr7"), SdfValueTypeNames->Half)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFloat,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr8"), SdfValueTypeNames->Float)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kDouble,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr9"), SdfValueTypeNames->Double)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kString,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr10"), SdfValueTypeNames->String)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix2d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr11"), SdfValueTypeNames->Matrix2d)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr12"), SdfValueTypeNames->Matrix3d)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix4d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr13"), SdfValueTypeNames->Matrix4d)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuath,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr14"), SdfValueTypeNames->Quath)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatf,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr15"), SdfValueTypeNames->Quatf)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatd,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr16"), SdfValueTypeNames->Quatd)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr17"), SdfValueTypeNames->Half2)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr18"), SdfValueTypeNames->Half3)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr19"), SdfValueTypeNames->Half4)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr20"), SdfValueTypeNames->Float2)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr21"), SdfValueTypeNames->Float3)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr22"), SdfValueTypeNames->Float4)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr23"), SdfValueTypeNames->Double2)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr24"), SdfValueTypeNames->Double3)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr25"), SdfValueTypeNames->Double4)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2i,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr26"), SdfValueTypeNames->Int2)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3i,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr27"), SdfValueTypeNames->Int3)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4i,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr28"), SdfValueTypeNames->Int4)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr29"), SdfValueTypeNames->Point3h)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr30"), SdfValueTypeNames->Point3f)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr31"), SdfValueTypeNames->Point3d)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr32"), SdfValueTypeNames->Normal3h)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr33"), SdfValueTypeNames->Normal3f)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr34"), SdfValueTypeNames->Normal3d)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr35"), SdfValueTypeNames->Color3h)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr36"), SdfValueTypeNames->Color3f)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr37"), SdfValueTypeNames->Color3d)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFrame4d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("attr38"), SdfValueTypeNames->Frame4d)));

    // array types
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kBool,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr1"), SdfValueTypeNames->BoolArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUChar,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr2"), SdfValueTypeNames->UCharArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr3"), SdfValueTypeNames->IntArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr4"), SdfValueTypeNames->UIntArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kInt64,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr5"), SdfValueTypeNames->Int64Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kUInt64,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr6"), SdfValueTypeNames->UInt64Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kHalf,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr7"), SdfValueTypeNames->HalfArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFloat,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr8"), SdfValueTypeNames->FloatArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kDouble,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr9"), SdfValueTypeNames->DoubleArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kString,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr10"), SdfValueTypeNames->StringArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix2d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr11"), SdfValueTypeNames->Matrix2dArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr12"), SdfValueTypeNames->Matrix3dArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kMatrix4d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr13"), SdfValueTypeNames->Matrix4dArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuath,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr14"), SdfValueTypeNames->QuathArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatf,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr15"), SdfValueTypeNames->QuatfArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kQuatd,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr16"), SdfValueTypeNames->QuatdArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr17"), SdfValueTypeNames->Half2Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr18"), SdfValueTypeNames->Half3Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr19"), SdfValueTypeNames->Half4Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr20"), SdfValueTypeNames->Float2Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr21"), SdfValueTypeNames->Float3Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr22"), SdfValueTypeNames->Float4Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr23"), SdfValueTypeNames->Double2Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr24"), SdfValueTypeNames->Double3Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr25"), SdfValueTypeNames->Double4Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec2i,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr26"), SdfValueTypeNames->Int2Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3i,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr27"), SdfValueTypeNames->Int3Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec4i,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr28"), SdfValueTypeNames->Int4Array)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr29"), SdfValueTypeNames->Point3hArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr30"), SdfValueTypeNames->Point3fArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr31"), SdfValueTypeNames->Point3dArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr32"), SdfValueTypeNames->Normal3hArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr33"), SdfValueTypeNames->Normal3fArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr34"), SdfValueTypeNames->Normal3dArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3h,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr35"), SdfValueTypeNames->Color3hArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3f,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr36"), SdfValueTypeNames->Color3fArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kVec3d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr37"), SdfValueTypeNames->Color3dArray)));
    EXPECT_EQ(
        AL::usdmaya::utils::UsdDataType::kFrame4d,
        AL::usdmaya::utils::getAttributeType(
            prim.CreateAttribute(TfToken("aattr38"), SdfValueTypeNames->Frame4dArray)));
}
