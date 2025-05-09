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
#include "translatorCamera.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#if PXR_VERSION >= 2411
#include <pxr/base/ts/spline.h>
#include <pxr/base/ts/tangentConversions.h>
#endif
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MDagModifier.h>
#include <maya/MDistance.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

static bool _ReadToCamera(
    const UsdGeomCamera&         usdCamera,
    MFnCamera&                   cameraObject,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((MayaCameraTypeName, "camera"))
    ((MayaCameraShapeNameSuffix, "Shape"))

    ((MayaCameraAttrNameHorizontalAperture, "horizontalFilmAperture"))
    ((MayaCameraAttrNameVerticalAperture, "verticalFilmAperture"))
    ((MayaCameraAttrNameHorizontalApertureOffset, "horizontalFilmOffset"))
    ((MayaCameraAttrNameVerticalApertureOffset, "verticalFilmOffset"))
    ((MayaCameraAttrNameOrthographicWidth, "orthographicWidth"))
    ((MayaCameraAttrNameFocalLength, "focalLength"))
    ((MayaCameraAttrNameFocusDistance, "focusDistance"))
    ((MayaCameraAttrNameFStop, "fStop"))
    ((MayaCameraAttrNameDepthOfField, "depthOfField"))
    ((MayaCameraAttrNameNearClippingPlane, "nearClipPlane"))
    ((MayaCameraAttrNameFarClippingPlane, "farClipPlane"))
);
// clang-format on

static bool _CheckUsdTypeAndResizeArrays(
    const UsdAttribute&  usdAttr,
    const TfType&        expectedType,
    const GfInterval&    timeInterval,
    std::vector<double>* timeSamples,
    MTimeArray*          timeArray,
    MDoubleArray*        valueArray)
{
    // Validate that the attribute holds values of the expected type.
    const TfType type = usdAttr.GetTypeName().GetType();
    if (type != expectedType) {
        TF_CODING_ERROR(
            "Unsupported type name for USD attribute '%s': %s",
            usdAttr.GetName().GetText(),
            type.GetTypeName().c_str());
        return false;
    }

    if (!usdAttr.GetTimeSamplesInInterval(timeInterval, timeSamples)) {
        return false;
    }

    size_t numTimeSamples = timeSamples->size();
    if (numTimeSamples < 1) {
        return false;
    }

    timeArray->setLength(numTimeSamples);
    valueArray->setLength(numTimeSamples);

    return true;
}

static bool _GetTimeAndValueArrayForUsdAttribute(
    const UsdAttribute&   usdAttr,
    const GfInterval&     timeInterval,
    MTimeArray*           timeArray,
    MDoubleArray*         valueArray,
    const MDistance::Unit convertToUnit = MDistance::kMillimeters,
    const double          timeSampleMultiplier = 1.0)
{
    static const TfType& floatType = TfType::Find<float>();
    std::vector<double>  timeSamples;

    if (!_CheckUsdTypeAndResizeArrays(
            usdAttr, floatType, timeInterval, &timeSamples, timeArray, valueArray)) {
        return false;
    }

    size_t      numTimeSamples = timeSamples.size();
    MTime::Unit timeUnit = MTime::uiUnit();
    for (size_t i = 0; i < numTimeSamples; ++i) {
        const double timeSample = timeSamples[i];
        float        attrValue;
        if (!usdAttr.Get(&attrValue, timeSample)) {
            return false;
        }

        switch (convertToUnit) {
        case MDistance::kInches: attrValue = UsdMayaUtil::ConvertMMToInches(attrValue); break;
        case MDistance::kCentimeters: attrValue = UsdMayaUtil::ConvertMMToCM(attrValue); break;
        default:
            // The input is expected to be in millimeters.
            break;
        }

        timeArray->set(MTime(timeSample * timeSampleMultiplier, timeUnit), i);
        valueArray->set(attrValue, i);
    }

    return true;
}

// This is primarily intended for use in translating the clippingRange
// USD attribute which is stored in USD as a single GfVec2f value but
// in Maya as separate nearClipPlane and farClipPlane attributes.
static bool _GetTimeAndValueArraysForUsdAttribute(
    const UsdAttribute& usdAttr,
    const GfInterval&   timeInterval,
    MTimeArray*         timeArray,
    MDoubleArray*       valueArray1,
    MDoubleArray*       valueArray2,
    const double        timeSampleMultiplier = 1.0)
{
    static const TfType& vec2fType = TfType::Find<GfVec2f>();
    std::vector<double>  timeSamples;

    if (!_CheckUsdTypeAndResizeArrays(
            usdAttr, vec2fType, timeInterval, &timeSamples, timeArray, valueArray1)) {
        return false;
    }

    size_t numTimeSamples = timeSamples.size();
    valueArray2->setLength(numTimeSamples);
    MTime::Unit timeUnit = MTime::uiUnit();

    for (size_t i = 0; i < numTimeSamples; ++i) {
        const double timeSample = timeSamples[i];
        GfVec2f      attrValue;
        if (!usdAttr.Get(&attrValue, timeSample)) {
            return false;
        }
        timeArray->set(MTime(timeSample * timeSampleMultiplier, timeUnit), i);
        valueArray1->set(attrValue[0], i);
        valueArray2->set(attrValue[1], i);
    }

    return true;
}

static bool _CreateAnimCurveForPlug(
    MPlug&                    plug,
    MTimeArray&               timeArray,
    MDoubleArray&             valueArray,
    UsdMayaPrimReaderContext* context)
{
    MFnAnimCurve animFn;
    MStatus      status;
    MObject      animObj = animFn.create(plug, nullptr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = animFn.addKeys(&timeArray, &valueArray);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (context) {
        // used for undo/redo
        context->RegisterNewMayaNode(animFn.name().asChar(), animObj);
    }

    return true;
}

#if PXR_VERSION >= 2411
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

static bool _CreatePlugSpline(
    MPlug&                    plug,
    TsSpline                  spline,
    UsdMayaPrimReaderContext* context,
    const MDistance::Unit     convertToUnit = MDistance::kMillimeters)
{
    auto valueType = spline.GetValueType();
    if (valueType != Ts_GetType<float>()) {
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

    unsigned int                           numKnots = static_cast<unsigned int>(knots.size());
    MTimeArray                             timeArray(numKnots, 0.0);
    MDoubleArray                           valuesArray(numKnots, 0.0);
    MIntArray                              tangentInTypeArray(numKnots, 0);
    std::vector<unsigned int>              knotTracker;
    MIntArray                              tangentOutTypeArray(numKnots, 0);
    std::vector<MFnAnimCurve::TangentType> tangentInTypeArray2;
    std::vector<MFnAnimCurve::TangentType> tangentOutTypeArray2;
    MIntArray                              tangentsLockedArray(numKnots, 1);
    MIntArray                              weightsLockedArray(numKnots, 0);
    MDoubleArray                           tangentInXArray(numKnots, 0.0);
    MDoubleArray                           tangentInYArray(numKnots, 0.0);
    MDoubleArray                           tangentOutXArray(numKnots, 0.0);
    MDoubleArray                           tangentOutYArray(numKnots, 0.0);

    unsigned int  knotIdx = 0;
    volatile auto preTanType = MFnAnimCurve::TangentType::kTangentFixed;
    for (const TsKnot& knot : knots) {
        float value;

        auto outTanType = _ConvertUsdTanTypeToMayaTanType(knot.GetNextInterpolation());
        if (knot.IsDualValued() && outTanType == MFnAnimCurve::kTangentStep) {
            knot.GetPreValue(&value);
            // preTanType = MFnAnimCurve::TangentType::kTangentFixed;
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

        auto keyIdx = animFn.addKeyframe(MTime(knot.GetTime()), value, preTanType, outTanType) - 1;
        // auto keyIdx = animFn.addKeyframe(MTime(knot.GetTime()), value) - 1;
        animFn.setTangentsLocked(keyIdx, true);
        animFn.setTangent(keyIdx, inMayaTime, inMayaSlope, true);
        animFn.setTangent(keyIdx, outMayaTime, outMayaSlope, false);
        // animFn.setInTangentType(keyIdx, preTanType);
        // animFn.setOutTangentType(keyIdx, outTanType);

        // valuesArray.set(value, knotIdx);
        // timeArray.set(MTime(knot.GetTime()), knotIdx);
        // tangentInTypeArray.set(preTanType, knotIdx);
        // tangentOutTypeArray.set(outTanType, knotIdx);
        // tangentInXArray.set(inMayaTime, knotIdx);
        // tangentInYArray.set(inMayaSlope, knotIdx);
        // tangentOutXArray.set(outMayaTime, knotIdx);
        // tangentOutYArray.set(outMayaSlope, knotIdx);
        //
        if (outTanType == MFnAnimCurve::kTangentStep
            || outTanType == MFnAnimCurve::kTangentStepNext) {
            knotTracker.emplace_back(knotIdx);
            preTanType = MFnAnimCurve::TangentType::kTangentFixed;
            // tangentInTypeArray2.emplace_back(preTanType);
            // tangentOutTypeArray2.emplace_back(outTanType);
        } else {
            preTanType = outTanType;
        }
        // preTanType = outTanType;
        knotIdx++;
    }

    // set all keys and angles
    // status = animFn.addKeysWithTangents(
    //    &timeArray,
    //    &valuesArray,
    //    MFnAnimCurve::kTangentGlobal,
    //    MFnAnimCurve::kTangentGlobal,
    //    &tangentInTypeArray,
    //    &tangentOutTypeArray,
    //    &tangentInXArray,
    //    &tangentInYArray,
    //    &tangentOutXArray,
    //    &tangentOutYArray,
    //    &tangentsLockedArray,
    //    &weightsLockedArray);

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
#endif

static bool _TranslateAnimatedUsdAttributeToPlug(
    const UsdAttribute&          usdAttr,
    MPlug&                       plug,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context,
    const MDistance::Unit        convertToUnit = MDistance::kMillimeters)
{
#if PXR_VERSION >= 2411
    // If the attribute has a spline, we ignore time samples.
    if (usdAttr.HasSpline()) {
        if (_CreatePlugSpline(plug, usdAttr.GetSpline(), context, convertToUnit)) {
            return true;
        }
    }
#endif

    if (args.GetTimeInterval().IsEmpty()) {
        return false;
    }

    MTimeArray   timeArray;
    MDoubleArray valueArray;
    if (!_GetTimeAndValueArrayForUsdAttribute(
            usdAttr,
            args.GetTimeInterval(),
            &timeArray,
            &valueArray,
            convertToUnit,
            (context != nullptr) ? context->GetTimeSampleMultiplier() : 1.0)) {
        return false;
    }

    if (!_CreateAnimCurveForPlug(plug, timeArray, valueArray, context)) {
        return false;
    }

    return true;
}

static bool _TranslateAnimatedUsdAttributeToPlugs(
    const UsdAttribute&          usdAttr,
    MPlug&                       plug1,
    MPlug&                       plug2,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context)
{
    if (args.GetTimeInterval().IsEmpty()) {
        return false;
    }

    MTimeArray   timeArray;
    MDoubleArray valueArray1;
    MDoubleArray valueArray2;
    if (!_GetTimeAndValueArraysForUsdAttribute(
            usdAttr,
            args.GetTimeInterval(),
            &timeArray,
            &valueArray1,
            &valueArray2,
            (context != nullptr) ? context->GetTimeSampleMultiplier() : 1.0)) {
        return false;
    }

    if (!_CreateAnimCurveForPlug(plug1, timeArray, valueArray1, context)) {
        return false;
    }

    if (!_CreateAnimCurveForPlug(plug2, timeArray, valueArray2, context)) {
        return false;
    }

    return true;
}

static bool _TranslateUsdAttributeToPlug(
    const UsdAttribute&          usdAttr,
    const MFnCamera&             cameraFn,
    const TfToken&               plugName,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context,
    const MDistance::Unit        convertToUnit = MDistance::kMillimeters)
{
    MStatus status;

    MPlug plug = cameraFn.findPlug(plugName.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // First check for and translate animation if there is any.
    if (!_TranslateAnimatedUsdAttributeToPlug(usdAttr, plug, args, context, convertToUnit)) {
        // If that fails, then try just setting a static value.
        UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
        float       attrValue;
        usdAttr.Get(&attrValue, timeCode);

        switch (convertToUnit) {
        case MDistance::kInches: attrValue = UsdMayaUtil::ConvertMMToInches(attrValue); break;
        case MDistance::kCentimeters: attrValue = UsdMayaUtil::ConvertMMToCM(attrValue); break;
        default:
            // The input is expected to be in millimeters.
            break;
        }

        status = plug.setFloat(attrValue);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    return true;
}

/* static */
bool UsdMayaTranslatorCamera::Read(
    const UsdGeomCamera&         usdCamera,
    MObject                      parentNode,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context)
{
    if (!usdCamera) {
        return false;
    }

    const UsdPrim& prim = usdCamera.GetPrim();
    const SdfPath  primPath = prim.GetPath();

    MStatus status;

    // Create the transform node for the camera.
    MObject transformObj;
    if (!UsdMayaTranslatorUtil::CreateTransformNode(
            prim, parentNode, args, context, &status, &transformObj)) {
        return false;
    }

    // Create the camera shape node.
    MDagModifier& dagMod = MayaUsd::MDagModifierUndoItem::create("Camera creation");
    MObject       cameraObj
        = dagMod.createNode(_tokens->MayaCameraTypeName.GetText(), transformObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);
    TF_VERIFY(!cameraObj.isNull());

    MFnCamera cameraFn(cameraObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    const std::string cameraShapeName
        = prim.GetName().GetString() + _tokens->MayaCameraShapeNameSuffix.GetString();
    cameraFn.setName(cameraShapeName.c_str(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    if (context) {
        const SdfPath shapePrimPath = primPath.AppendChild(TfToken(cameraShapeName));
        context->RegisterNewMayaNode(shapePrimPath.GetString(), cameraObj);
    }

    return _ReadToCamera(usdCamera, cameraFn, args, context);
}

/* static */
bool UsdMayaTranslatorCamera::ReadToCamera(const UsdGeomCamera& usdCamera, MFnCamera& cameraObject)
{
    VtDictionary userArgsDict;

    // Disable shading import since we're only interested in the camera.
    userArgsDict[UsdMayaJobImportArgsTokens->shadingMode] = std::vector<VtValue> { VtValue(
        std::vector<VtValue> { VtValue(UsdMayaShadingModeTokens->none.GetString()),
                               VtValue(std::string("default")) }) };

    UsdMayaJobImportArgs  importArgs = UsdMayaJobImportArgs::CreateFromDictionary(userArgsDict);
    UsdMayaPrimReaderArgs args(usdCamera.GetPrim(), importArgs);
    return _ReadToCamera(usdCamera, cameraObject, args, nullptr);
}

bool _ReadToCamera(
    const UsdGeomCamera&         usdCamera,
    MFnCamera&                   cameraFn,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context)
{
    MStatus status;

    // Now translate all of the USD camera attributes over to plugs on the
    // Maya cameraFn.
    UsdTimeCode  timeCode = UsdTimeCode::EarliestTime();
    UsdAttribute usdAttr;
    TfToken      plugName;

    // Set the type of projection. This is NOT keyable in Maya.
    TfToken projection;
    usdCamera.GetProjectionAttr().Get(&projection, timeCode);
    const bool isOrthographic = (projection == UsdGeomTokens->orthographic);
    status = cameraFn.setIsOrtho(isOrthographic);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Setup the aperture.
    usdAttr = usdCamera.GetHorizontalApertureAttr();
    plugName = _tokens->MayaCameraAttrNameHorizontalAperture;
    if (!_TranslateUsdAttributeToPlug(
            usdAttr,
            cameraFn,
            plugName,
            args,
            context,
            /* convertToUnit = */ MDistance::kInches)) {
        return false;
    }

    if (isOrthographic) {
        // For orthographic cameras, we'll re-use the horizontal aperture value
        // to fill in Maya's orthographicWidth. The film aperture and film
        // aperture offset plugs in Maya have no effect on orthographic cameras,
        // but we author them anyway so that the data is preserved. Note also
        // that Maya stores the orthographicWidth as centimeters.
        plugName = _tokens->MayaCameraAttrNameOrthographicWidth;
        if (!_TranslateUsdAttributeToPlug(
                usdAttr,
                cameraFn,
                plugName,
                args,
                context,
                /* convertToUnit = */ MDistance::kCentimeters)) {
            return false;
        }
    }

    usdAttr = usdCamera.GetVerticalApertureAttr();
    plugName = _tokens->MayaCameraAttrNameVerticalAperture;
    if (!_TranslateUsdAttributeToPlug(
            usdAttr,
            cameraFn,
            plugName,
            args,
            context,
            /* convertToUnit = */ MDistance::kInches)) {
        return false;
    }

    // XXX:
    // Lens Squeeze Ratio is DEPRECATED on USD schema.
    // Writing it out here for backwards compatibility (see bug 123124).
    cameraFn.setLensSqueezeRatio(1.0);

    usdAttr = usdCamera.GetHorizontalApertureOffsetAttr();
    plugName = _tokens->MayaCameraAttrNameHorizontalApertureOffset;
    if (!_TranslateUsdAttributeToPlug(
            usdAttr,
            cameraFn,
            plugName,
            args,
            context,
            /* convertToUnit = */ MDistance::kInches)) {
        return false;
    }

    usdAttr = usdCamera.GetVerticalApertureOffsetAttr();
    plugName = _tokens->MayaCameraAttrNameVerticalApertureOffset;
    if (!_TranslateUsdAttributeToPlug(
            usdAttr,
            cameraFn,
            plugName,
            args,
            context,
            /* convertToUnit = */ MDistance::kInches)) {
        return false;
    }

    // Set the lens parameters.
    usdAttr = usdCamera.GetFocalLengthAttr();
    plugName = _tokens->MayaCameraAttrNameFocalLength;
    if (!_TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName, args, context)) {
        return false;
    }

    usdAttr = usdCamera.GetFocusDistanceAttr();
    plugName = _tokens->MayaCameraAttrNameFocusDistance;
    if (!_TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName, args, context)) {
        return false;
    }

    // Convert USD fStop to Maya fStop respecting the USD notion that fStop=0 disables
    // depth of field.
    // TODO: Handle time-sampled fStop and possibly import/export a custom attribute for
    // fStop keyframe data in Maya. (Right now existance of samples or the USD
    // default value as zero is our signal)
    bool  enableMayaDOF = false;
    float usdFStop = 0;
    usdAttr = usdCamera.GetFStopAttr();
    if (usdAttr.IsAuthored()
        && (usdAttr.ValueMightBeTimeVarying() || (usdAttr.Get(&usdFStop) && usdFStop != 0))) {
        plugName = _tokens->MayaCameraAttrNameFStop;
        if (!_TranslateUsdAttributeToPlug(usdAttr, cameraFn, plugName, args, context)) {
            return false;
        }
        enableMayaDOF = true;
    }
    // Enable/Disable Maya camera's depthOfField
    plugName = _tokens->MayaCameraAttrNameDepthOfField;
    MPlug plug = cameraFn.findPlug(plugName.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = plug.setBool(enableMayaDOF);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Set the clipping planes. This one is a little different from the others
    // because it is stored in USD as a single GfVec2f value but in Maya as
    // separate nearClipPlane and farClipPlane attributes.
    usdAttr = usdCamera.GetClippingRangeAttr();
    MPlug nearClipPlug
        = cameraFn.findPlug(_tokens->MayaCameraAttrNameNearClippingPlane.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    MPlug farClipPlug
        = cameraFn.findPlug(_tokens->MayaCameraAttrNameFarClippingPlane.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    if (!_TranslateAnimatedUsdAttributeToPlugs(usdAttr, nearClipPlug, farClipPlug, args, context)) {
        GfVec2f clippingRange;
        usdCamera.GetClippingRangeAttr().Get(&clippingRange, timeCode);
        status = cameraFn.setNearClippingPlane(clippingRange[0]);
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = cameraFn.setFarClippingPlane(clippingRange[1]);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
