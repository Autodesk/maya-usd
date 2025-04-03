//
// Copyright 2023 Autodesk
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
#include "MayaUsdObject3d.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/usdGeom/camera.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdObject3d, MayaUsdObject3d);

MayaUsdObject3d::MayaUsdObject3d(const UsdUfe::UsdSceneItem::Ptr& item)
    : UsdUfe::UsdObject3d(item)
{
}

/*static*/
MayaUsdObject3d::Ptr MayaUsdObject3d::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdObject3d>(item);
}

PXR_NS::TfTokenVector MayaUsdObject3d::getPurposes(const Ufe::Path& path) const
{
    return getProxyShapePurposes(path);
}

void MayaUsdObject3d::adjustBBoxExtents(PXR_NS::GfBBox3d& bbox, const PXR_NS::UsdTimeCode time)
    const
{
    UsdMayaUtil::AddMayaExtents(bbox, prim(), time);
}

Ufe::BBox3d
MayaUsdObject3d::adjustAlignedBBox(const Ufe::BBox3d& bbox, const PXR_NS::UsdTimeCode time) const
{
    Ufe::BBox3d pulledBBox = getPulledPrimsBoundingBox(sceneItem()->path());
    return UsdUfe::combineUfeBBox(bbox, pulledBBox);
}

// This visibility method is for older maya versions that do not have computedVisibility() for
// cameras
#ifndef UFE_CAMERA_HAS_COMPUTEDVISIBILITY
bool MayaUsdObject3d::visibility() const
{
    PXR_NS::TfToken visibilityToken;
    auto            visAttr = PXR_NS::UsdGeomImageable(prim()).GetVisibilityAttr();
    visAttr.Get(&visibilityToken);

    // Check if the prim is a camera
    // if its a camera, then we need to check if the camera's parents are visible
    if (prim().IsA<PXR_NS::UsdGeomCamera>()
        && (visibilityToken == PXR_NS::UsdGeomTokens->inherited)) {
        // get the parent of the camera
        Ufe::Path parentPath = sceneItem()->path().pop();
        while (!parentPath.empty()) {
            // check if the parent is visible
            auto parentItem = Ufe::Hierarchy::createItem(parentPath);
            auto parentObject3d = Ufe::Object3d::object3d(parentItem);
            if (parentObject3d && !parentObject3d->visibility()) {
                return false;
            }
            parentPath = parentPath.pop();
        }
    }

    return visibilityToken != PXR_NS::UsdGeomTokens->invisible;
}
#endif // UFE_CAMERA_HAS_COMPUTEDVISIBILITY

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
