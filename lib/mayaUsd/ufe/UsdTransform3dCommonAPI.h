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

#include <mayaUsd/ufe/UfeVersionCompat.h>
#include <mayaUsd/ufe/UsdTransform3dBase.h>

#include <pxr/usd/usdGeom/xformCommonAPI.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Transform USD objects in 3D using the USD common transform API.
//
// See
// https://graphics.pixar.com/usd/docs/api/class_usd_geom_xform_common_a_p_i.html
// for details.
//
// The USD common transform API has a single pivot.  This pivot and its inverse
// bracket both rotation and scale.  This class uses the rotate pivot methods
// to read and write the single pivot.  The scale pivot command method returns a
// null pointer, the scale pivot modifier is a no-op, and the scale
// pivot accessor returns a zero vector.
//
class MAYAUSD_CORE_PUBLIC UsdTransform3dCommonAPI : public UsdTransform3dBase
{
public:
    typedef std::shared_ptr<UsdTransform3dCommonAPI> Ptr;

    UsdTransform3dCommonAPI(const UsdSceneItem::Ptr& item);
    ~UsdTransform3dCommonAPI() override = default;

    //! Create a UsdTransform3dCommonAPI.
    static UsdTransform3dCommonAPI::Ptr create(const UsdSceneItem::Ptr& item);

    Ufe::Vector3d translation() const override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;

    void translate(double x, double y, double z) override;
    void rotate(double x, double y, double z) override;
    void scale(double x, double y, double z) override;

    Ufe::TranslateUndoableCommand::Ptr translateCmd(double x, double y, double z) override;
    Ufe::RotateUndoableCommand::Ptr    rotateCmd(double x, double y, double z) override;
    Ufe::ScaleUndoableCommand::Ptr     scaleCmd(double x, double y, double z) override;

    Ufe::TranslateUndoableCommand::Ptr rotatePivotCmd(double x, double y, double z) override;
    void                               rotatePivot(double x, double y, double z) override;
    Ufe::Vector3d                      rotatePivot() const override;

    Ufe::SetMatrix4dUndoableCommand::Ptr setMatrixCmd(const Ufe::Matrix4d& m) override;

private:
    PXR_NS::UsdGeomXformCommonAPI _commonAPI;

}; // UsdTransform3dCommonAPI

//! \brief Factory to create a UsdTransform3dCommonAPI interface object.
//
// Note that all calls to specify time use the default time, but this
// could be changed to use the current time, using getTime(path()).

class MAYAUSD_CORE_PUBLIC UsdTransform3dCommonAPIHandler : public Ufe::Transform3dHandler
{
public:
    typedef std::shared_ptr<UsdTransform3dCommonAPIHandler> Ptr;

    UsdTransform3dCommonAPIHandler(const Ufe::Transform3dHandler::Ptr& nextHandler);

    //! Create a UsdTransform3dCommonAPIHandler.
    static Ptr create(const Ufe::Transform3dHandler::Ptr& nextHandler);

    // Ufe::Transform3dHandler overrides
    Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::Transform3d::Ptr editTransform3d(const Ufe::SceneItem::Ptr& item UFE_V2(
        ,
        const Ufe::EditTransform3dHint& hint)) const override;

private:
    Ufe::Transform3dHandler::Ptr _nextHandler;

}; // UsdTransform3dCommonAPIHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
