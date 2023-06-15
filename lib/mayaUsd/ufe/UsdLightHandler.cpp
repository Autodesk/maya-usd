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
#include "UsdLightHandler.h"

#include "UsdLight.h"

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usdLux/lightAPI.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdLightHandler::UsdLightHandler()
    : Ufe::LightHandler()
{
}

UsdLightHandler::~UsdLightHandler() { }

UsdLightHandler::Ptr UsdLightHandler::create() { return std::make_shared<UsdLightHandler>(); }

Ufe::Light::Ptr UsdLightHandler::light(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

    // Test if this item is a light. If not, then we cannot create a light
    // interface for it, which is a valid case (such as for a mesh node type).
    PXR_NS::UsdLuxLightAPI lightSchema(usdItem->prim());
    if (!lightSchema)
        return nullptr;

    return UsdLight::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
