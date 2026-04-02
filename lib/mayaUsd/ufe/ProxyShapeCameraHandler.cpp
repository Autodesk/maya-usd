//
// Copyright 2022 Autodesk
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
#include "ProxyShapeCameraHandler.h"

#include "pxr/usd/usdGeom/camera.h"

#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdCameraHandler.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

#if UFE_MAJOR_VERSION == 3 && UFE_CAMERAHANDLER_HAS_FINDALL
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::CameraHandler_v3_4, ProxyShapeCameraHandler);
#else
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::CameraHandler, ProxyShapeCameraHandler);
#endif // UFE_CAMERAHANDLER_HAS_FINDALL

ProxyShapeCameraHandler::ProxyShapeCameraHandler(const CAMERAHANDLERBASE::Ptr& mayaCameraHandler)
    : CAMERAHANDLERBASE()
    , _mayaCameraHandler(mayaCameraHandler)
{
}

/*static*/
ProxyShapeCameraHandler::Ptr
ProxyShapeCameraHandler::create(const Ufe::CameraHandler::Ptr& mayaCameraHandler)
{
    auto mayaCameraHandlerBase = std::static_pointer_cast<CAMERAHANDLERBASE>(mayaCameraHandler);
    return std::make_shared<ProxyShapeCameraHandler>(mayaCameraHandlerBase);
}

//------------------------------------------------------------------------------
// Ufe::CameraHandler overrides
//------------------------------------------------------------------------------
Ufe::Camera::Ptr ProxyShapeCameraHandler::camera(const Ufe::SceneItem::Ptr& item) const
{
    return _mayaCameraHandler ? _mayaCameraHandler->camera(item) : nullptr;
}

Ufe::Selection ProxyShapeCameraHandler::find_(const Ufe::Path& path) const
{
    Ufe::SceneItem::Ptr item = Ufe::Hierarchy::createItem(path);
    if (item && isAGatewayType(UsdUfe::getSceneItemNodeType(item))) {
        // path is a path to a proxyShape node.
        // Get the UsdStage for this proxy shape node and search it for cameras
        PXR_NS::UsdStageWeakPtr stage = getStage(path);
        if (stage != nullptr) {
            return UsdUfe::UsdCameraHandler::find(path, path, stage->GetPseudoRoot());
        }
        return Ufe::Selection();
    }
    return _mayaCameraHandler ? _mayaCameraHandler->find(path) : Ufe::Selection();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
