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
#include "UsdCameraHandler.h"

#include "UsdCamera.h"
#include "pxr/usd/usdGeom/camera.h"

#include <mayaUsd/ufe/UsdSceneItem.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdCameraHandler::UsdCameraHandler()
    : Ufe::CameraHandler()
{
}

UsdCameraHandler::~UsdCameraHandler() { }

/*static*/
UsdCameraHandler::Ptr UsdCameraHandler::create() { return std::make_shared<UsdCameraHandler>(); }

//------------------------------------------------------------------------------
// Ufe::CameraHandler overrides
//------------------------------------------------------------------------------
Ufe::Camera::Ptr UsdCameraHandler::camera(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

    // Test if this item is a camera. If not, then we cannot create a camera
    // interface for it, which is a valid case (such as for a mesh node type).
    PXR_NS::UsdGeomCamera primSchema(usdItem->prim());
    if (!primSchema)
        return nullptr;

    return UsdCamera::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
