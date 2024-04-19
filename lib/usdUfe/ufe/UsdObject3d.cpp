//
// Copyright 2019 Autodesk
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
#include "UsdObject3d.h"

#include <usdUfe/ufe/UsdUndoVisibleCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <ufe/attributes.h>
#include <ufe/types.h>

#include <stdexcept>

namespace {
Ufe::Vector3d toVector3d(const PXR_NS::GfVec3d& v) { return Ufe::Vector3d(v[0], v[1], v[2]); }
} // namespace

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::Object3d, UsdObject3d);

UsdObject3d::UsdObject3d(const UsdSceneItem::Ptr& item)
    : Ufe::Object3d()
    , fItem(item)
    , fPrim(item->prim())
{
}

/*static*/
UsdObject3d::Ptr UsdObject3d::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdObject3d>(item);
}

//------------------------------------------------------------------------------
// DCC specific helpers
//------------------------------------------------------------------------------
PXR_NS::TfTokenVector UsdObject3d::getPurposes(const Ufe::Path&) const { return {}; }

void UsdObject3d::adjustBBoxExtents(PXR_NS::GfBBox3d&, const PXR_NS::UsdTimeCode) const
{
    // Do nothing in base class.
}

Ufe::BBox3d
UsdObject3d::adjustAlignedBBox(const Ufe::BBox3d& bbox, const PXR_NS::UsdTimeCode time) const
{
    // Do nothing in the base class
    return bbox;
}

//------------------------------------------------------------------------------
// Ufe::Object3d overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdObject3d::sceneItem() const { return fItem; }

Ufe::BBox3d UsdObject3d::boundingBox() const
{
    // Use USD to compute the bounding box in local space.
    // UsdGeomBoundable::ComputeExtentFromPlugins() allows a plugin to register
    // an extent computation; this could be explored if needed in the future.
    //
    // Would be nice to know if the object extents are animated or not, so
    // we can bypass time computation and simply use UsdTimeCode::Default()
    // as the time.

    // Get the DCC specific purposes and then add in the default purpose.
    const auto& path = sceneItem()->path();
    auto        purposes = getPurposes(path);
    purposes.emplace_back(PXR_NS::UsdGeomTokens->default_);

    // UsdGeomImageable::ComputeUntransformedBound() just calls
    // UsdGeomBBoxCache, so do this here as well.
    auto time = getTime(path);
    auto bbox = PXR_NS::UsdGeomBBoxCache(time, purposes).ComputeUntransformedBound(fPrim);

    // Adjust extents for this runtime.
    adjustBBoxExtents(bbox, time);

    auto        range = bbox.ComputeAlignedRange();
    Ufe::BBox3d ufeBBox(toVector3d(range.GetMin()), toVector3d(range.GetMax()));

    // Allow a derived class (for a DCC) to adjust the bounding box.
    return adjustAlignedBBox(ufeBBox, time);
}

bool UsdObject3d::visibility() const
{
    PXR_NS::TfToken visibilityToken;
    auto            visAttr = PXR_NS::UsdGeomImageable(fPrim).GetVisibilityAttr();
    visAttr.Get(&visibilityToken);

    return visibilityToken != PXR_NS::UsdGeomTokens->invisible;
}

void UsdObject3d::setVisibility(bool vis)
{
    AttributeEditRouterContext ctx(fPrim, PXR_NS::UsdGeomTokens->visibility);

    vis ? PXR_NS::UsdGeomImageable(fPrim).MakeVisible()
        : PXR_NS::UsdGeomImageable(fPrim).MakeInvisible();
}

Ufe::UndoableCommand::Ptr UsdObject3d::setVisibleCmd(bool vis)
{
    return UsdUndoVisibleCommand::create(fPrim, vis);
}

} // namespace USDUFE_NS_DEF
