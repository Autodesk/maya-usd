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
#ifndef USDUFE_USDCAMERA_H
#define USDUFE_USDCAMERA_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/camera.h>
#include <ufe/path.h>

namespace USDUFE_NS_DEF {

//! \brief Interface to transform objects in 3D.
class USDUFE_PUBLIC UsdCamera : public Ufe::Camera
{
public:
    typedef std::shared_ptr<UsdCamera> Ptr;

    UsdCamera() = default;
    UsdCamera(const UsdSceneItem::Ptr& item);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdCamera);

    //! Create a UsdCamera.
    static UsdCamera::Ptr create(const UsdSceneItem::Ptr& item);

    static bool isCameraToken(const PXR_NS::TfToken&);

    inline PXR_NS::UsdPrim prim() const
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        if (TF_VERIFY(_item != nullptr))
            return _item->prim();
        else
            return PXR_NS::UsdPrim();
    }

    // Ufe::Camera overrides
    const Ufe::Path&    path() const override;
    Ufe::SceneItem::Ptr sceneItem() const override;

    Ufe::HorizontalApertureUndoableCommand::Ptr horizontalApertureCmd(float) override;
    float                                       horizontalAperture() const override;

    Ufe::VerticalApertureUndoableCommand::Ptr verticalApertureCmd(float) override;
    float                                     verticalAperture() const override;

    Ufe::HorizontalApertureOffsetUndoableCommand::Ptr horizontalApertureOffsetCmd(float) override;
    float                                             horizontalApertureOffset() const override;

    Ufe::VerticalApertureOffsetUndoableCommand::Ptr verticalApertureOffsetCmd(float) override;
    float                                           verticalApertureOffset() const override;

    Ufe::FStopUndoableCommand::Ptr fStopCmd(float) override;
    float                          fStop() const override;

    Ufe::FocalLengthUndoableCommand::Ptr focalLengthCmd(float) override;
    float                                focalLength() const override;

    Ufe::FocusDistanceUndoableCommand::Ptr focusDistanceCmd(float) override;
    float                                  focusDistance() const override;

    Ufe::NearClipPlaneUndoableCommand::Ptr nearClipPlaneCmd(float) override;
    float                                  nearClipPlane() const override;

    Ufe::FarClipPlaneUndoableCommand::Ptr farClipPlaneCmd(float) override;
    float                                 farClipPlane() const override;

    Ufe::ProjectionUndoableCommand::Ptr projectionCmd(Ufe::Camera::Projection projection) override;
    Ufe::Camera::Projection             projection() const override;

#ifdef UFE_CAMERA_HAS_RENDERABLE
    bool renderable() const override;
#endif // UFE_CAMERA_HAS_RENDERABLE
private:
    UsdSceneItem::Ptr _item;

}; // UsdCamera

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDCAMERA_H
