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
#include <pxr/pxr.h>

#if PXR_VERSION >= 2411

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
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
    /**
     * @brief Extracts knot data from a Maya animation curve and converts it into a USD knot map.
     *
     * This function retrieves the animation curve associated with a specified Maya attribute,
     * processes its keyframes, and converts the tangent and value data into a USD knot
     * map.
     *
     * @param depNode The Maya dependency node containing the attribute.
     * @param name The name of the Maya attribute to retrieve the animation curve from.
     * @param scaling A scaling factor applied to the values extracted from the curve (default
     * is 1.0).
     * @return TsKnotMap A USD knot map containing the processed keyframe data from the Maya
     * animation curve.
     */
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
                knot.SetValue(static_cast<T>(value) * scaling);
            } else {
                TsConvertToStandardTangent(
                    static_cast<float>(outTangentX),
                    static_cast<float>(outTangentY),
                    true,
                    true,
                    false,
                    &outTime,
                    &outSlope);
                knot.SetValue(static_cast<T>(value) * scaling);
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

    /**
     * @brief Retrieves a USD spline from a Maya curve attribute.
     *
     * This function extracts the spline data from a specified Maya attribute and converts it into
     * a USD spline. The USD spline includes pre- and post-extrapolation settings based on the Maya
     * curve's infinity types.
     *
     * @param depNode The Maya dependency node containing the attribute.
     * @param name The name of the Maya attribute to retrieve the spline data from.
     * @return TsSpline The USD spline created from the Maya curve attribute.
     */
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

    /**
     * @brief Writes a Maya spline attribute to a USD attribute.
     *
     * This function retrieves the knots and spline data from a Maya attribute and writes them to
     * the corresponding USD attribute. If the Maya attribute does not have a spline, it writes the
     * constant value instead.
     *
     * @param depNode The Maya dependency node containing the attribute.
     * @param prim The USD primitive to which the attribute will be written.
     * @param mayaAttrName The name of the Maya attribute to retrieve the spline data from.
     * @param usdAttrName The name of the USD attribute to write the spline data to.
     * @return bool Returns true if the attribute was successfully written, false otherwise.
     */
    template <typename T>
    static bool WriteSplineAttribute(
        const MFnDependencyNode& depNode,
        UsdPrim&                 prim,
        const std::string&       mayaAttrName,
        const TfToken&           usdAttrName)
    {
        auto usdAttr = prim.GetAttribute(usdAttrName);
        if (!usdAttr) {
            return false;
        }

        TsKnotMap knots
            = UsdMayaSplineUtils::GetKnotsFromMayaCurve<T>(depNode, mayaAttrName.c_str());
        if (knots.empty()) {
            MStatus status;
            auto    plug = depNode.findPlug(mayaAttrName.c_str(), true, &status);
            T       val;
            plug.getValue(val);
            if (UsdMayaWriteUtil::SetAttribute(usdAttr, val, UsdTimeCode::Default())) {
                return true;
            }

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

    /**
     * @brief Converts a float-based USD spline into a boolean-based USD spline using a lambda
     * function.
     *
     * This function iterates through the knots of a given float-based USD spline, applies a
     * user-defined lambda function to convert the float values into boolean values, and updates the
     * spline accordingly.
     *
     * @param spline The input float-based USD spline to be converted.
     * @param lambda A function that takes a float value and returns a boolean value.
     * @return TsSpline The resulting boolean-based USD spline after applying the lambda function.
     */
    MAYAUSD_CORE_PUBLIC static TsSpline
    BoolSplineFromFloatSpline(const TsSpline& spline, const std::function<bool(float)>& lambda)
    {
        auto    boolSpline = spline;
        auto    knots = boolSpline.GetKnots();
        VtValue val;
        for (auto& knot : knots) {
            knot.GetValue(&val);
#if PXR_VERSION >= 2505
            knot.SetValue(lambda(val.Get<float>()));
#else
            knot.SetValue(static_cast<float>(lambda(val.Get<float>())));
#endif
        }
        boolSpline.SetKnots(knots);
        return boolSpline;
    }

    /**
     * @brief Combines two Maya curves into a single USD spline by applying a lambda function to
     * their values.
     *
     * This function retrieves spline data from two Maya attributes, applies a user-defined lambda
     * function to combine their values, and returns the resulting USD spline. If one of the
     * attributes does not have a curve, the constant value from the plug is used instead. If both
     * attributes lack curves, an empty spline is returned.
     *
     * @param depNode The Maya dependency node containing the attributes.
     * @param attrName1 The name of the first Maya attribute.
     * @param attrName2 The name of the second Maya attribute.
     * @param lambda A function that takes two values (from the two attributes) and
     * returns the computed value.
     * @return TsSpline The resulting USD spline after combining the values of the two Maya
     * attributes.
     */
    template <typename T>
    static TsSpline CombineMayaCurveToUsdSpline(
        const MFnDependencyNode&      depNode,
        const MString&                attrName1,
        const MString&                attrName2,
        const std::function<T(T, T)>& lambda)
    {
        // Retrieve spline for the first attribute
        TsSpline  spline1 = GetSplineFromMayaCurve<T>(depNode, attrName1);
        TsKnotMap knots = GetKnotsFromMayaCurve<T>(depNode, attrName1);
        spline1.SetKnots(knots);
        T    constantValue1 = T();
        bool hasCurve1 = !spline1.IsEmpty();

        // Retrieve spline for the second attribute
        TsSpline spline2 = GetSplineFromMayaCurve<T>(depNode, attrName2);
        knots = GetKnotsFromMayaCurve<T>(depNode, attrName2);
        spline2.SetKnots(knots);
        T    constantValue2 = T();
        bool hasCurve2 = !spline2.IsEmpty();

        // If both curves are empty, return an empty spline
        if (!hasCurve1 && !hasCurve2) {
            return TsSpline(TfType::Find<T>());
        }

        if (!hasCurve1) {
            // If no curve, retrieve the constant value from the plug
            MPlug plug1 = depNode.findPlug(attrName1, true);
            if (!plug1.isNull()) {
                plug1.getValue(constantValue1);
            }
        }

        if (!hasCurve2) {
            // If no curve, retrieve the constant value from the plug
            MPlug plug2 = depNode.findPlug(attrName2, true);
            if (!plug2.isNull()) {
                plug2.getValue(constantValue2);
            }
        }

        TsSpline resultSpline;
        TsSpline secondarySpline;
        T        constVal = T();

        // Arbitrarily choose the spline with more knots as the result spline
        if (spline1.GetKnots().size() > spline2.GetKnots().size()) {
            resultSpline = spline1;
            secondarySpline = spline2;
            constVal = constantValue2;
        } else {
            resultSpline = spline2;
            secondarySpline = spline1;
            constVal = constantValue1;
        }

        // Iterate through the knots and apply the lambda function
        for (auto knot : resultSpline.GetKnots()) {
            T v;
            knot.GetValue(&v);

            T v2;
            if (!secondarySpline.IsEmpty()) {
                // Find the knot in the secondary spline that matches the time of the current knot
                auto   time = knot.GetTime();
                TsKnot secondaryKnot;
                if (secondarySpline.GetKnot(time, &secondaryKnot)) {
                    secondaryKnot.GetValue(&v2);
                } else {
                    secondarySpline.Eval(time, &v2);
                }
            } else {
                v2 = constVal;
            }

            T resultValue = lambda(v, v2);
            knot.SetValue(resultValue);
        }
        return resultSpline;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE
#endif
#endif
