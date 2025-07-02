//
// Copyright 2016 Pixar
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
#include "translatorXformable.h"

#include <mayaUsd/fileio/translators/translatorPrim.h>
#include <mayaUsd/fileio/utils/splineUtils.h>
#include <mayaUsd/fileio/utils/xformStack.h>

#include <pxr/base/gf/math.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MDagModifier.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>

#include <algorithm>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// This function retrieves a value for a given xformOp and given time sample. It
// knows how to deal with different type of ops and angle conversion
static bool _getXformOpAsVec3d(
    const UsdGeomXformOp& xformOp,
    const TfToken&        opName,
    GfVec3d&              value,
    const UsdTimeCode&    usdTime)
{
    bool retValue = false;

#ifdef USD_SUPPORT_INDIVIDUAL_TRANSFORMS
    if ((xformOp.GetTypeName() == SdfValueTypeNames->Float
         || xformOp.GetTypeName() == SdfValueTypeNames->Double)
        // RotateAxis is an individual transform that was supported before usd2505.
        // Keep the existing behavior
        && opName != UsdMayaXformStackTokens->rotateAxis) {
        return retValue;
    }
#endif
    const UsdGeomXformOp::Type opType = xformOp.GetOpType();

    if (opType == UsdGeomXformOp::TypeScale) {
        value = GfVec3d(1.0);
    } else {
        value = GfVec3d(0.0);
    }

    // Check whether the XformOp is a type of rotation.
    int    rotAxis = -1;
    double angleMult = GfDegreesToRadians(1.0);

    switch (opType) {
    case UsdGeomXformOp::TypeRotateX: rotAxis = 0; break;
    case UsdGeomXformOp::TypeRotateY: rotAxis = 1; break;
    case UsdGeomXformOp::TypeRotateZ: rotAxis = 2; break;
    case UsdGeomXformOp::TypeRotateXYZ:
    case UsdGeomXformOp::TypeRotateXZY:
    case UsdGeomXformOp::TypeRotateYXZ:
    case UsdGeomXformOp::TypeRotateYZX:
    case UsdGeomXformOp::TypeRotateZXY:
    case UsdGeomXformOp::TypeRotateZYX: break;
    default:
        // This XformOp is not a rotation, so we're not converting an
        // angular value from degrees to radians.
        angleMult = 1.0;
        break;
    }

    // If we encounter a transform op, we treat it as a shear operation.
    if (opType == UsdGeomXformOp::TypeTransform) {
        // GetOpTransform() handles the inverse op case for us.
        GfMatrix4d xform = xformOp.GetOpTransform(usdTime);
        value[0] = xform[1][0]; // xyVal
        value[1] = xform[2][0]; // xzVal
        value[2] = xform[2][1]; // yzVal
        retValue = true;
    } else if (rotAxis != -1) {
        // Single Axis rotation
        double valued = 0;
        retValue = xformOp.GetAs<double>(&valued, usdTime);
        if (retValue) {
            if (xformOp.IsInverseOp()) {
                valued = -valued;
            }
            value[rotAxis] = valued * angleMult;
        }
    } else {
        GfVec3d valued;
        retValue = xformOp.GetAs<GfVec3d>(&valued, usdTime);
        if (retValue) {
            if (xformOp.IsInverseOp()) {
                valued = -valued;
            }
            value[0] = valued[0] * angleMult;
            value[1] = valued[1] * angleMult;
            value[2] = valued[2] * angleMult;
        }
    }

    return retValue;
}

#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
static bool
_getSingleXformOp(const UsdGeomXformOp& xformOp, double& value, const UsdTimeCode& usdTime)
{
    bool retValue = false;

    const UsdGeomXformOp::Type opType = xformOp.GetOpType();

    if (opType == UsdGeomXformOp::TypeScale) {
        value = 1.0;
    } else {
        value = 0.0;
    }

    double angleMult = 1.0;
    switch (opType) {
    case UsdGeomXformOp::TypeRotateX: angleMult = GfDegreesToRadians(1.0); break;
    case UsdGeomXformOp::TypeRotateY: angleMult = GfDegreesToRadians(1.0); break;
    case UsdGeomXformOp::TypeRotateZ: angleMult = GfDegreesToRadians(1.0); break;
    default:
        // This XformOp is not a rotation, so we're not converting an
        // angular value from degrees to radians.
        break;
    }

    retValue = xformOp.GetAs<double>(&value, usdTime);
    if (retValue) {
        value *= angleMult;
        if (xformOp.IsInverseOp()) {
            value = -value;
        }
        retValue = true;
    }

    return retValue;
}
#endif

// Sets the animation curve (a knot per frame) for a given plug/attribute
static MObject _setAnimPlugData(
    MPlug                           plg,
    std::vector<double>&            value,
    MTimeArray&                     timeArray,
    const UsdMayaPrimReaderContext* context)
{
    MStatus      status;
    MFnAnimCurve animFn;
    // Make the plug keyable before attaching an anim curve
    if (!plg.isKeyable()) {
        plg.setKeyable(true);
    }
    MObject animObj = animFn.create(plg, nullptr, &status);
    if (status == MS::kSuccess) {
        MDoubleArray valueArray(&value[0], value.size());
        animFn.addKeys(&timeArray, &valueArray);
        if (context) {
            context->RegisterNewMayaNode(animFn.name().asChar(), animObj);
        }
    } else {
        MString mayaPlgName = plg.partialName(true, true, true, false, true, true, &status);
        TF_RUNTIME_ERROR(
            "Failed to create animation object for attribute: %s", mayaPlgName.asChar());
    }

    return animObj;
}

// Returns true if the array is not constant
static bool _isArrayVarying(std::vector<double>& value)
{
    bool isVarying = false;
    for (unsigned int i = 1; i < value.size(); i++) {
        if (!GfIsClose(value[0], value[i], 1e-9)) {
            isVarying = true;
            break;
        }
    }
    return isVarying;
}

// Sets the Maya Attribute values. Sets the value to the first element of the
// double arrays and then if the array is varying defines an anym curve for the
// attribute
static void _setMayaAttribute(
    MFnDagNode&                     depFn,
    std::vector<double>&            xVal,
    std::vector<double>&            yVal,
    std::vector<double>&            zVal,
    MTimeArray&                     timeArray,
    const MString&                  opName,
    const MString&                  x,
    const MString&                  y,
    const MString&                  z,
    const UsdMayaPrimReaderContext* context,
    bool                            applyEulerFilter = false)
{

    // if have multiple values, and applyEulerFilter, filter the values
    //
    if (applyEulerFilter && opName == "rotate") {
        if (xVal.size() == static_cast<size_t>(timeArray.length()) && xVal.size() == yVal.size()
            && xVal.size() == zVal.size()) {
            MPlug                         rotOrder = depFn.findPlug("rotateOrder");
            MEulerRotation::RotationOrder order
                = static_cast<MEulerRotation::RotationOrder>(rotOrder.asInt());

            MEulerRotation last(xVal[0], yVal[0], zVal[0], order);
            for (size_t i = 1; i < xVal.size(); ++i) {
                MEulerRotation current(xVal[i], yVal[i], zVal[i], order);
                current.setToClosestSolution(last);
                xVal[i] = current[0];
                yVal[i] = current[1];
                zVal[i] = current[2];
                last = current;
            }
        }
    }

    MPlug plg;
    if (x != "" && !xVal.empty()) {
        plg = depFn.findPlug(opName + x);
        if (!plg.isNull()) {
            plg.setDouble(xVal[0]);
            if (xVal.size() > 1 && (applyEulerFilter || _isArrayVarying(xVal))) {
                _setAnimPlugData(plg, xVal, timeArray, context);
            }
        }
    }
    if (y != "" && !yVal.empty()) {
        plg = depFn.findPlug(opName + y);
        if (!plg.isNull()) {
            plg.setDouble(yVal[0]);
            if (yVal.size() > 1 && (applyEulerFilter || _isArrayVarying(yVal))) {
                _setAnimPlugData(plg, yVal, timeArray, context);
            }
        }
    }
    if (z != "" && !zVal.empty()) {
        plg = depFn.findPlug(opName + z);
        if (!plg.isNull()) {
            plg.setDouble(zVal[0]);
            if (zVal.size() > 1 && (applyEulerFilter || _isArrayVarying(zVal))) {
                _setAnimPlugData(plg, zVal, timeArray, context);
            }
        }
    }
}

// For each xformop, we gather it's data either time sampled or not and we push
// it to the corresponding Maya xform
static bool _pushUSDXformOpToMayaXform(
    const UsdGeomXformOp&           xformop,
    const TfToken&                  opName,
    MFnDagNode&                     MdagNode,
    const UsdMayaPrimReaderArgs&    args,
    const UsdMayaPrimReaderContext* context)
{
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
    // If the xformop has a spline, we write it to the plug directly
    const auto&                opAttr = xformop.GetAttr();
    const UsdGeomXformOp::Type opType = xformop.GetOpType();
    if (opAttr.HasSpline()) {
        MPlug plg = MdagNode.findPlug(MString(opName.GetString().c_str()), false);
        if (!plg.isNull()) {
            auto spline = opAttr.GetSpline();
            bool rotOp = opType == UsdGeomXformOp::TypeRotateX
                || opType == UsdGeomXformOp::TypeRotateY || opType == UsdGeomXformOp::TypeRotateZ;

            if (UsdGeomXformOp::GetPrecisionFromValueTypeName(xformop.GetAttr().GetTypeName())
                == UsdGeomXformOp::PrecisionDouble) {
                return UsdMayaSplineUtils::WriteUsdSplineToPlug<double>(plg, spline, context);
            }

            return UsdMayaSplineUtils::WriteUsdSplineToPlug<float>(
                plg, spline, context, rotOp ? M_PI / 180.0 : 1.f);
        }
    }
#endif
    MTime::Unit timeUnit = MTime::uiUnit();
    double timeSampleMultiplier = (context != nullptr) ? context->GetTimeSampleMultiplier() : 1.0;

    std::vector<double> xValue;
    std::vector<double> yValue;
    std::vector<double> zValue;
    GfVec3d             value;
    bool                isSingleTransformOp = false;
    std::vector<double> singleTransformOp;
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
    double singleVal = 0.0;
#endif
    MString             singleOpName;
    std::vector<double> timeSamples;

    bool applyEulerFilter = args.GetJobArguments().applyEulerFilter;

    if (!args.GetTimeInterval().IsEmpty()) {
        xformop.GetTimeSamplesInInterval(args.GetTimeInterval(), &timeSamples);
    }
    MTimeArray timeArray;
    if (!timeSamples.empty()) {
        timeArray.setLength(timeSamples.size());
        xValue.resize(timeSamples.size());
        yValue.resize(timeSamples.size());
        zValue.resize(timeSamples.size());
        singleTransformOp.resize(timeSamples.size());
        for (unsigned int ti = 0; ti < timeSamples.size(); ++ti) {
            UsdTimeCode time(timeSamples[ti]);
            if (_getXformOpAsVec3d(xformop, opName, value, time)) {
                xValue[ti] = value[0];
                yValue[ti] = value[1];
                zValue[ti] = value[2];
                timeArray.set(MTime(timeSamples[ti] * timeSampleMultiplier, timeUnit), ti);
            }
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
            else if (_getSingleXformOp(xformop, singleVal, time)) {
                singleTransformOp[ti] = singleVal;
                isSingleTransformOp = true;
            }
#endif
            else {
                TF_RUNTIME_ERROR(
                    "Missing sampled data on xformOp: %s", xformop.GetName().GetText());
            }
        }
    } else {
        // pick the first available sample or default
        UsdTimeCode time = UsdTimeCode::EarliestTime();
        if (_getXformOpAsVec3d(xformop, opName, value, time)) {
            xValue.resize(1);
            yValue.resize(1);
            zValue.resize(1);
            xValue[0] = value[0];
            yValue[0] = value[1];
            zValue[0] = value[2];
        }
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
        else if (_getSingleXformOp(xformop, singleVal, time)) {
            singleTransformOp.resize(1);
            singleTransformOp[0] = singleVal;
            isSingleTransformOp = true;
        }
#endif
        else {
            TF_RUNTIME_ERROR("Missing default data on xformOp: %s", xformop.GetName().GetText());
        }
    }
    if (isSingleTransformOp) {
        std::string transformType = opName.GetString();
        char        axis = transformType.back(); // get the last character, which is possibly axis
        if (axis == 'X' || axis == 'Y' || axis == 'Z') {
            transformType.pop_back();
        }
        singleOpName = MString(transformType.c_str());
    }
    if (!xValue.empty() || isSingleTransformOp) {
        if (opName == UsdMayaXformStackTokens->shear) {
            _setMayaAttribute(
                MdagNode,
                xValue,
                yValue,
                zValue,
                timeArray,
                MString(opName.GetText()),
                "XY",
                "XZ",
                "YZ",
                context);
        } else if (opName == UsdMayaXformStackTokens->pivot) {
            _setMayaAttribute(
                MdagNode,
                xValue,
                yValue,
                zValue,
                timeArray,
                MString("rotatePivot"),
                "X",
                "Y",
                "Z",
                context);
            _setMayaAttribute(
                MdagNode,
                xValue,
                yValue,
                zValue,
                timeArray,
                MString("scalePivot"),
                "X",
                "Y",
                "Z",
                context);
        } else if (opName == UsdMayaXformStackTokens->pivotTranslate) {
            _setMayaAttribute(
                MdagNode,
                xValue,
                yValue,
                zValue,
                timeArray,
                MString("rotatePivotTranslate"),
                "X",
                "Y",
                "Z",
                context);
            _setMayaAttribute(
                MdagNode,
                xValue,
                yValue,
                zValue,
                timeArray,
                MString("scalePivotTranslate"),
                "X",
                "Y",
                "Z",
                context);
        }
#ifdef USD_SUPPORT_INDIVIDUAL_TRANSFORMS
        else if (
            (opType == UsdGeomXformOp::TypeTranslateX || opType == UsdGeomXformOp::TypeRotateX
             || opType == UsdGeomXformOp::TypeScaleX)
            && isSingleTransformOp) {
            _setMayaAttribute(
                MdagNode,
                singleTransformOp,
                yValue,
                zValue,
                timeArray,
                singleOpName,
                "X",
                "",
                "",
                context);
        } else if (
            (opType == UsdGeomXformOp::TypeTranslateY || opType == UsdGeomXformOp::TypeRotateY
             || opType == UsdGeomXformOp::TypeScaleY)
            && isSingleTransformOp) {
            _setMayaAttribute(
                MdagNode,
                xValue,
                singleTransformOp,
                zValue,
                timeArray,
                singleOpName,
                "",
                "Y",
                "",
                context);
        } else if (
            (opType == UsdGeomXformOp::TypeTranslateZ || opType == UsdGeomXformOp::TypeRotateZ
             || opType == UsdGeomXformOp::TypeScaleZ)
            && isSingleTransformOp) {
            _setMayaAttribute(
                MdagNode,
                xValue,
                yValue,
                singleTransformOp,
                timeArray,
                singleOpName,
                "",
                "",
                "Z",
                context);
        }
#endif
        else {
            if (opName == UsdMayaXformStackTokens->rotate) {
                MFnTransform trans;
                if (trans.setObject(MdagNode.object())) {
                    auto MrotOrder = UsdMayaXformStack::RotateOrderFromOpType<
                        MTransformationMatrix::RotationOrder>(xformop.GetOpType());
                    MPlug plg = MdagNode.findPlug("rotateOrder");
                    if (!plg.isNull()) {
                        trans.setRotationOrder(MrotOrder, /*no need to reorder*/ false);
                    }
                }
            } else if (opName == UsdMayaXformStackTokens->rotateAxis) {
                // Rotate axis only accepts input in XYZ form
                // (though it's actually stored as a quaternion),
                // so we need to convert other rotation orders to XYZ
                const auto opType = xformop.GetOpType();
                if (opType != UsdGeomXformOp::TypeRotateXYZ && opType != UsdGeomXformOp::TypeRotateX
                    && opType != UsdGeomXformOp::TypeRotateY
                    && opType != UsdGeomXformOp::TypeRotateZ) {
                    for (size_t i = 0u; i < xValue.size(); ++i) {
                        auto MrotOrder = UsdMayaXformStack::RotateOrderFromOpType<
                            MEulerRotation::RotationOrder>(xformop.GetOpType());
                        MEulerRotation eulerRot(xValue[i], yValue[i], zValue[i], MrotOrder);
                        eulerRot.reorderIt(MEulerRotation::kXYZ);
                        xValue[i] = eulerRot.x;
                        yValue[i] = eulerRot.y;
                        zValue[i] = eulerRot.z;
                    }
                }
            }
            _setMayaAttribute(
                MdagNode,
                xValue,
                yValue,
                zValue,
                timeArray,
                MString(opName.GetText()),
                "X",
                "Y",
                "Z",
                context,
                applyEulerFilter && opName == UsdMayaXformStackTokens->rotate);
        }
        return true;
    }

    return false;
}

// Simple function that determines if the matrix is identity
// XXX Maybe there is something already in Gf but couldn't see it
static bool _isIdentityMatrix(const GfMatrix4d& m)
{
    static const GfMatrix4d identityMatrix(1.0);
    static constexpr double tolerance = 1e-9;

    return GfIsClose(m, identityMatrix, tolerance);
}

// For each xformop, we gather it's data either time sampled or not and we push it to the
// corresponding Maya xform
static bool _pushUSDXformToMayaXform(
    const UsdGeomXformable&         xformSchema,
    MFnDagNode&                     MdagNode,
    const UsdMayaPrimReaderArgs&    args,
    const UsdMayaPrimReaderContext* context)
{
    MTime::Unit timeUnit = MTime::uiUnit();
    double timeSampleMultiplier = (context != nullptr) ? context->GetTimeSampleMultiplier() : 1.0;

    std::vector<double> timeSamples;
    if (!args.GetTimeInterval().IsEmpty()) {
        xformSchema.GetTimeSamplesInInterval(args.GetTimeInterval(), &timeSamples);
    }

    std::vector<UsdTimeCode> timeCodes;
    MTimeArray               timeArray;

    if (!timeSamples.empty()) {
        // Convert all the time samples to UsdTimeCodes.
        std::transform(
            timeSamples.cbegin(),
            timeSamples.cend(),
            std::back_inserter(timeCodes),
            [](const double timeSample) { return UsdTimeCode(timeSample); });

        timeArray.setLength(timeCodes.size());
    } else {
        // If there were no time samples, pick the first available sample or default and
        // leave the MTimeArray empty.
        timeCodes.push_back(UsdTimeCode::EarliestTime());
    }

    // Storage for all of the components of the Maya transform attributes. Maya
    // only allows double-valued animation curves, so we store each channel
    // independently.
    std::vector<double> TxVal(timeCodes.size());
    std::vector<double> TyVal(timeCodes.size());
    std::vector<double> TzVal(timeCodes.size());
    std::vector<double> RxVal(timeCodes.size());
    std::vector<double> RyVal(timeCodes.size());
    std::vector<double> RzVal(timeCodes.size());
    std::vector<double> SxVal(timeCodes.size(), 1.0);
    std::vector<double> SyVal(timeCodes.size(), 1.0);
    std::vector<double> SzVal(timeCodes.size(), 1.0);
    std::vector<double> ShearXYVal(timeCodes.size());
    std::vector<double> ShearXZVal(timeCodes.size());
    std::vector<double> ShearYZVal(timeCodes.size());

    for (size_t ti = 0u; ti < timeCodes.size(); ++ti) {
        const UsdTimeCode& timeCode = timeCodes[ti];

        GfMatrix4d usdLocalTransform(1.0);
        bool       resetsXformStack;
        if (!xformSchema.GetLocalTransformation(&usdLocalTransform, &resetsXformStack, timeCode)
            && !xformSchema.GetPrim().IsInstance()) {
            if (timeCode.IsDefault()) {
                TF_RUNTIME_ERROR(
                    "Missing xform data at the default time on USD prim <%s>",
                    xformSchema.GetPath().GetText());
            } else {
                TF_RUNTIME_ERROR(
                    "Missing xform data at time %f on USD prim <%s>",
                    timeCode.GetValue(),
                    xformSchema.GetPath().GetText());
            }

            continue;
        }

        MVector translation(0, 0, 0);
        MVector rotation(0, 0, 0);
        MVector scale(1, 1, 1);
        MVector shear(0, 0, 0);

        if (!_isIdentityMatrix(usdLocalTransform)) {
            double usdLocalTransformData[4u][4u];
            usdLocalTransform.Get(usdLocalTransformData);
            const MMatrix               localMatrix(usdLocalTransformData);
            const MTransformationMatrix localTransformationMatrix(localMatrix);

            double  tempVec[3u];
            MStatus status;

            translation = localTransformationMatrix.getTranslation(MSpace::kTransform, &status);
            CHECK_MSTATUS(status);

            status = localTransformationMatrix.getScale(tempVec, MSpace::kTransform);
            CHECK_MSTATUS(status);
            scale = MVector(tempVec);

            MTransformationMatrix::RotationOrder rotateOrder;
            status = localTransformationMatrix.getRotation(tempVec, rotateOrder);
            CHECK_MSTATUS(status);
            rotation = MVector(tempVec);

            status = localTransformationMatrix.getShear(tempVec, MSpace::kTransform);
            CHECK_MSTATUS(status);
            shear = MVector(tempVec);
        }

        TxVal[ti] = translation[0];
        TyVal[ti] = translation[1];
        TzVal[ti] = translation[2];

        RxVal[ti] = rotation[0];
        RyVal[ti] = rotation[1];
        RzVal[ti] = rotation[2];

        SxVal[ti] = scale[0];
        SyVal[ti] = scale[1];
        SzVal[ti] = scale[2];

        ShearXYVal[ti] = shear[0];
        ShearXZVal[ti] = shear[1];
        ShearYZVal[ti] = shear[2];

        if (!timeSamples.empty()) {
            timeArray.set(MTime(timeCode.GetValue() * timeSampleMultiplier, timeUnit), ti);
        }
    }

    // All of these vectors should have the same size and greater than 0 to set their values
    if (TxVal.size() == TyVal.size() && TxVal.size() == TzVal.size() && !TxVal.empty()) {
        _setMayaAttribute(
            MdagNode, TxVal, TyVal, TzVal, timeArray, MString("translate"), "X", "Y", "Z", context);
        _setMayaAttribute(
            MdagNode, RxVal, RyVal, RzVal, timeArray, MString("rotate"), "X", "Y", "Z", context);
        _setMayaAttribute(
            MdagNode, SxVal, SyVal, SzVal, timeArray, MString("scale"), "X", "Y", "Z", context);
        _setMayaAttribute(
            MdagNode,
            ShearXYVal,
            ShearXZVal,
            ShearYZVal,
            timeArray,
            MString("shear"),
            "XY",
            "XZ",
            "YZ",
            context);

        return true;
    }

    return false;
}

void UsdMayaTranslatorXformable::Read(
    const UsdGeomXformable&      xformSchema,
    MObject                      mayaNode,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context)
{
    MStatus status;

    // == Read attrs ==
    // Read parent class attrs
    UsdMayaTranslatorPrim::Read(xformSchema.GetPrim(), mayaNode, args, context);

    // Scanning Xformops to see if we have a general Maya xform or an xform
    // that conform to the commonAPI
    //
    // If fail to retrieve proper ops with proper name and order, will try to
    // decompose the xform matrix
    bool                        resetsXformStack = false;
    std::vector<UsdGeomXformOp> xformops = xformSchema.GetOrderedXformOps(&resetsXformStack);

    // When we find ops, we match the ops by suffix ("" will define the basic
    // translate, rotate, scale) and by order. If we find an op with a
    // different name or out of order that will miss the match, we will rely on
    // matrix decomposition

    UsdMayaXformStack::OpClassList stackOps = UsdMayaXformStack::FirstMatchingSubstack(
        {
            &UsdMayaXformStack::MayaStack(),
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
                &UsdMayaXformStack::MayaIndividualTransformsStack(),
#endif
                &UsdMayaXformStack::CommonStack()
        },
        xformops);

    MFnDagNode MdagNode(mayaNode);
    if (!stackOps.empty()) {
        // make sure stackIndices.size() == xformops.size()
        std::string rotOrderStr = "";
        for (unsigned int i = 0; i < stackOps.size(); i++) {
            const UsdGeomXformOp&               xformop(xformops[i]);
            const UsdMayaXformOpClassification& opDef(stackOps[i]);
            // If we got a valid stack, we have both the members of the inverted twins..
            // ...so we can go ahead and skip the inverted twin
            if (opDef.IsInvertedTwin())
                continue;

            const TfToken& opName(opDef.GetName());

            _pushUSDXformOpToMayaXform(xformop, opName, MdagNode, args, context);

#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
            // If we have an individual rotation, we need to build the rotation order
            if (opName == UsdMayaXformStackTokens->rotateX) {
                rotOrderStr.insert(0, "x");
            } else if (opName == UsdMayaXformStackTokens->rotateY) {
                rotOrderStr.insert(0, "y");
            } else if (opName == UsdMayaXformStackTokens->rotateZ) {
                rotOrderStr.insert(0, "z");
            }
#endif
        }
#if USD_SUPPORT_INDIVIDUAL_TRANSFORMS
        if (!rotOrderStr.empty()) {
            MFnTransform trans;
            if (trans.setObject(MdagNode.object())) {
                // Static lookup table for rotation order mapping
                static const std::unordered_map<std::string, MTransformationMatrix::RotationOrder>
                    rotOrderMap = { { "xyz", MTransformationMatrix::RotationOrder::kXYZ },
                                    { "xzy", MTransformationMatrix::RotationOrder::kXZY },
                                    { "yxz", MTransformationMatrix::RotationOrder::kYXZ },
                                    { "yzx", MTransformationMatrix::RotationOrder::kYZX },
                                    { "zxy", MTransformationMatrix::RotationOrder::kZXY },
                                    { "zyx", MTransformationMatrix::RotationOrder::kZYX },
                                    { "xy", MTransformationMatrix::RotationOrder::kXYZ },
                                    { "xz", MTransformationMatrix::RotationOrder::kXZY },
                                    { "yx", MTransformationMatrix::RotationOrder::kYXZ },
                                    { "yz", MTransformationMatrix::RotationOrder::kYZX },
                                    { "zx", MTransformationMatrix::RotationOrder::kZXY },
                                    { "zy", MTransformationMatrix::RotationOrder::kZYX },
                                    { "x", MTransformationMatrix::RotationOrder::kXYZ },
                                    { "y", MTransformationMatrix::RotationOrder::kXYZ },
                                    { "z", MTransformationMatrix::RotationOrder::kXYZ } };

                auto it = rotOrderMap.find(rotOrderStr);
                auto rotOrder = (it != rotOrderMap.end()) ? it->second : [&]() {
                    TF_WARN(
                        "Unsupported rotation order '%s' for prim <%s>",
                        rotOrderStr.c_str(),
                        xformSchema.GetPath().GetText());
                    return MTransformationMatrix::RotationOrder::kXYZ;
                }();
                MPlug plgRotateOrder = MdagNode.findPlug("rotateOrder", false);
                if (!plgRotateOrder.isNull()) {
                    trans.setRotationOrder(rotOrder, /*no need to reorder*/ false);
                }
            }
        }
#endif
    } else {
        if (!_pushUSDXformToMayaXform(xformSchema, MdagNode, args, context)) {
            TF_RUNTIME_ERROR(
                "Unable to successfully decompose matrix at USD prim <%s>",
                xformSchema.GetPath().GetText());
        }
    }

    if (resetsXformStack) {
        MPlug plg = MdagNode.findPlug("inheritsTransform");
        if (!plg.isNull())
            plg.setBool(false);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
