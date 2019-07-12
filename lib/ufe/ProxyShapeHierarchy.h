// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdSceneItem.h"

#include "ufe/hierarchy.h"
#include "ufe/hierarchyHandler.h"

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

	ProxyShapeHierarchy(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler);
	~ProxyShapeHierarchy() override;

	// Delete the copy/move constructors assignment operators.
	ProxyShapeHierarchy(const ProxyShapeHierarchy&) = delete;
	ProxyShapeHierarchy& operator=(const ProxyShapeHierarchy&) = delete;
	ProxyShapeHierarchy(ProxyShapeHierarchy&&) = delete;
	ProxyShapeHierarchy& operator=(ProxyShapeHierarchy&&) = delete;

	//! Create a ProxyShapeHierarchy from a UFE hierarchy handler.
	static ProxyShapeHierarchy::Ptr create(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler);

	void setItem(const Ufe::SceneItem::Ptr& item);

	// Ufe::Hierarchy overrides
	Ufe::SceneItem::Ptr sceneItem() const override;
	bool hasChildren() const override;
	Ufe::SceneItemList children() const override;
	Ufe::SceneItem::Ptr parent() const override;
	Ufe::AppendedChild appendChild(const Ufe::SceneItem::Ptr& child) override;
	Ufe::SceneItem::Ptr createGroup(const Ufe::PathComponent& name) const override;
	Ufe::Group createGroupCmd(const Ufe::PathComponent& name) const override;

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
