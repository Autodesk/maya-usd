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
#pragma once

#include "../base/api.h"

#include <ufe/sceneItem.h>

#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time scene item interface
class MAYAUSD_CORE_PUBLIC UsdSceneItem : public Ufe::SceneItem
{
public:
	typedef std::shared_ptr<UsdSceneItem> Ptr;

	UsdSceneItem(const Ufe::Path& path, const UsdPrim& prim);
	~UsdSceneItem() override = default;

	// Delete the copy/move constructors assignment operators.
	UsdSceneItem(const UsdSceneItem&) = delete;
	UsdSceneItem& operator=(const UsdSceneItem&) = delete;
	UsdSceneItem(UsdSceneItem&&) = delete;
	UsdSceneItem& operator=(UsdSceneItem&&) = delete;

	//! Create a UsdSceneItem from a UFE path and a USD prim.
	static UsdSceneItem::Ptr create(const Ufe::Path& path, const UsdPrim& prim);

	const UsdPrim& prim() const { return fPrim; }

	// Ufe::SceneItem overrides
	std::string nodeType() const override;

private:
	UsdPrim fPrim;
}; // UsdSceneItem

} // namespace ufe
} // namespace MayaUsd
