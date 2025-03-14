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

#include "pxr/usd/usdGeom/camera.h"

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdCamera.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

#if UFE_MAJOR_VERSION == 3 && UFE_CAMERAHANDLER_HAS_FINDALL
USDUFE_VERIFY_CLASS_SETUP(Ufe::CameraHandler_v3_4, UsdCameraHandler);
#else
USDUFE_VERIFY_CLASS_SETUP(Ufe::CameraHandler, UsdCameraHandler);
#endif // UFE_CAMERAHANDLER_HAS_FINDALL

/*static*/
UsdCameraHandler::Ptr UsdCameraHandler::create() { return std::make_shared<UsdCameraHandler>(); }

//------------------------------------------------------------------------------
// Ufe::CameraHandler overrides
//------------------------------------------------------------------------------
Ufe::Camera::Ptr UsdCameraHandler::camera(const Ufe::SceneItem::Ptr& item) const
{
    auto usdItem = downcast(item);
    TF_VERIFY(usdItem);

    // Test if this item is a camera. If not, then we cannot create a camera
    // interface for it, which is a valid case (such as for a mesh node type).
    PXR_NS::UsdGeomCamera primSchema(usdItem->prim());
    if (!primSchema)
        return nullptr;

    return UsdCamera::create(usdItem);
}

#if defined(UFE_V4_FEATURES_AVAILABLE) || (UFE_MAJOR_VERSION == 3 && UFE_CAMERAHANDLER_HAS_FINDALL)
Ufe::Selection UsdCameraHandler::find_(const Ufe::Path& path) const
{
    TF_VERIFY(path.runTimeId() == getUsdRunTimeId());
    Ufe::Path stagePath(path.getSegments()[0]); // assumes there is only ever two segments
    return find(stagePath, path, ufePathToPrim(path));
}

/*static*/
Ufe::Selection UsdCameraHandler::find(
    const Ufe::Path&       stagePath,
    const Ufe::Path&       searchPath,
    const PXR_NS::UsdPrim& prim)
{
    Ufe::Selection result;
    if (prim.IsA<PXR_NS::UsdGeomCamera>()) {
        result.append(Ufe::Hierarchy::createItem(searchPath));
    }
    PXR_NS::UsdPrimSubtreeRange range = prim.GetDescendants();
    for (auto iter = range.begin(); iter != range.end(); ++iter) {
        if (iter->IsA<PXR_NS::UsdGeomCamera>()) {
            Ufe::Path cameraPath = stagePath + usdPathToUfePathSegment(iter->GetPath());
            result.append(Ufe::Hierarchy::createItem(cameraPath));
        }
    }
    return result;
}
#endif // UFE_MAJOR_VERSION == 3 && UFE_CAMERAHANDLER_HAS_FINDALL

} // namespace USDUFE_NS_DEF
