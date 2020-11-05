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

#include "AL/usdmaya/Api.h"

#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/mesh.h>

#include <cstdint>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An enum describing the type of transformation found in a UsdGeomXformOp
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
enum TransformOperation : uint8_t
{
    kTranslate = 0,
    kPivot,
    kRotatePivotTranslate,
    kRotatePivot,
    kRotate,
    kRotateAxis,
    kRotatePivotInv,
    kScalePivotTranslate,
    kScalePivot,
    kShear,
    kScale,
    kScalePivotInv,
    kPivotInv,
    kTransform,
    kUnknownOp
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Convert the textual name of a transformation operation into an easier to handle enum
/// value \param  opName \return the transform op enum \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_PUBLIC
TransformOperation xformOpToEnum(const std::string& opName);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a function to check to see if the incoming transform operations are compatible with the
/// maya transform
///         types.
/// \param  it the start of the transform operations
/// \param  end the end of the transform operations
/// \param  output a simpler set of sorted enums, which are used later as a quicker way to index the
/// transform ops. \return true if the type is compatible with maya \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_PUBLIC
bool matchesMayaProfile(
    std::vector<UsdGeomXformOp>::const_iterator it,
    std::vector<UsdGeomXformOp>::const_iterator end,
    std::vector<TransformOperation>::iterator   output);

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
