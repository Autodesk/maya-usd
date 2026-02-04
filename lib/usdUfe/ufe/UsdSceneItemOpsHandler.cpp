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
#include "UsdSceneItemOpsHandler.h"

#include <usdUfe/ufe/Utils.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::SceneItemOpsHandler, UsdSceneItemOpsHandler);

/*static*/
UsdSceneItemOpsHandler::Ptr UsdSceneItemOpsHandler::create()
{
    return std::make_shared<UsdSceneItemOpsHandler>();
}

//------------------------------------------------------------------------------
// Ufe::SceneItemOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::SceneItemOps::Ptr UsdSceneItemOpsHandler::sceneItemOps(const Ufe::SceneItem::Ptr& item) const
{
    auto usdItem = downcast(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif
    return UsdSceneItemOps::create(usdItem);
}

} // namespace USDUFE_NS_DEF
