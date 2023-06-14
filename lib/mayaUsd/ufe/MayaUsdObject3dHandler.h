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
#pragma once

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdObject3dHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time 3D object handler.
/*!
        Factory object for Object3d interfaces.
 */
class MAYAUSD_CORE_PUBLIC MayaUsdObject3dHandler : public UsdUfe::UsdObject3dHandler
{
public:
    typedef std::shared_ptr<MayaUsdObject3dHandler> Ptr;

    MayaUsdObject3dHandler();
    ~MayaUsdObject3dHandler() override;

    // Delete the copy/move constructors assignment operators.
    MayaUsdObject3dHandler(const MayaUsdObject3dHandler&) = delete;
    MayaUsdObject3dHandler& operator=(const MayaUsdObject3dHandler&) = delete;
    MayaUsdObject3dHandler(MayaUsdObject3dHandler&&) = delete;
    MayaUsdObject3dHandler& operator=(MayaUsdObject3dHandler&&) = delete;

    //! Create a MayaUsdObject3dHandler.
    static MayaUsdObject3dHandler::Ptr create();

    // MayaUsdObject3dHandler overrides
    Ufe::Object3d::Ptr object3d(const Ufe::SceneItem::Ptr& item) const override;

}; // MayaUsdObject3dHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
