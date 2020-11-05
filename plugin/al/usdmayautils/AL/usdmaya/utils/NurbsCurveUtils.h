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

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>

#include <maya/MFnNurbsCurve.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace utils {

AL_USDMAYA_UTILS_PUBLIC
void copyPoints(
    const MFnNurbsCurve& fnCurve,
    const UsdAttribute&  pointsAttr,
    UsdTimeCode          time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyExtent(
    const MFnNurbsCurve& fnCurve,
    const UsdAttribute&  extentAttr,
    UsdTimeCode          time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyCurveVertexCounts(
    const MFnNurbsCurve& fnCurve,
    const UsdAttribute&  countsAttr,
    UsdTimeCode          time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyKnots(
    const MFnNurbsCurve& fnCurve,
    const UsdAttribute&  knotsAttr,
    UsdTimeCode          time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyRanges(
    const MFnNurbsCurve& fnCurve,
    const UsdAttribute&  rangesAttr,
    UsdTimeCode          time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyOrder(
    const MFnNurbsCurve& fnCurve,
    const UsdAttribute&  orderAttr,
    UsdTimeCode          time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyNurbsCurveBindPoseData(
    MFnNurbsCurve&      fnCurve,
    UsdGeomNurbsCurves& usdCurves,
    UsdTimeCode         time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyWidths(
    const MObject&            widthObj,
    const MPlug&              widthPlug,
    const MFnDoubleArrayData& widthArray,
    const UsdAttribute&       widthsAttr,
    UsdTimeCode               time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
void copyWidths(
    const MObject&           widthObj,
    const MPlug&             widthPlug,
    const MFnFloatArrayData& widthArray,
    const UsdAttribute&      widthsAttr,
    UsdTimeCode              time = UsdTimeCode::Default());

AL_USDMAYA_UTILS_PUBLIC
bool getMayaCurveWidth(
    const MFnNurbsCurve& fnCurve,
    MObject&             object,
    MPlug&               plug,
    MFnDoubleArrayData&  array);

AL_USDMAYA_UTILS_PUBLIC
bool getMayaCurveWidth(const MFnNurbsCurve& fnCurve, MObject& object, MPlug& plug);

AL_USDMAYA_UTILS_PUBLIC
bool createMayaCurves(
    MFnNurbsCurve&            fnCurve,
    MObject&                  parent,
    const UsdGeomNurbsCurves& usdCurves,
    bool                      parentUnmerged);

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
    kCurveExtent = 1 << 6,
    kAllNurbsCurveComponents = 0xFFFFFFFF
};

AL_USDMAYA_UTILS_PUBLIC
uint32_t diffNurbsCurve(
    UsdGeomNurbsCurves& usdCurves,
    MFnNurbsCurve&      fnCurve,
    UsdTimeCode         timeCode,
    uint32_t            exportMask = AL::usdmaya::utils::kAllNurbsCurveComponents);

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
