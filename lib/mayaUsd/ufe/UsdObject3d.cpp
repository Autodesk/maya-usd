// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdObject3d.h"
#include "Utils.h"

#include "ufe/attributes.h"
#include "ufe/types.h"

#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <stdexcept>

namespace {
Ufe::Vector3d toVector3d(const GfVec3d& v)
{
    return Ufe::Vector3d(v[0], v[1], v[2]);
}

#if UFE_PREVIEW_VERSION_NUM >= 2010
Ufe::AttributeEnumString::Ptr getVisibilityAttribute(Ufe::SceneItem::Ptr item)
{
    auto objAttrs = Ufe::Attributes::attributes(item);
    if (objAttrs)
    {
        auto visAttr = std::dynamic_pointer_cast<Ufe::AttributeEnumString>(objAttrs->attribute(UsdGeomTokens->visibility));
        if (visAttr)
           return visAttr;
    }

    // Getting here is considered a serious error. In UsdObject3dHandler::object3d()
    // we only create and return a valid Ufe::Object3d interface for imageable geometry.
    // Those kind of prims must have a visibility attribute.
    std::string err = TfStringPrintf("Could not get visibility attribute for Object3d: %s", item->path().string().c_str());
    throw std::runtime_error(err.c_str());
}
#endif

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
    // this here as well.
    //
    // Would be nice to know if the object extents are animated or not, so
    // we can bypass time computation and simply use UsdTimeCode::Default()
    // as the time.
    TfTokenVector purposes{UsdGeomTokens->default_};
    UsdGeomBBoxCache bboxCache(getTime(sceneItem()->path()), purposes);
    auto bbox = bboxCache.ComputeLocalBound(fPrim);
    auto range = bbox.GetRange();
    auto min = range.GetMin();
    auto max = range.GetMax();
	return Ufe::BBox3d(toVector3d(min), toVector3d(max));
}

#if UFE_PREVIEW_VERSION_NUM >= 2010
bool UsdObject3d::visibility() const
{
    auto visAttr = getVisibilityAttribute(sceneItem());
    if (visAttr) {
        return (visAttr->get() != UsdGeomTokens->invisible);
    }
    return false;
}

void UsdObject3d::setVisibility(bool vis)
{
    auto visAttr = getVisibilityAttribute(sceneItem());
    if (visAttr) {
        visAttr->set(vis ? UsdGeomTokens->inherited : UsdGeomTokens->invisible);
    }
}
#endif

} // namespace ufe
} // namespace MayaUsd
