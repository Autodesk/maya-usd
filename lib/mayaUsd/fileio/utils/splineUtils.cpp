//
// Copyright 2025 Autodesk, Inc. All rights reserved.
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

#include "splineUtils.h"

#if PXR_VERSION >= 2411


PXR_NAMESPACE_OPEN_SCOPE

bool UsdMayaSplineUtils::WriteUsdSplineToPlug(
    MPlug&                    plug,
    TsSpline                  spline,
    UsdMayaPrimReaderContext* context,
    const TfType&             valueType,
    const MDistance::Unit     convertToUnit)
{
    if (valueType != spline.GetValueType()) {
        TF_CODING_ERROR(
            "Unsupported type name for Spline attribute '%s': %s",
            plug.partialName().asChar(),
            valueType.GetTypeName().c_str());
        return false;
    }

    TsKnotMap knots = spline.GetKnots();
    if (knots.empty()) {
        return false;
    }

    MFnAnimCurve animFn;
    MStatus      status;
    MObject      animObj = animFn.create(plug, nullptr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false)

    unsigned int numKnots = static_cast<unsigned int>(knots.size());
    MTimeArray   timeArray(numKnots, 0.0);
    MDoubleArray valuesArray(numKnots, 0.0);
    MIntArray    tangentInTypeArray(numKnots, 0);
    MIntArray    tangentOutTypeArray(numKnots, 0);
    MIntArray    tangentsLockedArray(numKnots, 1);
    MIntArray    weightsLockedArray(numKnots, 0);
    MDoubleArray tangentInXArray(numKnots, 0.0);
    MDoubleArray tangentInYArray(numKnots, 0.0);
    MDoubleArray tangentOutXArray(numKnots, 0.0);
    MDoubleArray tangentOutYArray(numKnots, 0.0);

    unsigned int knotIdx = 0;
    auto         preTanType = MFnAnimCurve::TangentType::kTangentFixed;
    for (const TsKnot& knot : knots) {
        float value;

        auto outTanType = _ConvertUsdTanTypeToMayaTanType(knot.GetNextInterpolation());
        if (knot.IsDualValued() && outTanType == MFnAnimCurve::kTangentStep) {
            knot.GetPreValue(&value);
            outTanType = MFnAnimCurve::TangentType::kTangentStepNext;
        } else {
            knot.GetValue(&value);
        }

        switch (convertToUnit) {
        case MDistance::kInches: value = UsdMayaUtil::ConvertMMToInches(value); break;
        case MDistance::kCentimeters: value = UsdMayaUtil::ConvertMMToCM(value); break;
        default:
            // The input is expected to be in millimeters.
            break;
        }

        TsTime inMayaTime {}, outMayaTime {};
        float  inUsdSlope = 0.f, outUsdSlope = 0.f, inMayaSlope = 0.f, outMayaSlope = 0.f;
        knot.GetPreTanSlope(&inUsdSlope);
        knot.GetPostTanSlope(&outUsdSlope);

        // Converting from standard (Usd) tangent to Maya tangent:
        // Usd tangents are specified by slope and length and Slopes are "rise over run": height
        // divided by length.
        // Maya tangents are specified by height and length. Height and length are both specified
        // multiplied by 3 Heights are positive for upward-sloping post-tangents, and negative for
        // upward-sloping pre-tangents.
        TsConvertFromStandardTangent(
            knot.GetPreTanWidth(), inUsdSlope, true, true, true, &inMayaTime, &inMayaSlope);
        TsConvertFromStandardTangent(
            knot.GetPostTanWidth(), outUsdSlope, true, true, false, &outMayaTime, &outMayaSlope);

        timeArray.set(MTime(knot.GetTime()), knotIdx);
        valuesArray.set(value, knotIdx);
        tangentInTypeArray.set(preTanType, knotIdx);
        tangentOutTypeArray.set(outTanType, knotIdx);

        // When tangent type is step or step next, maya requires the tangent values to be set to
        // DBL_MAX.
        if (outTanType == MFnAnimCurve::kTangentStep
            || outTanType == MFnAnimCurve::kTangentStepNext) {
            preTanType = MFnAnimCurve::TangentType::kTangentFixed;
            tangentInXArray.set(DBL_MAX, knotIdx);
            tangentInYArray.set(DBL_MAX, knotIdx);
            tangentOutXArray.set(DBL_MAX, knotIdx);
            tangentOutYArray.set(DBL_MAX, knotIdx);
        } else {
            preTanType = outTanType;
            tangentInXArray.set(inMayaTime, knotIdx);
            tangentInYArray.set(inMayaSlope, knotIdx);
            tangentOutXArray.set(outMayaTime, knotIdx);
            tangentOutYArray.set(outMayaSlope, knotIdx);
        }
        knotIdx++;
    }

    // set all keys and angles
    status = animFn.addKeysWithTangents(
        &timeArray,
        &valuesArray,
        MFnAnimCurve::kTangentGlobal,
        MFnAnimCurve::kTangentGlobal,
        &tangentInTypeArray,
        &tangentOutTypeArray,
        &tangentInXArray,
        &tangentInYArray,
        &tangentOutXArray,
        &tangentOutYArray,
        &tangentsLockedArray,
        &weightsLockedArray);

    CHECK_MSTATUS_AND_RETURN(status, false)

    animFn.setPreInfinityType(
        _ConvertUsdExtrapolationTypeToMaya(spline.GetPreExtrapolation().mode));
    animFn.setPostInfinityType(
        _ConvertUsdExtrapolationTypeToMaya(spline.GetPostExtrapolation().mode));
    animFn.setIsWeighted(false);

    if (context) {
        // used for undo/redo
        context->RegisterNewMayaNode(animFn.name().asChar(), animObj);
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif