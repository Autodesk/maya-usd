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

#include "UsdContextOpsHandler.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdContextOpsHandler::UsdContextOpsHandler()
	: Ufe::ContextOpsHandler()
{
	fUsdContextOps = UsdContextOps::create();
}

UsdContextOpsHandler::~UsdContextOpsHandler()
{
}

/*static*/
UsdContextOpsHandler::Ptr UsdContextOpsHandler::create()
{
	return std::make_shared<UsdContextOpsHandler>();
}

//------------------------------------------------------------------------------
// Ufe::ContextOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::ContextOps::Ptr UsdContextOpsHandler::contextOps(const Ufe::SceneItem::Ptr& item) const
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	fUsdContextOps->setItem(usdItem);
	return fUsdContextOps;
}

} // namespace ufe
} // namespace MayaUsd
