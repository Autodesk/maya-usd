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

#include "UsdSceneItemOpsHandler.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdSceneItemOpsHandler::UsdSceneItemOpsHandler()
	: Ufe::SceneItemOpsHandler()
{
	fUsdSceneItemOps = UsdSceneItemOps::create();
}

UsdSceneItemOpsHandler::~UsdSceneItemOpsHandler()
{
}

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
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	fUsdSceneItemOps->setItem(usdItem);
	return fUsdSceneItemOps;
}

} // namespace ufe
} // namespace MayaUsd
