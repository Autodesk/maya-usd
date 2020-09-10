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

#include <ufe/transform3d.h>
#include <ufe/transform3dHandler.h>

#include <pxr/usd/usd/prim.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Read-only implementation for USD object 3D transform information.
// 
// Methods in the interface which return a command to change the object's 3D
// transformation return a null pointer.
//
// Methods in the interface which change the object directly raise an exception.
//
// Note that all calls to specify time use the default time, but this
// could be changed to use the current time, using getTime(path()).

class MAYAUSD_CORE_PUBLIC UsdTransform3dBase : public Ufe::Transform3d
{
public:
    typedef std::shared_ptr<UsdTransform3dBase> Ptr;

    UsdTransform3dBase(const UsdSceneItem::Ptr& item);
    ~UsdTransform3dBase() override = default;

    // Delete the copy/move constructors assignment operators.
    UsdTransform3dBase(const UsdTransform3dBase&) = delete;
    UsdTransform3dBase& operator=(const UsdTransform3dBase&) = delete;
    UsdTransform3dBase(UsdTransform3dBase&&) = delete;
    UsdTransform3dBase& operator=(UsdTransform3dBase&&) = delete;

    //! Create a UsdTransform3dBase.
    static UsdTransform3dBase::Ptr create(const UsdSceneItem::Ptr& item);

    // Ufe::Transform3d overrides
    const Ufe::Path& path() const override;
    Ufe::SceneItem::Ptr sceneItem() const override;

    inline UsdSceneItem::Ptr usdSceneItem() const { return fItem; }
    inline UsdPrim prim() const { return fPrim; }

    Ufe::Vector3d translation() const override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;

    Ufe::TranslateUndoableCommand::Ptr translateCmd(double x, double y, double z) override;
    Ufe::RotateUndoableCommand::Ptr rotateCmd(double x, double y, double z) override;
    Ufe::ScaleUndoableCommand::Ptr scaleCmd(double x, double y, double z) override;
    void translate(double x, double y, double z) override;
    void rotate(double x, double y, double z) override;
    void scale(double x, double y, double z) override;

    Ufe::TranslateUndoableCommand::Ptr rotatePivotTranslateCmd() override;
    void rotatePivotTranslate(double x, double y, double z) override;
    Ufe::Vector3d rotatePivot() const override;

    Ufe::TranslateUndoableCommand::Ptr scalePivotTranslateCmd() override;
    void scalePivotTranslate(double x, double y, double z) override;
    Ufe::Vector3d scalePivot() const override;

#if UFE_PREVIEW_VERSION_NUM >= 2021
    Ufe::SetMatrixUndoableCommand::Ptr setMatrixCmd(const Ufe::Matrix4d& m) override;
    Ufe::Matrix4d matrix() const override;
#endif

    Ufe::Matrix4d segmentInclusiveMatrix() const override;
    Ufe::Matrix4d segmentExclusiveMatrix() const override;

protected:
    const char* pathCStr() const;

private:

    UsdSceneItem::Ptr fItem;
    UsdPrim fPrim;

}; // UsdTransform3dBase

//! \brief Factory to create a UsdTransform3dBase interface object.
//
// 
class MAYAUSD_CORE_PUBLIC UsdTransform3dBaseHandler : public Ufe::Transform3dHandler
{
public:
    typedef std::shared_ptr<UsdTransform3dBaseHandler> Ptr;

    UsdTransform3dBaseHandler();

    // Delete the copy/move constructors assignment operators.
    UsdTransform3dBaseHandler(const UsdTransform3dBaseHandler&) = delete;
    UsdTransform3dBaseHandler& operator=(const UsdTransform3dBaseHandler&) = delete;
    UsdTransform3dBaseHandler(UsdTransform3dBaseHandler&&) = delete;
    UsdTransform3dBaseHandler& operator=(UsdTransform3dBaseHandler&&) = delete;

    //! Create a UsdTransform3dBaseHandler.
    static Ptr create();

    // Ufe::Transform3dHandler overrides
    Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdTransform3dBaseHandler

} // namespace ufe
} // namespace MayaUsd
