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
#pragma once

#include "AL/usdmaya/utils/Api.h"
#include "AL/usdmaya/utils/ForwardDeclares.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A generalized set of USD attribute types that enable switch statements (instead of the
/// if/else approach
///         you require when using SdfValueTypeNames).
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
enum class UsdDataType : uint32_t
{
    kBool,
    kUChar,
    kInt,
    kUInt,
    kInt64,
    kUInt64,
    kHalf,
    kFloat,
    kDouble,
    kString,
    kMatrix2d,
    kMatrix3d,
    kMatrix4d,
    kQuatd,
    kQuatf,
    kQuath,
    kVec2d,
    kVec2f,
    kVec2h,
    kVec2i,
    kVec3d,
    kVec3f,
    kVec3h,
    kVec3i,
    kVec4d,
    kVec4f,
    kVec4h,
    kVec4i,
    kToken,
    kAsset,
    kFrame4d,
    kColor3h,
    kColor3f,
    kColor3d,
    kUnknown
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A method to quickly return the data type of a USD attribute
/// \param  usdAttr the usd attribute to query
/// \return the attribute type
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
UsdDataType getAttributeType(const UsdAttribute& usdAttr);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A method to quickly return the data type of a USD attribute
/// \param  typeName the type name of the usd attribute to query
/// \return the attribute type
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
UsdDataType getAttributeType(const SdfValueTypeName& typeName);

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
