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
#include <ufe/hierarchyHandler.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD gateway node hierarchy interface.
/*!
    This class defines a hierarchy interface for a single kind of Maya node,
    the USD gateway node.  This node is special in that its parent is a Maya
    node, but its children are children of the USD root prim.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeHierarchy : public Ufe::Hierarchy
{
public:
	typedef std::shared_ptr<ProxyShapeHierarchy> Ptr;

	ProxyShapeHierarchy(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);
	~ProxyShapeHierarchy() override;

	// Delete the copy/move constructors assignment operators.
	ProxyShapeHierarchy(const ProxyShapeHierarchy&) = delete;
	ProxyShapeHierarchy& operator=(const ProxyShapeHierarchy&) = delete;
	ProxyShapeHierarchy(ProxyShapeHierarchy&&) = delete;
	ProxyShapeHierarchy& operator=(ProxyShapeHierarchy&&) = delete;

	//! Create a ProxyShapeHierarchy from a UFE hierarchy handler.
	static ProxyShapeHierarchy::Ptr create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);
	static ProxyShapeHierarchy::Ptr create(
        const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler,
        const Ufe::SceneItem::Ptr&        item
    );

	void setItem(const Ufe::SceneItem::Ptr& item);

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
	const UsdPrim& getUsdRootPrim() const;

private:
	Ufe::SceneItem::Ptr fItem;
	Hierarchy::Ptr fMayaHierarchy;
	Ufe::HierarchyHandler::Ptr fMayaHierarchyHandler;

	// The root prim is initialized on first use and therefore mutable.
	mutable UsdPrim fUsdRootPrim;
}; // ProxyShapeHierarchy

} // namespace ufe
} // namespace MayaUsd
