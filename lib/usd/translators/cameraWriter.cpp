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
#include "cameraWriter.h"

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#if PXR_VERSION > 2411
#include <pxr/base/plug/registry.h>
#include <pxr/base/ts/spline.h>
#endif
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnAnimCurve.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(camera, PxrUsdTranslators_CameraWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(camera, UsdGeomCamera);

PxrUsdTranslators_CameraWriter::PxrUsdTranslators_CameraWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    if (!TF_VERIFY(GetDagPath().isValid())) {
        return;
    }

    UsdGeomCamera primSchema = UsdGeomCamera::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            primSchema, "Could not define UsdGeomCamera at path '%s'\n", GetUsdPath().GetText())) {
        return;
    }
    _usdPrim = primSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdGeomCamera at path '%s'\n",
            primSchema.GetPath().GetText())) {
        return;
    }
}

/* virtual */
void PxrUsdTranslators_CameraWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaPrimWriter::Write(usdTime);

    UsdGeomCamera primSchema(_usdPrim);

    auto animType = _writeJobCtx.GetArgs().animationType;
    if (usdTime.IsDefault() && animType != UsdMayaJobExportArgsTokens->timesamples) {
        writeCameraSplinesAttrs(primSchema, _writeJobCtx.GetArgs().timeSamples);
    }

    if (animType != UsdMayaJobExportArgsTokens->curves) {
        writeCameraAttrs(usdTime, primSchema);
    }
}

bool PxrUsdTranslators_CameraWriter::writeCameraSplinesAttrs(
    UsdGeomCamera&             primSchema,
    const std::vector<double>& timeSamples)
{
#if PXR_VERSION > 2411
    MStatus   status;
    MFnCamera camFn(GetDagPath(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false)

    auto primName = primSchema.GetPrim().GetPath().GetAsString();

    auto focalLengthAttr = primSchema.GetFocalLengthAttr();
    UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
        camFn, "focalLength", focalLengthAttr, timeSamples);

    auto focusDistanceAttr = primSchema.GetFocusDistanceAttr();
    UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
        camFn, "focusDistance", focusDistanceAttr, timeSamples);

    if (camFn.isDepthOfField()) {
        auto depthOfFieldAttr = primSchema.GetFStopAttr();
        UsdMayaWriteUtil::CreateSplineFromPlugToAttr(camFn, "fStop", depthOfFieldAttr, timeSamples);
    }

    // TODO: Clipping range is not yet supported in USD Spline curves.

    if (camFn.isOrtho(&status)) {

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetProjectionAttr(),
            UsdGeomTokens->orthographic,
            UsdTimeCode::Default(),
            _GetSparseValueWriter());

        auto projectionAttr = primSchema.GetProjectionAttr();
        if (!UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
                camFn,
                "fStop",
                projectionAttr,
                timeSamples,
                UsdMayaUtil::kMillimetersPerCentimeter)) {
            MGlobal::displayError(
                TfStringPrintf("Error adding spline attribute to fStop on prim '%s'.", primName)
                    .c_str());
        }

        auto horizontalApertureAttr = primSchema.GetHorizontalApertureAttr();
        if (!UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
                camFn,
                "orthoWidth",
                horizontalApertureAttr,
                timeSamples,
                UsdMayaUtil::kMillimetersPerCentimeter)) {
            MGlobal::displayError(
                TfStringPrintf(
                    "Error adding spline attribute to horizontalAperture on prim '%s'.", primName)
                    .c_str());
        }

        auto verticalApertureAttr = primSchema.GetVerticalApertureAttr();
        if (!UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
                camFn,
                "orthoWidth",
                verticalApertureAttr,
                timeSamples,
                UsdMayaUtil::kMillimetersPerCentimeter)) {
            MGlobal::displayError(
                TfStringPrintf(
                    "Error adding spline attribute to verticalAperture on prim '%s'.", primName)
                    .c_str());
        }
    } else {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetProjectionAttr(),
            UsdGeomTokens->perspective,
            UsdTimeCode::Default(),
            _GetSparseValueWriter());

        auto horizontalApertureAttr = primSchema.GetHorizontalApertureAttr();
        if (UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
                camFn, "horizontalFilmAperture", horizontalApertureAttr, timeSamples)) {

            MPlug        lensSqueezePlug = camFn.findPlug("lensSqueezeRatio", true, &status);
            MFnAnimCurve lensSqueezeCurve(lensSqueezePlug, &status);

            // Horizontal aperture needs to be multiplied by the lens squeeze ratio.
            TsSpline horizontalApertureSpline = horizontalApertureAttr.GetSpline();
            auto     knots = horizontalApertureSpline.GetKnots();
            for (auto knot : knots) {
                VtValue value;
                auto    time = knot.GetTime();
                knot.GetValue(&value);

                float squeezeRatio = static_cast<float>(lensSqueezeCurve.evaluate(time));
                knot.SetValue(UsdMayaUtil::ConvertInchesToMM(value.Get<float>() * squeezeRatio));
            }

            horizontalApertureSpline.ClearKnots();
            horizontalApertureSpline.SetKnots(knots);
            if (!horizontalApertureAttr.SetSpline(horizontalApertureSpline)) {
                MGlobal::displayError(TfStringPrintf(
                                          "Error adding spline attribute to perspective "
                                          "horizontalFilmAperture on prim '%s'.",
                                          primName)
                                          .c_str());
            }
        }

        auto verticalApertureAttr = primSchema.GetVerticalApertureAttr();
        UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
            camFn,
            "verticalFilmAperture",
            verticalApertureAttr,
            timeSamples,
            UsdMayaUtil::kMillimetersPerCentimeter);

        const bool hasCameraShake = camFn.shakeEnabled();
        auto       horizontalApertureOffsetAttr = primSchema.GetHorizontalApertureOffsetAttr();
        if (UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
                camFn, "horizontalFilmOffset", horizontalApertureOffsetAttr, timeSamples)) {

            MPlug        horizontalShakePlug = camFn.findPlug("horizontalShake", true, &status);
            MFnAnimCurve horizontalShakeCurve(horizontalShakePlug, &status);

            MPlug horizontalFilmOffsetPlug = camFn.findPlug("horizontalFilmOffset", true, &status);
            MFnAnimCurve horizontalFilmOffsetCurve(horizontalFilmOffsetPlug, &status);

            TsSpline horizontalApertureOffsetSpline = horizontalApertureOffsetAttr.GetSpline();
            auto     knots = horizontalApertureOffsetSpline.GetKnots();
            for (auto knot : knots) {
                VtValue v;
                auto    time = knot.GetTime();
                knot.GetValue(&v);

                // If the camera has camera shake, make sure to add that to the camera offset.
                float shakeOffset = hasCameraShake ? horizontalShakeCurve.evaluate(time) : 0.f;

                knot.SetValue(UsdMayaUtil::ConvertInchesToMM(v.Get<float>() + shakeOffset));
            }

            horizontalApertureOffsetSpline.ClearKnots();
            horizontalApertureOffsetSpline.SetKnots(knots);
            if (!horizontalApertureAttr.SetSpline(horizontalApertureOffsetSpline)) {
                MGlobal::displayError(TfStringPrintf(
                                          "Error adding spline attribute to perspective "
                                          "horizontalFilmOffset on prim '%s'.",
                                          primName)
                                          .c_str());
            }
        }

        auto verticalApertureOffsetAttr = primSchema.GetVerticalApertureOffsetAttr();
        if (UsdMayaWriteUtil::CreateSplineFromPlugToAttr(
                camFn, "verticalFilmOffset", verticalApertureOffsetAttr, timeSamples)) {

            MPlug        verticalShakePlug = camFn.findPlug("verticalShake", true, &status);
            MFnAnimCurve verticalShakeCurve(verticalShakePlug, &status);

            MPlug verticalFilmOffsetPlug = camFn.findPlug("verticalFilmOffset", true, &status);
            MFnAnimCurve verticalFilmOffsetCurve(verticalFilmOffsetPlug, &status);

            TsSpline verticalApertureOffsetSpline = verticalApertureOffsetAttr.GetSpline();
            auto     knots = verticalApertureOffsetSpline.GetKnots();
            for (auto knot : knots) {
                VtValue v;
                auto    time = knot.GetTime();
                knot.GetValue(&v);

                // If the camera has camera shake, make sure to add that to the camera offset.
                float shakeOffset = hasCameraShake ? verticalShakeCurve.evaluate(time) : 0.f;

                knot.SetValue(UsdMayaUtil::ConvertInchesToMM(v.Get<float>() + shakeOffset));
            }

            verticalApertureOffsetSpline.ClearKnots();
            verticalApertureOffsetSpline.SetKnots(knots);
            if (!horizontalApertureAttr.SetSpline(verticalApertureOffsetSpline)) {
                MGlobal::displayError(TfStringPrintf(
                                          "Error adding spline attribute to perspective "
                                          "horizontalFilmOffset on prim '%s'.",
                                          primName)
                                          .c_str());
            }
        }
    }
#endif
    return true;
}

bool PxrUsdTranslators_CameraWriter::writeCameraAttrs(
    const UsdTimeCode& usdTime,
    UsdGeomCamera&     primSchema)
{
    // Since write() above will take care of any animation on the camera's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return true;
    }

    MStatus status;

    MFnCamera camFn(GetDagPath(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // NOTE: We do not use a GfCamera and then call SetFromCamera() below
    // because we want the xformOps populated by the parent class to survive.
    // Using SetFromCamera() would stomp them with a single "transform" xformOp.

    if (camFn.isOrtho()) {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetProjectionAttr(),
            UsdGeomTokens->orthographic,
            usdTime,
            _GetSparseValueWriter());

        // Contrary to the documentation, Maya actually stores the orthographic
        // width in centimeters (Maya's internal unit system), not inches.
        const double orthoWidth = UsdMayaUtil::ConvertCMToMM(camFn.orthoWidth());

        // It doesn't seem to be possible to specify a non-square orthographic
        // camera in Maya, and aspect ratio, lens squeeze ratio, and film
        // offset have no effect.
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetHorizontalApertureAttr(),
            static_cast<float>(orthoWidth),
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetVerticalApertureAttr(),
            static_cast<float>(orthoWidth),
            usdTime,
            _GetSparseValueWriter());
    } else {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetProjectionAttr(),
            UsdGeomTokens->perspective,
            usdTime,
            _GetSparseValueWriter());

        // Lens squeeze ratio applies horizontally only.
        const double horizontalAperture = UsdMayaUtil::ConvertInchesToMM(
            camFn.horizontalFilmAperture() * camFn.lensSqueezeRatio());
        const double verticalAperture
            = UsdMayaUtil::ConvertInchesToMM(camFn.verticalFilmAperture());

        // Film offset and shake (when enabled) have the same effect on film back
        const double horizontalApertureOffset = UsdMayaUtil::ConvertInchesToMM(
            (camFn.shakeEnabled() ? camFn.horizontalFilmOffset() + camFn.horizontalShake()
                                  : camFn.horizontalFilmOffset()));
        const double verticalApertureOffset = UsdMayaUtil::ConvertInchesToMM(
            (camFn.shakeEnabled() ? camFn.verticalFilmOffset() + camFn.verticalShake()
                                  : camFn.verticalFilmOffset()));

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetHorizontalApertureAttr(),
            static_cast<float>(horizontalAperture),
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetVerticalApertureAttr(),
            static_cast<float>(verticalAperture),
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetHorizontalApertureOffsetAttr(),
            static_cast<float>(horizontalApertureOffset),
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetVerticalApertureOffsetAttr(),
            static_cast<float>(verticalApertureOffset),
            usdTime,
            _GetSparseValueWriter());
    }

    // Set the lens parameters.
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetFocalLengthAttr(),
        static_cast<float>(camFn.focalLength()),
        usdTime,
        _GetSparseValueWriter());

    // Always export focus distance regardless of what
    // camFn.isDepthOfField() says. Downstream tools can choose to ignore or
    // override it.
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetFocusDistanceAttr(),
        static_cast<float>(camFn.focusDistance()),
        usdTime,
        _GetSparseValueWriter());

    // USD specifies fStop=0 to disable depth-of-field, so we have to honor that by
    // munging isDepthOfField and fStop together.
    // XXX: Should an additional custom maya-namespaced attribute write the actual value?
    if (camFn.isDepthOfField()) {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetFStopAttr(),
            static_cast<float>(camFn.fStop()),
            usdTime,
            _GetSparseValueWriter());
    }

    // Set the clipping planes.
    GfVec2f clippingRange(camFn.nearClippingPlane(), camFn.farClippingPlane());
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetClippingRangeAttr(), clippingRange, usdTime, _GetSparseValueWriter());

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
