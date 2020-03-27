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

#include "UsdHierarchyHandler.h"
#include "UsdSceneItem.h"
#include "Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdHierarchyHandler::UsdHierarchyHandler()
	: Ufe::HierarchyHandler()
{
	fUsdRootChildHierarchy = UsdRootChildHierarchy::create();
	fUsdHierarchy = UsdHierarchy::create();
}

UsdHierarchyHandler::~UsdHierarchyHandler()
{
}

/*static*/
UsdHierarchyHandler::Ptr UsdHierarchyHandler::create()
{
	return std::make_shared<UsdHierarchyHandler>();
}

//------------------------------------------------------------------------------
// UsdHierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr UsdHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
	if(isRootChild(usdItem->path()))
	{
		fUsdRootChildHierarchy->setItem(usdItem);
		return fUsdRootChildHierarchy;
	}
	else
	{
		fUsdHierarchy->setItem(usdItem);
		return fUsdHierarchy;
	}
}

Ufe::SceneItem::Ptr UsdHierarchyHandler::createItem(const Ufe::Path& path) const
{
	const UsdPrim prim = ufePathToPrim(path);
	return prim.IsValid() ? UsdSceneItem::create(path, prim) : nullptr;
}

} // namespace ufe
} // namespace MayaUsd
