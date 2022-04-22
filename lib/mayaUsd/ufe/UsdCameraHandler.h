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
#pragma once

#include <mayaUsd/base/api.h>

#include <ufe/cameraHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdCameraHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdCameraHandler : public Ufe::CameraHandler
{
public:
    typedef std::shared_ptr<UsdCameraHandler> Ptr;

    UsdCameraHandler();
    ~UsdCameraHandler();

    // Delete the copy/move constructors assignment operators.
    UsdCameraHandler(const UsdCameraHandler&) = delete;
    UsdCameraHandler& operator=(const UsdCameraHandler&) = delete;
    UsdCameraHandler(UsdCameraHandler&&) = delete;
    UsdCameraHandler& operator=(UsdCameraHandler&&) = delete;

    //! Create a UsdCameraHandler.
    static UsdCameraHandler::Ptr create();

    // Ufe::CameraHandler overrides
    Ufe::Camera::Ptr camera(const Ufe::SceneItem::Ptr& item) const override;

#if defined(UFE_V4_FEATURES_AVAILABLE) && (UFE_PREVIEW_VERSION_NUM >= 4008)
    Ufe::Selection findCamerasInSceneSegment(const Ufe::Path& path) const override;
#endif

}; // UsdCameraHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
