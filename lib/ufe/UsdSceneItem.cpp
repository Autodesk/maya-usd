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

#include "UsdSceneItem.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdSceneItem::UsdSceneItem(const Ufe::Path& path, const UsdPrim& prim)
	: Ufe::SceneItem(path)
	, fPrim(prim)
{
}

/*static*/
UsdSceneItem::Ptr UsdSceneItem::create(const Ufe::Path& path, const UsdPrim& prim)
{
	return std::make_shared<UsdSceneItem>(path, prim);
}

//------------------------------------------------------------------------------
// Ufe::SceneItem overrides
//------------------------------------------------------------------------------

std::string UsdSceneItem::nodeType() const
{
	return fPrim.GetTypeName();
}

} // namespace ufe
} // namespace MayaUsd
