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
#ifndef MAYAUSD_PROXYSHAPECAMERAHANDLER_H
#define MAYAUSD_PROXYSHAPECAMERAHANDLER_H

#include <mayaUsd/base/api.h>

#include <ufe/cameraHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

#if UFE_MAJOR_VERSION == 3 && UFE_CAMERAHANDLER_HAS_FINDALL
#define CAMERAHANDLERBASE Ufe::CameraHandler_v3_4
#else
#define CAMERAHANDLERBASE Ufe::CameraHandler
#endif // UFE_CAMERAHANDLER_HAS_FINDALL

//! \brief Interface to create a ProxyShapeCameraHandler interface object.
class MAYAUSD_CORE_PUBLIC ProxyShapeCameraHandler : public CAMERAHANDLERBASE
{
public:
    typedef std::shared_ptr<ProxyShapeCameraHandler> Ptr;

    ProxyShapeCameraHandler(const CAMERAHANDLERBASE::Ptr&);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(ProxyShapeCameraHandler);

    //! Create a ProxyShapeCameraHandler from a UFE camera handler.
    static ProxyShapeCameraHandler::Ptr create(const Ufe::CameraHandler::Ptr&);

    // Ufe::CameraHandler overrides
    Ufe::Camera::Ptr camera(const Ufe::SceneItem::Ptr& item) const override;

    Ufe::Selection find_(const Ufe::Path& path) const override;

private:
    CAMERAHANDLERBASE::Ptr _mayaCameraHandler;

}; // ProxyShapeCameraHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_PROXYSHAPECAMERAHANDLER_H
