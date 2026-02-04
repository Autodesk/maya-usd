//
// Copyright 2025 Autodesk
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
#include "UsdLight2Handler.h"

#include "UsdLight2.h"

#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usdLux/rectLight.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::Light2Handler, UsdLight2Handler);

UsdLight2Handler::Ptr UsdLight2Handler::create() { return std::make_shared<UsdLight2Handler>(); }

Ufe::Light2::Ptr UsdLight2Handler::light(const Ufe::SceneItem::Ptr& item) const
{
    auto usdItem = downcast(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

    // Test if this item is an area light.
    // For the time being, we only handle Rect Light here.
    PXR_NS::UsdLuxRectLight lightSchema(usdItem->prim());
    if (!lightSchema)
        return nullptr;

    return UsdLight2::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
