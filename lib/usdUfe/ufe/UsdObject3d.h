// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/base/gf/bbox3d.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>

#include <ufe/object3d.h>
#include <ufe/path.h>

namespace USDUFE_NS_DEF {

//! \brief USD run-time 3D object interface
/*!
    This class implements the Object3d interface for USD prims.
*/
class USDUFE_PUBLIC UsdObject3d : public Ufe::Object3d
{
public:
    using Ptr = std::shared_ptr<UsdObject3d>;

    UsdObject3d(const UsdSceneItem::Ptr& item);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdObject3d);

    //! Create a UsdObject3d.
    static UsdObject3d::Ptr create(const UsdSceneItem::Ptr& item);

    PXR_NS::UsdPrim prim() const { return fPrim; }

    // DCC specific helpers:

    //! Return the non-default purposes of the gateway node along the
    //! argument path.
    //! The default purpose is not returned, and is considered implicit.
    virtual PXR_NS::TfTokenVector getPurposes(const Ufe::Path& path) const;

    //! Adjust the input bounding box extents for the given runtime.
    virtual void adjustBBoxExtents(PXR_NS::GfBBox3d& bbox, const PXR_NS::UsdTimeCode time) const;

    //! Adjust the aligned bounding box for the given runtime.
    virtual Ufe::BBox3d
    adjustAlignedBBox(const Ufe::BBox3d& bbox, const PXR_NS::UsdTimeCode time) const;

    // Ufe::Object3d overrides
    Ufe::SceneItem::Ptr       sceneItem() const override;
    Ufe::BBox3d               boundingBox() const override;
    bool                      visibility() const override;
    void                      setVisibility(bool vis) override;
    Ufe::UndoableCommand::Ptr setVisibleCmd(bool vis) override;

private:
    UsdSceneItem::Ptr fItem;
    PXR_NS::UsdPrim   fPrim;

}; // UsdObject3d

} // namespace USDUFE_NS_DEF
