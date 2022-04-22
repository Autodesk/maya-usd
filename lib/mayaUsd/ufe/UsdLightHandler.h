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
#pragma once

#include <mayaUsd/base/api.h>

#include <ufe/lightHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdLightHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdLightHandler : public Ufe::LightHandler
{
public:
    typedef std::shared_ptr<UsdLightHandler> Ptr;

    UsdLightHandler();
    ~UsdLightHandler();

    // Delete the copy/move constructors assignment operators.
    UsdLightHandler(const UsdLightHandler&) = delete;
    UsdLightHandler& operator=(const UsdLightHandler&) = delete;
    UsdLightHandler(UsdLightHandler&&) = delete;
    UsdLightHandler& operator=(UsdLightHandler&&) = delete;

    //! Create a UsdLightHandler.
    static UsdLightHandler::Ptr create();

    // Ufe::LightHandler overrides
    Ufe::Light::Ptr light(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdLightHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
