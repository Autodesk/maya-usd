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
#include <mayaUsd/fileio/utils/splineUtils.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#if PXR_VERSION >= 2411
#include <pxr/base/plug/registry.h>
#include <pxr/base/ts/spline.h>
#endif
#include <pxr/imaging/hd/camera.h>
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
        writeCameraSplinesAttrs(primSchema);
    }

    if (animType != UsdMayaJobExportArgsTokens->curves) {
        writeCameraAttrs(usdTime, primSchema);
    }
}

bool PxrUsdTranslators_CameraWriter::writeCameraSplinesAttrs(UsdGeomCamera& primSchema)
{
#if PXR_VERSION >= 2411
    MStatus   status;
    MFnCamera camFn(GetDagPath(), &status);
    CHECK_MSTATUS_AND_RETURN(status, false)

    auto cameraPrim = primSchema.GetPrim();
    auto primName = primSchema.GetPrim().GetPath().GetAsString();

    UsdMayaSplineUtils::WriteSplineAttribute<float>(
        camFn, cameraPrim, "focalLength", HdCameraTokens->focalLength, _metersPerUnitScalingFactor);

    UsdMayaSplineUtils::WriteSplineAttribute<float>(
        camFn,
        cameraPrim,
        "focusDistance",
        HdCameraTokens->focusDistance,
        _metersPerUnitScalingFactor);

    if (camFn.isDepthOfField()) {
        UsdMayaSplineUtils::WriteSplineAttribute<float>(
            camFn, cameraPrim, "fStop", HdCameraTokens->fStop);
    }

    // TODO: Clipping range is not yet supported in USD Spline (GfVec2f).
    // Set the clipping planes.
    GfVec2f clippingRange(camFn.nearClippingPlane(), camFn.farClippingPlane());
    UsdMayaWriteUtil::SetScaledAttribute(
        primSchema.GetClippingRangeAttr(),
        clippingRange,
        _metersPerUnitScalingFactor,
        UsdTimeCode::Default(),
        _GetSparseValueWriter());

    if (camFn.isOrtho(&status)) {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetProjectionAttr(),
            UsdGeomTokens->orthographic,
            UsdTimeCode::Default(),
            _GetSparseValueWriter());

        UsdMayaSplineUtils::WriteSplineAttribute<float>(
            camFn,
            cameraPrim,
            "orthographicWidth",
            HdCameraTokens->horizontalAperture,
            UsdMayaUtil::kMillimetersPerCentimeter * _metersPerUnitScalingFactor);

        auto vertAttr = primSchema.GetVerticalApertureAttr();
        auto horzApertureSpline = primSchema.GetHorizontalApertureAttr().GetSpline();
        if (!horzApertureSpline.IsEmpty()) {
            vertAttr.SetSpline(horzApertureSpline);
        } else {
            VtValue val;
            primSchema.GetHorizontalApertureAttr().Get(&val);
            vertAttr.Set(val);
        }

    } else {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetProjectionAttr(),
            UsdGeomTokens->perspective,
            UsdTimeCode::Default(),
            _GetSparseValueWriter());

        auto horzAttr = primSchema.GetHorizontalApertureAttr();

        std::function horzApertureLambda = [](float horzAperture, float lensSqueezeRatio) {
            return static_cast<float>(
                UsdMayaUtil::ConvertInchesToMM(horzAperture * lensSqueezeRatio));
        };
        TsSpline horzApertureSpline = UsdMayaSplineUtils::CombineMayaCurveToUsdSpline<float>(
            camFn, "horizontalFilmAperture", "lensSqueezeRatio", horzApertureLambda);

        if (horzApertureSpline.IsEmpty()) {
            double horizontalAperture = UsdMayaUtil::ConvertInchesToMM(
                camFn.horizontalFilmAperture() * camFn.lensSqueezeRatio());
            UsdMayaWriteUtil::SetScaledAttribute(
                horzAttr,
                static_cast<float>(horizontalAperture),
                _metersPerUnitScalingFactor,
                UsdTimeCode::Default(),
                _GetSparseValueWriter());
        } else {
            horzAttr.SetSpline(horzApertureSpline);
        }

        UsdMayaSplineUtils::WriteSplineAttribute<float>(
            camFn,
            cameraPrim,
            "verticalFilmAperture",
            HdCameraTokens->verticalAperture,
            UsdMayaUtil::ConvertInchesToMM(1) * _metersPerUnitScalingFactor);

        if (camFn.shakeEnabled()) {
            std::function shakeLambda = [](float apertureOffset, float shakeOffset) {
                return static_cast<float>(
                    UsdMayaUtil::ConvertInchesToMM(apertureOffset + shakeOffset));
            };

            auto     horzOffsetAttr = primSchema.GetHorizontalApertureOffsetAttr();
            TsSpline horzOffsetSpline = UsdMayaSplineUtils::CombineMayaCurveToUsdSpline<float>(
                camFn, "horizontalFilmOffset", "horizontalShake", shakeLambda);
            if (horzOffsetSpline.IsEmpty()) {
                const double horizontalApertureOffset = UsdMayaUtil::ConvertInchesToMM(
                    (camFn.shakeEnabled() ? camFn.horizontalFilmOffset() + camFn.horizontalShake()
                                          : camFn.horizontalFilmOffset()));
                UsdMayaWriteUtil::SetScaledAttribute(
                    horzOffsetAttr,
                    static_cast<float>(horizontalApertureOffset),
                    _metersPerUnitScalingFactor,
                    UsdTimeCode::Default(),
                    _GetSparseValueWriter());
            } else {
                horzOffsetAttr.SetSpline(horzOffsetSpline);
            }
            auto     vertOffsetAtt = primSchema.GetVerticalApertureOffsetAttr();
            TsSpline vertOffsetSpline = UsdMayaSplineUtils::CombineMayaCurveToUsdSpline<float>(
                camFn, "verticalFilmOffset", "verticalShake", shakeLambda);
            if (vertOffsetSpline.IsEmpty()) {
                const double verticalApertureOffset = UsdMayaUtil::ConvertInchesToMM(
                    camFn.shakeEnabled() ? camFn.verticalFilmOffset() + camFn.verticalShake()
                                         : camFn.verticalFilmOffset());
                UsdMayaWriteUtil::SetScaledAttribute(
                    vertOffsetAtt,
                    static_cast<float>(verticalApertureOffset),
                    _metersPerUnitScalingFactor,
                    UsdTimeCode::Default(),
                    _GetSparseValueWriter());
            } else {
                vertOffsetAtt.SetSpline(vertOffsetSpline);
            }
        } else {
            UsdMayaSplineUtils::WriteSplineAttribute<float>(
                camFn,
                cameraPrim,
                "horizontalFilmOffset",
                HdCameraTokens->horizontalApertureOffset,
                UsdMayaUtil::kMillimetersPerInch * _metersPerUnitScalingFactor);
            UsdMayaSplineUtils::WriteSplineAttribute<float>(
                camFn,
                cameraPrim,
                "verticalFilmOffset",
                HdCameraTokens->verticalApertureOffset,
                UsdMayaUtil::kMillimetersPerInch * _metersPerUnitScalingFactor);
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
        UsdMayaWriteUtil::SetScaledAttribute(
            primSchema.GetHorizontalApertureAttr(),
            static_cast<float>(orthoWidth),
            _metersPerUnitScalingFactor,
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetScaledAttribute(
            primSchema.GetVerticalApertureAttr(),
            static_cast<float>(orthoWidth),
            _metersPerUnitScalingFactor,
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

        UsdMayaWriteUtil::SetScaledAttribute(
            primSchema.GetHorizontalApertureAttr(),
            static_cast<float>(horizontalAperture),
            _metersPerUnitScalingFactor,
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetScaledAttribute(
            primSchema.GetVerticalApertureAttr(),
            static_cast<float>(verticalAperture),
            _metersPerUnitScalingFactor,
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetScaledAttribute(
            primSchema.GetHorizontalApertureOffsetAttr(),
            static_cast<float>(horizontalApertureOffset),
            _metersPerUnitScalingFactor,
            usdTime,
            _GetSparseValueWriter());

        UsdMayaWriteUtil::SetScaledAttribute(
            primSchema.GetVerticalApertureOffsetAttr(),
            static_cast<float>(verticalApertureOffset),
            _metersPerUnitScalingFactor,
            usdTime,
            _GetSparseValueWriter());
    }

    // Set the lens parameters.
    UsdMayaWriteUtil::SetScaledAttribute(
        primSchema.GetFocalLengthAttr(),
        static_cast<float>(camFn.focalLength()),
        _metersPerUnitScalingFactor,
        usdTime,
        _GetSparseValueWriter());

    // Always export focus distance regardless of what
    // camFn.isDepthOfField() says. Downstream tools can choose to ignore or
    // override it.
    UsdMayaWriteUtil::SetScaledAttribute(
        primSchema.GetFocusDistanceAttr(),
        static_cast<float>(camFn.focusDistance()),
        _metersPerUnitScalingFactor,
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
    UsdMayaWriteUtil::SetScaledAttribute(
        primSchema.GetClippingRangeAttr(),
        clippingRange,
        _metersPerUnitScalingFactor,
        usdTime,
        _GetSparseValueWriter());

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
