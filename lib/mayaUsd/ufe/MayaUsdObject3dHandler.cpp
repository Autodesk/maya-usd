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
//

#include "MayaUsdObject3dHandler.h"

#include <mayaUsd/ufe/MayaUsdObject3d.h>
#include <mayaUsd/ufe/Utils.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdObject3dHandler, MayaUsdObject3dHandler);

/*static*/
MayaUsdObject3dHandler::Ptr MayaUsdObject3dHandler::create()
{
    return std::make_shared<MayaUsdObject3dHandler>();
}

//------------------------------------------------------------------------------
// MayaUsdObject3dHandler overrides
//------------------------------------------------------------------------------

Ufe::Object3d::Ptr MayaUsdObject3dHandler::object3d(const Ufe::SceneItem::Ptr& item) const
{
    if (canCreateObject3dForItem(item)) {
        auto usdItem = downcast(item);
        return MayaUsdObject3d::create(usdItem);
    }
    return nullptr;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
