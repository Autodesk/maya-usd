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

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

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

//------------------------------------------------------------------------------
// Ufe::Camera overrides
//------------------------------------------------------------------------------

const Ufe::Path& UsdCamera::path() const { return fItem->path(); }

Ufe::SceneItem::Ptr UsdCamera::sceneItem() const { return fItem; }

Ufe::HorizontalApertureUndoableCommand::Ptr
UsdCamera::horizontalApertureCmd(float horizontalAperture)
{
    return nullptr;
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

Ufe::VerticalApertureUndoableCommand::Ptr UsdCamera::verticalApertureCmd(float verticalAperture)
{
    return nullptr;
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

Ufe::HorizontalApertureOffsetUndoableCommand::Ptr UsdCamera::horizontalApertureOffsetCmd(float)
{
    return nullptr;
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

Ufe::VerticalApertureOffsetUndoableCommand::Ptr UsdCamera::verticalApertureOffsetCmd(float)
{
    return nullptr;
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

Ufe::FStopUndoableCommand::Ptr UsdCamera::fStopCmd(float) { return nullptr; }

float UsdCamera::fStop() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies fStop in stage units. Store the fStop in stage units.
    float fStop = gfCamera.GetFStop();

    // Convert the fStop value to mm, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(fStop, stageUnits, UsdGeomLinearUnits::millimeters);
}

Ufe::FocalLengthUndoableCommand::Ptr UsdCamera::focalLengthCmd(float) { return nullptr; }

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

Ufe::FocusDistanceUndoableCommand::Ptr UsdCamera::focusDistanceCmd(float) { return nullptr; }

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

Ufe::NearClipPlaneUndoableCommand::Ptr UsdCamera::nearClipPlaneCmd(float) { return nullptr; }

float UsdCamera::nearClipPlane() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies near clip plane in stage units. Store the near clip plane in
    // stage units.
    GfRange1f clippingRange = gfCamera.GetClippingRange();
    float     nearClipPlane = clippingRange.GetMin();

    // Convert the near clip plane value to cm, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(nearClipPlane, stageUnits, UsdGeomLinearUnits::centimeters);
}

Ufe::FarClipPlaneUndoableCommand::Ptr UsdCamera::farClipPlaneCmd(float) { return nullptr; }

float UsdCamera::farClipPlane() const
{
    // inspired by UsdImagingCameraAdapter::UpdateForTime
    UsdGeomCamera usdGeomCamera(prim());
    GfCamera      gfCamera = usdGeomCamera.GetCamera(getTime(sceneItem()->path()));
    // The USD schema specifies far clip plane in stage units. Store the far clip plane in
    // stage units.
    GfRange1f clippingRange = gfCamera.GetClippingRange();
    float     farClipPlane = clippingRange.GetMax();

    // Convert the far clip plane value to cm, the return unit of this function.

    // Figure out the stage unit
    UsdStageWeakPtr stage = prim().GetStage();

    double stageUnits = UsdGeomLinearUnits::centimeters;
    if (UsdGeomStageHasAuthoredMetersPerUnit(stage)) {
        stageUnits = UsdGeomGetStageMetersPerUnit(stage);
    }

    return UsdMayaUtil::ConvertUnit(farClipPlane, stageUnits, UsdGeomLinearUnits::centimeters);
}

Ufe::ProjectionUndoableCommand::Ptr UsdCamera::projectionCmd(Ufe::Camera::Projection projection)
{
    return nullptr;
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
