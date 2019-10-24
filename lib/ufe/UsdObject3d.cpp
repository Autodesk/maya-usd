// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdObject3d.h"

#include "ufe/types.h"

#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usd/timeCode.h"

namespace {
Ufe::Vector3d toVector3d(const GfVec3d& v)
{
    return Ufe::Vector3d(v[0], v[1], v[2]);
}

}

MAYAUSD_NS_DEF {
namespace ufe {

UsdObject3d::UsdObject3d(const UsdSceneItem::Ptr& item)
    : Ufe::Object3d(), fItem(item), fPrim(item->prim()) {}

UsdObject3d::~UsdObject3d() {}

/*static*/
UsdObject3d::Ptr UsdObject3d::create(const UsdSceneItem::Ptr& item)
{
	return std::make_shared<UsdObject3d>(item);
}

//------------------------------------------------------------------------------
// Ufe::Object3d overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdObject3d::sceneItem() const
{
	return fItem;
}

Ufe::BBox3d UsdObject3d::boundingBox() const
{
    // Use USD to compute the bounding box.  This is strictly speaking
    // incorrect, as a USD node may eventually have a Maya child, given the
    // full generality of UFE paths.  However, as of 24-Oct-2019, this does not
    // exist.  To support this use case,
    // UsdGeomBoundable::ComputeExtentFromPlugins() allows a plugin to register
    // an extent computation; this should be explored.
    //
    // UsdGeomImageable::ComputeLocalBound() just calls UsdGeomBBoxCache, so do
    // this here as well.  Using the default value through
    // UsdTimeCode::Default() is incorrect: need to get time from Maya.
    TfTokenVector purposes{UsdGeomTokens->default_};
    UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), purposes);
    auto bbox = bboxCache.ComputeLocalBound(fPrim);
    auto range = bbox.GetRange();
    auto min = range.GetMin();
    auto max = range.GetMax();
	return Ufe::BBox3d(toVector3d(min), toVector3d(max));
}

} // namespace ufe
} // namespace MayaUsd
