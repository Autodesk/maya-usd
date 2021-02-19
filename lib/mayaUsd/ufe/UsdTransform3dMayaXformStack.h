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

#include <pxr/usd/usdGeom/xformable.h>

#include <map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Transform USD objects in 3D using the Maya transform stack.
//
// The Maya transform stack is described here:
// http://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__cpp_ref_class_m_fn_transform_html
//
// The Maya transform stack represents a local matrix transformation as a fixed
// list of transform ops of a prescribed type and semantics.  This class allows
// for UFE transformation of objects that use this local matrix representation.
//
class MAYAUSD_CORE_PUBLIC UsdTransform3dMayaXformStack : public UsdTransform3dBase
{
public:
    enum OpNdx
    {
        NdxTranslate = 0,
        NdxRotatePivotTranslate,
        NdxRotatePivot,
        NdxRotate,
        NdxRotateAxis,
        NdxRotatePivotInverse,
        NdxScalePivotTranslate,
        NdxScalePivot,
        NdxShear,
        NdxScale,
        NdxScalePivotInverse,
        NbOpNdx
    };

    typedef std::shared_ptr<UsdTransform3dMayaXformStack> Ptr;
    typedef Ufe::Vector3d (*CvtRotXYZFromAttrFn)(const VtValue& value);
    typedef VtValue (*CvtRotXYZToAttrFn)(double x, double y, double z);
    typedef void (*SetXformOpOrderFn)(const UsdGeomXformable&);

    UsdTransform3dMayaXformStack(const UsdSceneItem::Ptr& item);
    ~UsdTransform3dMayaXformStack() override = default;

    //! Create a UsdTransform3dMayaXformStack.
    static UsdTransform3dMayaXformStack::Ptr create(const UsdSceneItem::Ptr& item);

    Ufe::Vector3d translation() const override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;

    Ufe::TranslateUndoableCommand::Ptr translateCmd(double x, double y, double z) override;
    Ufe::RotateUndoableCommand::Ptr    rotateCmd(double x, double y, double z) override;
    Ufe::ScaleUndoableCommand::Ptr     scaleCmd(double x, double y, double z) override;

    Ufe::TranslateUndoableCommand::Ptr rotatePivotCmd(double x, double y, double z) override;
    Ufe::Vector3d                      rotatePivot() const override;

    Ufe::TranslateUndoableCommand::Ptr scalePivotCmd(double x, double y, double z) override;
    Ufe::Vector3d                      scalePivot() const override;

    Ufe::TranslateUndoableCommand::Ptr
    translateRotatePivotCmd(double x, double y, double z) override;

    Ufe::Vector3d rotatePivotTranslation() const override;

    Ufe::TranslateUndoableCommand::Ptr
    translateScalePivotCmd(double x, double y, double z) override;

    Ufe::Vector3d scalePivotTranslation() const override;

    Ufe::SetMatrix4dUndoableCommand::Ptr setMatrixCmd(const Ufe::Matrix4d& m) override;

protected:
    bool                                    hasOp(OpNdx ndx) const;
    UsdGeomXformOp                          getOp(OpNdx ndx) const;
    virtual SetXformOpOrderFn               getXformOpOrderFn() const;
    virtual TfToken                         getOpSuffix(OpNdx ndx) const;
    virtual TfToken                         getTRSOpSuffix() const;
    virtual CvtRotXYZFromAttrFn             getCvtRotXYZFromAttrFn(const TfToken& opName) const;
    virtual CvtRotXYZToAttrFn               getCvtRotXYZToAttrFn(const TfToken& opName) const;
    virtual std::map<OpNdx, UsdGeomXformOp> getOrderedOps() const;

    template <class V> Ufe::Vector3d getVector3d(const TfToken& attrName) const;

    template <class V>
    Ufe::SetVector3dUndoableCommand::Ptr
    setVector3dCmd(const V& v, const TfToken& attrName, const TfToken& opSuffix = TfToken());

    UsdGeomXformable _xformable;

private:
    Ufe::TranslateUndoableCommand::Ptr
    pivotCmd(const TfToken& pvtOpSuffix, double x, double y, double z);
}; // UsdTransform3dMayaXformStack

//! \brief Factory to create a UsdTransform3dMayaXformStack interface object.
//
// Note that all calls to specify time use the default time, but this
// could be changed to use the current time, using getTime(path()).

class MAYAUSD_CORE_PUBLIC UsdTransform3dMayaXformStackHandler : public Ufe::Transform3dHandler
{
public:
    typedef std::shared_ptr<UsdTransform3dMayaXformStackHandler> Ptr;

    UsdTransform3dMayaXformStackHandler(const Ufe::Transform3dHandler::Ptr& nextHandler);

    //! Create a UsdTransform3dMayaXformStackHandler.
    static Ptr create(const Ufe::Transform3dHandler::Ptr& nextHandler);

    // Ufe::Transform3dHandler overrides
    Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::Transform3d::Ptr editTransform3d(const Ufe::SceneItem::Ptr& item UFE_V2(
        ,
        const Ufe::EditTransform3dHint& hint)) const override;

private:
    Ufe::Transform3dHandler::Ptr _nextHandler;

}; // UsdTransform3dMayaXformStackHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
