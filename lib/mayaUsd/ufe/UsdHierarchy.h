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

#include <ufe/hierarchy.h>
#include <ufe/path.h>

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy interface
/*!
    This class implements the hierarchy interface for normal USD prims, using
    standard USD calls to obtain a prim's parent and children.
*/
class MAYAUSD_CORE_PUBLIC UsdHierarchy : public Ufe::Hierarchy
{
public:
	typedef std::shared_ptr<UsdHierarchy> Ptr;

	UsdHierarchy();
	~UsdHierarchy() override;

	// Delete the copy/move constructors assignment operators.
	UsdHierarchy(const UsdHierarchy&) = delete;
	UsdHierarchy& operator=(const UsdHierarchy&) = delete;
	UsdHierarchy(UsdHierarchy&&) = delete;
	UsdHierarchy& operator=(UsdHierarchy&&) = delete;

	//! Create a UsdHierarchy.
	static UsdHierarchy::Ptr create();

	void setItem(UsdSceneItem::Ptr item);
	const Ufe::Path& path() const;

	UsdSceneItem::Ptr usdSceneItem() const;

	// Ufe::Hierarchy overrides
	Ufe::SceneItem::Ptr sceneItem() const override;
	bool hasChildren() const override;
	Ufe::SceneItemList children() const override;
	Ufe::SceneItem::Ptr parent() const override;
	Ufe::AppendedChild appendChild(const Ufe::SceneItem::Ptr& child) override;

#ifdef UFE_V2_FEATURES_AVAILABLE
	Ufe::SceneItem::Ptr createGroup(const Ufe::PathComponent& name) const override;
	Ufe::Group createGroupCmd(const Ufe::PathComponent& name) const override;
#endif

private:
	UsdSceneItem::Ptr fItem;
	UsdPrim fPrim;

}; // UsdHierarchy

} // namespace ufe
} // namespace MayaUsd
