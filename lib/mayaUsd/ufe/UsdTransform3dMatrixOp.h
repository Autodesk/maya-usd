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

#include <pxr/usd/usdGeom/xformOp.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to transform objects in 3D.
//
// The UsdTransform3dMatrixOp class implements the Transform3d interface for a
// single UsdGeomXformOp matrix transform op in a UsdGeomXformable prim.  It
// allows reading and editing the local transformation of that single transform
// op.  The parent transformation of the transform op is the concatenation of
// the scene item's parent transformation and the combined transformation of
// all transform ops preceding this one.
//
// This is a departure from the standard Transform3d interface, which allows
// reading and editing the local transformation of the prim as a whole.  Having
// the interface target a single transform op allows for fine-grained control
// and editing of individual transform ops.  To read the local transformation
// of the prim as a whole, use UsdTransform3dBase.
//
// Note that all calls to specify time use the default time, but this
// could be changed to use the current time, using getTime(path()).

class MAYAUSD_CORE_PUBLIC UsdTransform3dMatrixOp : public UsdTransform3dBase
{
public:
    typedef std::shared_ptr<UsdTransform3dMatrixOp> Ptr;

    UsdTransform3dMatrixOp(const UsdSceneItem::Ptr& item, const PXR_NS::UsdGeomXformOp& op);
    ~UsdTransform3dMatrixOp() override = default;

    //! Create a UsdTransform3dMatrixOp.
    static UsdTransform3dMatrixOp::Ptr
    create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdGeomXformOp& op);

    Ufe::Vector3d translation() const override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;

    Ufe::TranslateUndoableCommand::Ptr translateCmd(double x, double y, double z) override;
    Ufe::RotateUndoableCommand::Ptr    rotateCmd(double x, double y, double z) override;
    Ufe::ScaleUndoableCommand::Ptr     scaleCmd(double x, double y, double z) override;

    Ufe::SetMatrix4dUndoableCommand::Ptr setMatrixCmd(const Ufe::Matrix4d& m) override;
    void                                 setMatrix(const Ufe::Matrix4d& m) override;
    Ufe::Matrix4d                        matrix() const override;

    Ufe::Matrix4d segmentInclusiveMatrix() const override;
    Ufe::Matrix4d segmentExclusiveMatrix() const override;

private:
    const PXR_NS::UsdGeomXformOp _op;

}; // UsdTransform3dMatrixOp

//! \brief Factory to create a UsdTransform3dMatrixOp interface object.
//
//
class MAYAUSD_CORE_PUBLIC UsdTransform3dMatrixOpHandler : public Ufe::Transform3dHandler
{
public:
    typedef std::shared_ptr<UsdTransform3dMatrixOpHandler> Ptr;

    UsdTransform3dMatrixOpHandler(const Ufe::Transform3dHandler::Ptr& nextHandler);

    //! Create a UsdTransform3dMatrixOpHandler.
    static Ptr create(const Ufe::Transform3dHandler::Ptr& nextHandler);

    // Ufe::Transform3dHandler overrides
    Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::Transform3d::Ptr editTransform3d(const Ufe::SceneItem::Ptr& item UFE_V2(
        ,
        const Ufe::EditTransform3dHint& hint)) const override;

private:
    Ufe::Transform3dHandler::Ptr _nextHandler;

}; // UsdTransform3dMatrixOpHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
