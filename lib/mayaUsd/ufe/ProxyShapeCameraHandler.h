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

//! \brief Interface to create a ProxyShapeCameraHandler interface object.
class MAYAUSD_CORE_PUBLIC ProxyShapeCameraHandler : public Ufe::CameraHandler
{
public:
    typedef std::shared_ptr<ProxyShapeCameraHandler> Ptr;

    ProxyShapeCameraHandler(const Ufe::CameraHandler::Ptr&);
    ~ProxyShapeCameraHandler();

    // Delete the copy/move constructors assignment operators.
    ProxyShapeCameraHandler(const ProxyShapeCameraHandler&) = delete;
    ProxyShapeCameraHandler& operator=(const ProxyShapeCameraHandler&) = delete;
    ProxyShapeCameraHandler(ProxyShapeCameraHandler&&) = delete;
    ProxyShapeCameraHandler& operator=(ProxyShapeCameraHandler&&) = delete;

    //! Create a ProxyShapeCameraHandler from a UFE camera handler.
    static ProxyShapeCameraHandler::Ptr create(const Ufe::CameraHandler::Ptr&);

    // Ufe::CameraHandler overrides
    Ufe::Camera::Ptr camera(const Ufe::SceneItem::Ptr& item) const override;

    Ufe::Selection find(const Ufe::Path& path) const override;

private:
    Ufe::CameraHandler::Ptr fMayaCameraHandler;

}; // ProxyShapeCameraHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
