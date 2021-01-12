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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/camera.h>
#include <ufe/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to transform objects in 3D.
class MAYAUSD_CORE_PUBLIC UsdCamera : public Ufe::Camera
{
public:
    typedef std::shared_ptr<UsdCamera> Ptr;

    UsdCamera();
    UsdCamera(const UsdSceneItem::Ptr& item);
    ~UsdCamera() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdCamera(const UsdCamera&) = delete;
    UsdCamera& operator=(const UsdCamera&) = delete;
    UsdCamera(UsdCamera&&) = delete;
    UsdCamera& operator=(UsdCamera&&) = delete;

    //! Create a UsdCamera.
    static UsdCamera::Ptr create();
    static UsdCamera::Ptr create(const UsdSceneItem::Ptr& item);

    void           setItem(const UsdSceneItem::Ptr& item);
    inline UsdPrim prim() const
    {
        TF_AXIOM(fItem != nullptr);
        return fItem->prim();
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

private:
    UsdSceneItem::Ptr fItem;

}; // UsdCamera

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
