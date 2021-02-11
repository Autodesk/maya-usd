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

#if UFE_PREVIEW_VERSION_NUM >= 2034
#include <mayaUsd/ufe/UsdUndoVisibleCommand.h>
#endif

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <ufe/attributes.h>
#include <ufe/types.h>

#include <stdexcept>

namespace {
Ufe::Vector3d toVector3d(const GfVec3d& v) { return Ufe::Vector3d(v[0], v[1], v[2]); }
} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdObject3d::UsdObject3d(const UsdSceneItem::Ptr& item)
    : Ufe::Object3d()
    , fItem(item)
    , fPrim(item->prim())
{
}

UsdObject3d::~UsdObject3d() { }

/*static*/
UsdObject3d::Ptr UsdObject3d::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdObject3d>(item);
}

//------------------------------------------------------------------------------
// Ufe::Object3d overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdObject3d::sceneItem() const { return fItem; }

Ufe::BBox3d UsdObject3d::boundingBox() const
{
    // Use USD to compute the bounding box.  This is strictly speaking
    // incorrect, as a USD node may eventually have a Maya child, given the
    // full generality of UFE paths.  However, as of 24-Oct-2019, this does not
    // exist.  To support this use case,
    // UsdGeomBoundable::ComputeExtentFromPlugins() allows a plugin to register
    // an extent computation; this should be explored.
    //
    // UsdGeomImageable::ComputeUntransformedBound() just calls
    // UsdGeomBBoxCache, so do this here as well.
    //
    // Would be nice to know if the object extents are animated or not, so
    // we can bypass time computation and simply use UsdTimeCode::Default()
    // as the time.

    auto path = sceneItem()->path();
    auto purposes = getProxyShapePurposes(path);
    // Add in the default purpose.
    purposes.emplace_back(UsdGeomTokens->default_);

    auto bbox = UsdGeomBBoxCache(getTime(path), purposes).ComputeUntransformedBound(fPrim);
    auto range = bbox.ComputeAlignedRange();
    auto min = range.GetMin();
    auto max = range.GetMax();
    return Ufe::BBox3d(toVector3d(min), toVector3d(max));
}

bool UsdObject3d::visibility() const
{
    TfToken visibilityToken;
    auto    visAttr = UsdGeomImageable(fPrim).GetVisibilityAttr();
    visAttr.Get(&visibilityToken);

    return visibilityToken != UsdGeomTokens->invisible;
}

void UsdObject3d::setVisibility(bool vis)
{
    vis ? UsdGeomImageable(fPrim).MakeVisible() : UsdGeomImageable(fPrim).MakeInvisible();
}

#if UFE_PREVIEW_VERSION_NUM >= 2034
Ufe::UndoableCommand::Ptr UsdObject3d::setVisibleCmd(bool vis)
{
    return UsdUndoVisibleCommand::create(fPrim, vis);
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
