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
    typedef Ufe::Vector3d (*CvtRotXYZFromAttrFn)(const PXR_NS::VtValue& value);
    typedef PXR_NS::VtValue (*CvtRotXYZToAttrFn)(double x, double y, double z);
    typedef bool (*SetXformOpOrderFn)(const PXR_NS::UsdGeomXformable&);

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
    bool                        hasOp(OpNdx ndx) const;
    PXR_NS::UsdGeomXformOp      getOp(OpNdx ndx) const;
    virtual SetXformOpOrderFn   getXformOpOrderFn() const;
    virtual PXR_NS::TfToken     getOpSuffix(OpNdx ndx) const;
    virtual PXR_NS::TfToken     getTRSOpSuffix() const;
    virtual CvtRotXYZFromAttrFn getCvtRotXYZFromAttrFn(const PXR_NS::TfToken& opName) const;
    virtual CvtRotXYZToAttrFn   getCvtRotXYZToAttrFn(const PXR_NS::TfToken& opName) const;
    virtual std::map<OpNdx, PXR_NS::UsdGeomXformOp> getOrderedOps() const;

    template <class V> Ufe::Vector3d getVector3d(const PXR_NS::TfToken& attrName) const;

    template <class V>
    Ufe::SetVector3dUndoableCommand::Ptr setVector3dCmd(
        const V&               v,
        const PXR_NS::TfToken& attrName,
        const PXR_NS::TfToken& opSuffix = PXR_NS::TfToken());

    PXR_NS::UsdGeomXformable _xformable;

private:
    Ufe::TranslateUndoableCommand::Ptr
    pivotCmd(const PXR_NS::TfToken& pvtOpSuffix, double x, double y, double z);

    bool isAttributeEditAllowed(const PXR_NS::TfToken attrName, std::string& errMsg) const;
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
#ifdef UFE_V4_FEATURES_AVAILABLE
#if (UFE_PREVIEW_VERSION_NUM >= 4025)
    Ufe::Transform3dRead::Ptr transform3dRead(const Ufe::SceneItem::Ptr& item) const override;
#endif
#endif
    Ufe::Transform3d::Ptr editTransform3d(const Ufe::SceneItem::Ptr& item UFE_V2(
        ,
        const Ufe::EditTransform3dHint& hint)) const override;

private:
    Ufe::Transform3dHandler::Ptr _nextHandler;

}; // UsdTransform3dMayaXformStackHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
