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

#include "maya/MFnNurbsCurve.h"
#include "maya/MFnDoubleArrayData.h"
#include "maya/MObject.h"
#include "maya/MPlug.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/nurbsCurves.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace utils {

  extern void convert3DFloatArrayTo4DDoubleArray(const float* const input, double* const output, size_t count);
  extern void copyPoints(const MFnNurbsCurve& fnCurve, const UsdAttribute& pointsAttr,
                         UsdTimeCode time = UsdTimeCode::Default());
  extern void copyCurveVertexCounts(const MFnNurbsCurve& fnCurve, const UsdAttribute& countsAttr,
                                    UsdTimeCode time = UsdTimeCode::Default());
  extern void copyKnots(const MFnNurbsCurve& fnCurve, const UsdAttribute& knotsAttr,
                        UsdTimeCode time = UsdTimeCode::Default());
  extern void copyRanges(const MFnNurbsCurve& fnCurve, const UsdAttribute& rangesAttr,
                         UsdTimeCode time = UsdTimeCode::Default());
  extern void copyOrder(const MFnNurbsCurve& fnCurve, const UsdAttribute& orderAttr,
                        UsdTimeCode time = UsdTimeCode::Default());
  extern void copyWidths(const MObject& widthObj, const MPlug& widthPlug, const MFnDoubleArrayData& widthArray,
                         const UsdAttribute& widthsAttr, UsdTimeCode time = UsdTimeCode::Default());
  extern bool getMayaCurveWidth(const MFnNurbsCurve& fnCurve, MObject& object, MPlug& plug, MFnDoubleArrayData& array);
  extern bool createMayaCurves(MFnNurbsCurve& fnCurve, MObject& parent, const UsdGeomNurbsCurves& usdCurves,
                               bool animalSchema, bool parentUnmerged);

  //----------------------------------------------------------------------------------------------------------------------
  /// \brief  a set of bit flags that identify which nurbs curves components have changed
  //----------------------------------------------------------------------------------------------------------------------
  enum DiffNurbsCurve
  {
    kCurvePoints = 1 << 0,
    kCurveVertexCounts = 1 << 1,
    kKnots = 1 << 2,
    kRanges = 1 << 3,
    kOrder = 1 << 4,
    kWidths = 1 << 5,
    kAllNurbsCurveComponents = 0xFFFFFFFF
  };

  uint32_t diffNurbsCurve(UsdGeomNurbsCurves& usdCurves, MFnNurbsCurve& fnCurve, UsdTimeCode timeCode,
                          uint32_t exportMask = AL::usdmaya::utils::kAllNurbsCurveComponents);

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
