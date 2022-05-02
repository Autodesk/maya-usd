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
#include "UsdCameraHandler.h"
#include "Global.h"

#include "pxr/usd/usdGeom/camera.h"

#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

ProxyShapeCameraHandler::ProxyShapeCameraHandler(const Ufe::CameraHandler::Ptr& mayaCameraHandler)
    : Ufe::CameraHandler()
    , fMayaCameraHandler(mayaCameraHandler)
{
}

ProxyShapeCameraHandler::~ProxyShapeCameraHandler() { }

/*static*/
ProxyShapeCameraHandler::Ptr ProxyShapeCameraHandler::create(const Ufe::CameraHandler::Ptr& mayaCameraHandler) { return std::make_shared<ProxyShapeCameraHandler>(mayaCameraHandler); }

//------------------------------------------------------------------------------
// Ufe::CameraHandler overrides
//------------------------------------------------------------------------------
Ufe::Camera::Ptr ProxyShapeCameraHandler::camera(const Ufe::SceneItem::Ptr& item) const
{
    return fMayaCameraHandler ? fMayaCameraHandler->camera(item) : nullptr;
}

Ufe::Selection ProxyShapeCameraHandler::find(const Ufe::Path& path) const
{
    Ufe::SceneItem::Ptr item = Ufe::Hierarchy::createItem(path);
    if (isAGatewayType(item->nodeType()))
    {
        // path is a path to a proxyShape node.
        // Get the UsdStage for this proxy shape node and search it for cameras
        PXR_NS::UsdStageWeakPtr stage = getStage(path);
        TF_VERIFY(stage);
        return UsdCameraHandler::find(path, path, stage->GetPseudoRoot());
    }
    return fMayaCameraHandler ? fMayaCameraHandler->find(path) : Ufe::Selection();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
