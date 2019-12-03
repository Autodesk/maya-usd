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

#include "UsdSceneItem.h"

#include <ufe/path.h>
#include <ufe/sceneItemOps.h>

#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface for scene item operations.
class MAYAUSD_CORE_PUBLIC UsdSceneItemOps : public Ufe::SceneItemOps
{
public:
	typedef std::shared_ptr<UsdSceneItemOps> Ptr;

	UsdSceneItemOps();
	~UsdSceneItemOps() override;

	// Delete the copy/move constructors assignment operators.
	UsdSceneItemOps(const UsdSceneItemOps&) = delete;
	UsdSceneItemOps& operator=(const UsdSceneItemOps&) = delete;
	UsdSceneItemOps(UsdSceneItemOps&&) = delete;
	UsdSceneItemOps& operator=(UsdSceneItemOps&&) = delete;

	//! Create a UsdSceneItemOps.
	static UsdSceneItemOps::Ptr create();

	void setItem(const UsdSceneItem::Ptr& item);
	const Ufe::Path& path() const;

	// Ufe::SceneItemOps overrides
	Ufe::SceneItem::Ptr sceneItem() const override;
	Ufe::UndoableCommand::Ptr deleteItemCmd() override;
	bool deleteItem() override;
	Ufe::Duplicate duplicateItemCmd() override;
	Ufe::SceneItem::Ptr duplicateItem() override;
	Ufe::Rename renameItemCmd(const Ufe::PathComponent& newName) override;
	Ufe::SceneItem::Ptr renameItem(const Ufe::PathComponent& newName) override;

private:
	UsdSceneItem::Ptr fItem;
	UsdPrim fPrim;

}; // UsdSceneItemOps

} // namespace ufe
} // namespace MayaUsd
