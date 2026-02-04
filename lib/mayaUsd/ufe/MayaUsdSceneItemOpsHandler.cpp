//
// Copyright 2019 Autodesk
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
#include "MayaUsdSceneItemOpsHandler.h"

#include <mayaUsd/ufe/MayaUsdSceneItemOps.h>
#include <mayaUsd/ufe/Utils.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdSceneItemOpsHandler, MayaUsdSceneItemOpsHandler);

/*static*/
MayaUsdSceneItemOpsHandler::Ptr MayaUsdSceneItemOpsHandler::create()
{
    return std::make_shared<MayaUsdSceneItemOpsHandler>();
}

//------------------------------------------------------------------------------
// Ufe::SceneItemOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::SceneItemOps::Ptr
MayaUsdSceneItemOpsHandler::sceneItemOps(const Ufe::SceneItem::Ptr& item) const
{
    auto usdItem = downcast(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif
    return MayaUsdSceneItemOps::create(usdItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
