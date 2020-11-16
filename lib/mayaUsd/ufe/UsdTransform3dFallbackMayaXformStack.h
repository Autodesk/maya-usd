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

#include <mayaUsd/ufe/UsdTransform3dBase.h>

#include <mayaUsd/fileio/utils/xformStack.h>

#include <pxr/usd/usdGeom/xformable.h>

#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {
namespace hack {

//! \brief Default transform stack implementation of the Transform3d interface.
//
// This implementation of the Transform3d interface is the fallback in the
// chain of responsibility.  If no previous Transform3d handler has been
// capable of creating a Transform3d interface for the USD transformable prim,
// the Transform3d handler for this class will be invoked.  This class appends
// a Maya transform stack to the existing transform stack, for maximum
// editability.  See \ref UsdTransform3dMayaXformStack documentation for more
// details on the Maya transform stack.
//
// Because of transform op name uniqueness requirements, this class must append
// transform ops with a custom suffix --- see
// https://graphics.pixar.com/usd/docs/api/class_usd_geom_xformable.html#details
// All transform ops have the following namespace components:
// "xformOp:<opType>:<suffix>"
// with suffix being optional and potentially composed of multiple namespace
// components.  This class uses the string "maya-default" in the suffix to
// identify its transform ops, so the namespace components are:
// "xformOp:<opType>:maya-default:<suffix>"
// The "maya-default" namespace component is not optional, but the rest of the
// suffix remains optional.
// 
// Because all Transform3d handlers have already run and failed to match
// (including the UsdTransform3dMayaXformStack), we know there is at least one
// transform op in the existing stack, since an empty stack matches the
// UsdTransform3dMayaXformStack.  Therefore, the stack must be composed of one
// or more non-matching transform ops, followed by zero or more transform ops
// matching our fallback.
//
// Once one or more transform ops from the default transform stack are appended
// to the existing transform stack, the default transform stack Transform3d
// handler will match, and thus all further editing operations will occur on
// the default transform stack.
//
// The Transform3d handler for this class can fail to match: if there is one
// default transform op in the stack, then all subsequent transform ops must be
// part of the default, and they must be in the Maya transform stack order.  If
// there is one or more non-default transform op after a default transform op,
// or if the default transform ops are not in the proper order for a Maya
// transform stack, the Transform3d handler will return a null Transform3d
// interface pointer.
//
class MAYAUSD_CORE_PUBLIC UsdTransform3dMayaXformStack : public UsdTransform3dBase
{
public:
    enum OpNdx {NdxTranslate = 0, NdxRotatePivotTranslate, NdxRotatePivot, 
                NdxRotate, NdxRotateAxis, NdxRotatePivotInverse, 
                NdxScalePivotTranslate, NdxScalePivot, NdxShear, NdxScale, 
                NdxScalePivotInverse, NbOpNdx};

    typedef std::shared_ptr<UsdTransform3dMayaXformStack> Ptr;

    UsdTransform3dMayaXformStack(
        const UsdSceneItem::Ptr& item, const std::vector<UsdGeomXformOp>& ops
    );
    ~UsdTransform3dMayaXformStack() override = default;

    //! Create a UsdTransform3dMayaXformStack for the given item.  The argument
    //! transform ops must match a Maya transform stack.
    static UsdTransform3dMayaXformStack::Ptr create(
        const UsdSceneItem::Ptr& item, const std::vector<UsdGeomXformOp>& ops
    );

    Ufe::Vector3d translation() const override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;

    Ufe::TranslateUndoableCommand::Ptr translateCmd(double x, double y, double z) override;
    Ufe::RotateUndoableCommand::Ptr rotateCmd(double x, double y, double z) override;
    Ufe::ScaleUndoableCommand::Ptr scaleCmd(double x, double y, double z) override;

    Ufe::TranslateUndoableCommand::Ptr rotatePivotCmd(double x, double y, double z) override;
    Ufe::Vector3d rotatePivot() const override;

    Ufe::TranslateUndoableCommand::Ptr scalePivotCmd(double x, double y, double z) override;
    Ufe::Vector3d scalePivot() const override;

    Ufe::TranslateUndoableCommand::Ptr translateRotatePivotCmd(
        double x, double y, double z) override;

    Ufe::Vector3d rotatePivotTranslation() const override;

    Ufe::TranslateUndoableCommand::Ptr translateScalePivotCmd(
        double x, double y, double z) override;

    Ufe::Vector3d scalePivotTranslation() const override;

    Ufe::Matrix4d segmentExclusiveMatrix() const override;

private:

    bool           hasOp(OpNdx ndx) const;
    UsdGeomXformOp getOp(OpNdx ndx) const;

    Ufe::TranslateUndoableCommand::Ptr pivotCmd(
        const TfToken& pvtOpSuffix,
        double x, double y, double z
    );
    template<class V>
    Ufe::Vector3d getVector3d(const TfToken& attrName) const;
    template<class V>
    Ufe::SetVector3dUndoableCommand::Ptr setVector3dCmd(
        const V& v, const TfToken& attrName, const TfToken& opSuffix = TfToken()
    );

    void setXformOpOrder();
    
    // Cache of ops in the ordered ops vector, indexed by position.
    std::unordered_map<OpNdx, UsdGeomXformOp> _orderedOps;

    UsdGeomXformable _xformable;

}; // UsdTransform3dMayaXformStack

//! \brief Factory to create the fallback Transform3d interface object.
//
// Since this is the fallback Transform3d handler, it is the final handler in
// the chain of responsibility.

class MAYAUSD_CORE_PUBLIC UsdTransform3dMayaXformStackHandler
  : public Ufe::Transform3dHandler
{
public:
    typedef std::shared_ptr<UsdTransform3dMayaXformStackHandler> Ptr;

    UsdTransform3dMayaXformStackHandler();

    //! Create a UsdTransform3dMayaXformStackHandler.
    static Ptr create();

    // Ufe::Transform3dHandler overrides
    Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::Transform3d::Ptr editTransform3d(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdTransform3dMayaXformStackHandler

} // namespace hack
} // namespace ufe
} // namespace MayaUsd
