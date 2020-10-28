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
#include "UsdTransform3dHandler.h"

#include <mayaUsd/ufe/UsdSceneItem.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dHandler::UsdTransform3dHandler() : Ufe::Transform3dHandler()
{}

UsdTransform3dHandler::~UsdTransform3dHandler()
{
}

/*static*/
UsdTransform3dHandler::Ptr UsdTransform3dHandler::create()
{
	return std::make_shared<UsdTransform3dHandler>();
}

//------------------------------------------------------------------------------
// Ufe::Transform3dHandler overrides
//------------------------------------------------------------------------------

Ufe::Transform3d::Ptr UsdTransform3dHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	return UsdTransform3d::create(usdItem);
}

} // namespace ufe
} // namespace MayaUsd
