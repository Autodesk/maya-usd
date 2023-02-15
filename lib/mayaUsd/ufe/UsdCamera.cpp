//
// Copyright 2020 Autodesk
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
#include "UsdCamera.h"

#include "private/Utils.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/metrics.h"

#include <mayaUsd/ufe/UsdUndoableCommand.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/imaging/hd/camera.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdCamera::UsdCamera()
    : Camera()
{
}

UsdCamera::UsdCamera(const UsdSceneItem::Ptr& item)
    : Camera()
    , fItem(item)
{
}

/* static */
UsdCamera::Ptr UsdCamera::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdCamera>(item);
}

/* static */
bool UsdCamera::isCameraToken(const PXR_NS::TfToken& token)
{
    static std::unordered_set<PXR_NS::TfToken, PXR_NS::TfToken::HashFunctor> cameraTokens {
        HdCameraTokens->horizontalAperture,
        HdCameraTokens->verticalAperture,
        HdCameraTokens->horizontalApertureOffset,
        HdCameraTokens->verticalApertureOffset,
        HdCameraTokens->focalLength,
        HdCameraTokens->clippingRange,
        HdCameraTokens->fStop
    };
    // There are more HdCameraTokens that Maya ignores:
    // worldToViewMatrix, projectionMatrix, clipPlanes, windowPolicy, shutterOpen,
    // shutterClose

    return cameraTokens.count(token) > 0;
}

//------------------------------------------------------------------------------
// Ufe::Camera overrides
//------------------------------------------------------------------------------

namespace {

float convertToStageUnits(float value, double valueUnits, const PXR_NS::UsdPrim& prim)
{
    // Figure out the stage unit
    UsdStageWeakPtr stage = prim.GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(value, valueUnits, stageUnits);
}

float convertToTenthOfStageUnits(float value, double valueUnits, const PXR_NS::UsdPrim& prim)
{
    // Tenth of units means the values are ten times greater.
    // For example, if the stage units is cm, then 10th of stage units is mm.
    // So 1cm becomes 10mm, multiplying the value by 10.
    return 10.0f * convertToStageUnits(value, valueUnits, prim);
}

} // namespace

const Ufe::Path& UsdCamera::path() const { return fItem->path(); }

Ufe::SceneItem::Ptr UsdCamera::sceneItem() const { return fItem; }

Ufe::HorizontalApertureUndoableCommand::Ptr UsdCamera::horizontalApertureCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this horizontal aperture function are inches.
        // The USD schema specifies horizontal aperture in tenths of a stage unit.
        const float convertedValue
            = convertToTenthOfStageUnits(value, UsdGeomLinearUnits::inches, prim());
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateHorizontalApertureAttr();
        return attr.Set<float>(convertedValue);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::HorizontalApertureUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::horizontalAperture() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies horizontal aperture in tenths of a stage unit. Store
    // the horizontal aperture in stage units.
    float horizontalAperture = gfCamera.GetHorizontalAperture() / 10.0f;

    // Convert the horizontal aperture value to inches, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(horizontalAperture, stageUnits, UsdGeomLinearUnits::inches);
}

Ufe::VerticalApertureUndoableCommand::Ptr UsdCamera::verticalApertureCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this vertical aperture function are inches.
        // The USD schema specifies vertical aperture in tenths of a stage unit.
        const float convertedValue
            = convertToTenthOfStageUnits(value, UsdGeomLinearUnits::inches, prim());
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateVerticalApertureAttr();
        return attr.Set<float>(convertedValue);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::VerticalApertureUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::verticalAperture() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies vertical aperture in tenths of a stage unit. Store
    // the vertical aperture in stage units.
    float verticalAperture = gfCamera.GetVerticalAperture() / 10.0f;

    // Convert the vertical aperture value to inches, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(verticalAperture, stageUnits, UsdGeomLinearUnits::inches);
}

Ufe::HorizontalApertureOffsetUndoableCommand::Ptr
UsdCamera::horizontalApertureOffsetCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this horizontal aperture offset function are inches.
        // The USD schema specifies horizontal aperture offset in tenths of a stage unit.
        const float convertedValue
            = convertToTenthOfStageUnits(value, UsdGeomLinearUnits::inches, prim());
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateHorizontalApertureOffsetAttr();
        return attr.Set<float>(convertedValue);
    };

    return std::make_shared<
        UsdFunctionUndoableSetCommand<Ufe::HorizontalApertureOffsetUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::horizontalApertureOffset() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies horizontal aperture offset in tenths of a stage unit. Store
    // the horizontal aperture offset in stage units.
    float horizontalApertureOffset = gfCamera.GetHorizontalApertureOffset() / 10.0f;

    // Convert the horizontal aperture offset value to inches, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(
        horizontalApertureOffset, stageUnits, UsdGeomLinearUnits::inches);
}

Ufe::VerticalApertureOffsetUndoableCommand::Ptr UsdCamera::verticalApertureOffsetCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this vertical aperture offset function are inches.
        // The USD schema specifies vertical aperture offset in tenths of a stage unit.
        const float convertedValue
            = convertToTenthOfStageUnits(value, UsdGeomLinearUnits::inches, prim());
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateVerticalApertureOffsetAttr();
        return attr.Set<float>(convertedValue);
    };

    return std::make_shared<
        UsdFunctionUndoableSetCommand<Ufe::VerticalApertureOffsetUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::verticalApertureOffset() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies vertical aperture offset in tenths of a stage unit. Store
    // the vertical aperture offset in stage units.
    float verticalApertureOffset = gfCamera.GetVerticalApertureOffset() / 10.0f;

    // Convert the vertical aperture offset value to inches, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(verticalApertureOffset, stageUnits, UsdGeomLinearUnits::inches);
}

Ufe::FStopUndoableCommand::Ptr UsdCamera::fStopCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this fStop function are mm.
        // The USD schema specifies fStop in stage unit.
        //
        // TODO: Actually, the UsdGeomCamera docs fails to mention units...
        //       Moreover, it makes no sense for fstops to have units?
        const float convertedValue
            = convertToStageUnits(value, UsdGeomLinearUnits::millimeters, prim());
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateFStopAttr();
        return attr.Set<float>(convertedValue);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::FStopUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::fStop() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies fStop in stage units. Store the fStop in stage units.
    //
    // TODO: Actually, the UsdGeomCamera docs fails to mention units...
    //       Moreover, it makes no sense for fstops to have units?
    float fStop = gfCamera.GetFStop();

    // Convert the fStop value to mm, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

#if MAYA_API_VERSION >= 20220100
    return UsdMayaUtil::ConvertUnit(fStop, stageUnits, UsdGeomLinearUnits::millimeters);
#else
    float retVal = UsdMayaUtil::ConvertUnit(fStop, stageUnits, UsdGeomLinearUnits::millimeters);
    return retVal < FLT_EPSILON ? FLT_EPSILON : retVal;
#endif
}

Ufe::FocalLengthUndoableCommand::Ptr UsdCamera::focalLengthCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this focal length function are mm.
        // The USD schema specifies focal length in tenths of a stage unit.
        const float convertedValue
            = convertToTenthOfStageUnits(value, UsdGeomLinearUnits::millimeters, prim());
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateFocalLengthAttr();
        return attr.Set<float>(convertedValue);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::FocalLengthUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::focalLength() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies focal length in tenths of a stage unit. Store the focal length in
    // stage units.
    float focalLength = gfCamera.GetFocalLength() / 10.0f;

    // Convert the focal length value to mm, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(focalLength, stageUnits, UsdGeomLinearUnits::millimeters);
}

Ufe::FocusDistanceUndoableCommand::Ptr UsdCamera::focusDistanceCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this focus distance function are cm.
        // The USD schema specifies focus distance in stage units.
        const float convertedValue
            = convertToStageUnits(value, UsdGeomLinearUnits::centimeters, prim());
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateFocusDistanceAttr();
        return attr.Set<float>(convertedValue);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::FocusDistanceUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::focusDistance() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies focus distance in stage units. Store the focus distance in
    // stage units.
    float focusDistance = gfCamera.GetFocusDistance();

    // Convert the focus distance value to cm, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(focusDistance, stageUnits, UsdGeomLinearUnits::centimeters);
}

Ufe::NearClipPlaneUndoableCommand::Ptr UsdCamera::nearClipPlaneCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this near clip plane function are unspecified.
        // The USD schema specifies near clip plane in stage units.
        // Since UFE does not specify units, we do no conversion.
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateClippingRangeAttr();

        GfVec2f range;
        attr.Get<GfVec2f>(&range);
        range[0] = value;

        return attr.Set<GfVec2f>(range);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::NearClipPlaneUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::nearClipPlane() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies near clip plane in stage units. Store the near clip plane in
    // stage units.
    GfRange1f clippingRange = gfCamera.GetClippingRange();
    float     nearClipPlane = clippingRange.GetMin();

    // Ufe doesn't convert linear units for prim size or translation, so don't convert the
    // clipping plane.

    return nearClipPlane;
}

Ufe::FarClipPlaneUndoableCommand::Ptr UsdCamera::farClipPlaneCmd(float value)
{
    auto command = [this, value]() -> bool {
        // The unit of this near clip plane function are unspecified.
        // The USD schema specifies near clip plane in stage units.
        // Since UFE does not specify units, we do no conversion.
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateClippingRangeAttr();

        GfVec2f range;
        attr.Get<GfVec2f>(&range);
        range[1] = value;

        return attr.Set<GfVec2f>(range);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::FarClipPlaneUndoableCommand>>(
        command, sceneItem()->path());
}

float UsdCamera::farClipPlane() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies far clip plane in stage units. Store the far clip plane in
    // stage units.
    GfRange1f clippingRange = gfCamera.GetClippingRange();
    float     farClipPlane = clippingRange.GetMax();

    // Ufe doesn't convert linear units for prim size or translation, so don't convert the
    // clipping plane.

    return farClipPlane;
}

Ufe::ProjectionUndoableCommand::Ptr UsdCamera::projectionCmd(Ufe::Camera::Projection projection)
{
    auto command = [this, projection]() -> bool {
        const TfToken value = projection == Ufe::Camera::Orthographic
            ? PXR_NS::UsdGeomTokens->orthographic
            : PXR_NS::UsdGeomTokens->perspective;
        PXR_NS::UsdGeomCamera usdGeomCamera(prim());
        PXR_NS::UsdAttribute  attr = usdGeomCamera.CreateProjectionAttr();
        return attr.Set<TfToken>(value);
    };

    return std::make_shared<UsdFunctionUndoableSetCommand<Ufe::ProjectionUndoableCommand>>(
        command, sceneItem()->path());
}

Ufe::Camera::Projection UsdCamera::projection() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifics some camera parameters is tenths of a world
    // unit (e.g., focalLength = 50mm). Convert to world units.
    if (GfCamera::Orthographic == gfCamera.GetProjection()) {
        return Ufe::Camera::Orthographic;
    }
    return Ufe::Camera::Perspective;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
