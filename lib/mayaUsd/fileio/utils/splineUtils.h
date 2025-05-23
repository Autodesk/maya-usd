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
#ifndef PXRUSDMAYA_SPLINEUTILS_H
#define PXRUSDMAYA_SPLINEUTILS_H
#include <maya/MGlobal.h>
#include <pxr/pxr.h>

#if PXR_VERSION >= 2411

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/type.h>
#include <pxr/base/ts/knot.h>
#include <pxr/base/ts/spline.h>
#include <pxr/base/ts/tangentConversions.h>
#include <pxr/base/ts/types.h>

#include <maya/MDistance.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

class FlexibleSparseValueWriter;
struct UsdMayaJobExportArgs;

static TsInterpMode _ConvertMayaTanTypeToUsdTanType(MFnAnimCurve::TangentType mayaTangentType)
{
    switch (mayaTangentType) {
    case MFnAnimCurve::TangentType::kTangentStep:
    case MFnAnimCurve::TangentType::kTangentStepNext: return TsInterpHeld;
    case MFnAnimCurve::TangentType::kTangentLinear: return TsInterpLinear;
    default: return TsInterpCurve;
    }
}

static TsExtrapMode _ConvertUsdExtrapolationToMaya(MFnAnimCurve::InfinityType mayaExtrapolation)
{
    switch (mayaExtrapolation) {
    case MFnAnimCurve::InfinityType::kLinear: return TsExtrapMode::TsExtrapLinear;
    case MFnAnimCurve::InfinityType::kCycle: return TsExtrapMode::TsExtrapLoopReset;
    case MFnAnimCurve::InfinityType::kOscillate: return TsExtrapMode::TsExtrapLoopOscillate;
    case MFnAnimCurve::InfinityType::kCycleRelative: return TsExtrapMode::TsExtrapLoopRepeat;
    case MFnAnimCurve::InfinityType::kConstant:
    default: return TsExtrapMode::TsExtrapHeld;
    }
}

static MFnAnimCurve::InfinityType _ConvertUsdExtrapolationTypeToMaya(TsExtrapMode usdExtrapolation)
{
    switch (usdExtrapolation) {
    case TsExtrapLinear: return MFnAnimCurve::InfinityType::kLinear;
    case TsExtrapLoopReset: return MFnAnimCurve::InfinityType::kCycle;
    case TsExtrapLoopOscillate: return MFnAnimCurve::InfinityType::kOscillate;
    case TsExtrapLoopRepeat: return MFnAnimCurve::InfinityType::kCycleRelative;
    case TsExtrapHeld:
    default: return MFnAnimCurve::InfinityType::kConstant;
    }
}

static MFnAnimCurve::TangentType _ConvertUsdTanTypeToMayaTanType(TsInterpMode usdTanType)
{
    switch (usdTanType) {
    case TsInterpMode::TsInterpHeld: return MFnAnimCurve::TangentType::kTangentStep;
    case TsInterpMode::TsInterpLinear: return MFnAnimCurve::TangentType::kTangentLinear;
    default: return MFnAnimCurve::TangentType::kTangentAuto;
    }
}

/// This struct contains helpers for writing USD (thus reading Maya data).
struct UsdMayaSplineUtils
{
    /// Get the UsdKnots from a Maya curve.
    ///
    /// Returns an empty TsKnotMap if the curve doesn't exist or has no keys.
    template <typename T>
    static TsKnotMap GetKnotsFromMayaCurve(
        const MFnDependencyNode& depNode,
        const MString&           name,
        float                    scaling = 1.f)
    {
        TsKnotMap knots;
        auto      valueType = TfType::Find<T>();

        MStatus status;
        depNode.attribute(name, &status);
        CHECK_MSTATUS_AND_RETURN(status, knots)
        MPlug plug = depNode.findPlug(name, true, &status);
        CHECK_MSTATUS_AND_RETURN(status, knots)

        // get the animation curve for the given maya attribute if there's one
        MFnAnimCurve flAnimCurve(plug, &status);
        if (status != MStatus::kSuccess) {
            return knots;
        }

        auto numKeys = flAnimCurve.numKeys();
        for (unsigned int k = 0; k < numKeys; ++k) {
            auto time = flAnimCurve.time(k).value();

            auto   value = flAnimCurve.value(k);
            MTime  convert(1.0, MTime::kSeconds);
            double inTangentX, inTangentY;
            double outTangentX, outTangentY;
            flAnimCurve.getTangent(k, inTangentX, inTangentY, true);
            flAnimCurve.getTangent(k, outTangentX, outTangentY, false);

            // This was taken from the .getTangent() docs:
            // Need to multiply the value with the time unit conversion factor
            inTangentX *= convert.as(MTime::uiUnit());
            outTangentX *= convert.as(MTime::uiUnit());

            TsTime inTime {}, outTime {};
            float  inSlope = 0.f, outSlope = 0.f;

            // Converting from maya tangent to standard (Usd) tangent:
            // Usd tangents are specified by slope and length and Slopes are "rise over run": height
            // divided by length.
            // Maya tangents are specified by height and length. Height and length
            // are both specified multiplied by 3 Heights are positive for upward-sloping
            // post-tangents, and negative for upward-sloping pre-tangents.
            TsConvertToStandardTangent(
                static_cast<float>(inTangentX),
                static_cast<float>(inTangentY),
                true,
                true,
                true,
                &inTime,
                &inSlope);

            TsKnot     knot(valueType);
            const auto outTanType = flAnimCurve.outTangentType(k);
            const auto convertedValue = static_cast<T>(value) * scaling;

            // Deal with the case where slope would be infinite,
            // Because when there's a step on the curve is discontinuous
            if (outTanType == MFnAnimCurve::kTangentStepNext) {
                // Maya's step next is a special case where the value jumps to the next key's value.
                // If this is the last key, then set the value to the current value, making it
                // behave like a step.
                knot.SetPreValue(convertedValue);
                if (k + 1 < numKeys) {
                    knot.SetValue(static_cast<T>(flAnimCurve.value(k + 1)) * scaling);
                } else {
                    knot.SetValue(convertedValue);
                }
            } else if (outTanType == MFnAnimCurve::kTangentStep) {
                // no need to convert tangent in this case because it is 0 for the step.
                knot.SetValue(static_cast<float>(value) * scaling);
            } else {
                TsConvertToStandardTangent(
                    static_cast<float>(outTangentX),
                    static_cast<float>(outTangentY),
                    true,
                    true,
                    false,
                    &outTime,
                    &outSlope);
                knot.SetValue(static_cast<float>(value) * scaling);
            }

            knot.SetTime(time);
            knot.SetPostTanSlope(outSlope);
            knot.SetPreTanSlope(inSlope);
            knot.SetPostTanWidth(outTime);
            knot.SetPreTanWidth(inTime);
            knot.SetNextInterpolation(_ConvertMayaTanTypeToUsdTanType(outTanType));

            knots.insert(knot);
        }

        return knots;
    }

    /// Get the UsdSpline from a Maya curve.
    ///
    /// The spline will have different configuration based on the wanted curve.
    template <typename T>
    static TsSpline GetSplineFromMayaCurve(const MFnDependencyNode& depNode, const MString& name)
    {
        auto spline = TsSpline(TfType::Find<T>());

        MStatus status;
        depNode.attribute(name, &status);
        CHECK_MSTATUS_AND_RETURN(status, spline)
        MPlug plug = depNode.findPlug(name, true, &status);
        CHECK_MSTATUS_AND_RETURN(status, spline)

        // get the animation curve for the given maya attribute
        MFnAnimCurve flAnimCurve(plug, &status);
        CHECK_MSTATUS_AND_RETURN(status, spline)

        TsExtrapolation preExtrapolation(
            _ConvertUsdExtrapolationToMaya(flAnimCurve.preInfinityType()));
        TsExtrapolation postExtrapolation(
            _ConvertUsdExtrapolationToMaya(flAnimCurve.postInfinityType()));
        spline.SetPreExtrapolation(preExtrapolation);
        spline.SetPostExtrapolation(postExtrapolation);

        return spline;
    }

    template <typename T>
    static bool WriteUsdSplineToPlug(
        MPlug&                          plug,
        TsSpline                        spline,
        class UsdMayaPrimReaderContext* context,
        const MDistance::Unit           convertToUnit = MDistance::kMillimeters)
    {
        return WriteUsdSplineToPlug(plug, spline, context, TfType::Find<T>(), convertToUnit);
    }

    MAYAUSD_CORE_PUBLIC static bool WriteUsdSplineToPlug(
        MPlug&                          plug,
        TsSpline                        spline,
        class UsdMayaPrimReaderContext* context,
        const TfType&                   valueType,
        const MDistance::Unit           convertToUnit = MDistance::kMillimeters);


    template <typename T>
    static bool WriteSplineAttribute(
        const MFnDependencyNode& depNode,
        UsdPrim&     prim,
        const std::string& mayaAttrName,
        const TfToken&     usdAttrName)
    {
        TsKnotMap knots
            = UsdMayaSplineUtils::GetKnotsFromMayaCurve<T>(depNode, mayaAttrName.c_str());
        if (knots.empty()) {
            return false;
        }

        auto usdAttr = prim.GetAttribute(usdAttrName);
        if (!usdAttr) {
            return false;
        }

        TsSpline spline
            = UsdMayaSplineUtils::GetSplineFromMayaCurve<T>(depNode, mayaAttrName.c_str());
        spline.SetKnots(knots);

        if (!usdAttr.SetSpline(spline)) {
            return false;
        }

        return true;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE
#endif
#endif
