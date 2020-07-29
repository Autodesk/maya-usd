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
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnCamera.h>
#include <maya/MFnDependencyNode.h>

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
    writeCameraAttrs(usdTime, primSchema);
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

    // Always export focus distance and fStop regardless of what
    // camFn.isDepthOfField() says. Downstream tools can choose to ignore or
    // override them.
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetFocusDistanceAttr(),
        static_cast<float>(camFn.focusDistance()),
        usdTime,
        _GetSparseValueWriter());

    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetFStopAttr(),
        static_cast<float>(camFn.fStop()),
        usdTime,
        _GetSparseValueWriter());

    // Set the clipping planes.
    GfVec2f clippingRange(camFn.nearClippingPlane(), camFn.farClippingPlane());
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetClippingRangeAttr(), clippingRange, usdTime, _GetSparseValueWriter());

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
