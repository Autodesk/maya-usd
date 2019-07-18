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
#include "ufe/path.h"

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

#ifdef UFE_GROUP_INTERFACE_AVAILABLE
	Ufe::SceneItem::Ptr createGroup(const Ufe::PathComponent& name) const override;
	Ufe::Group createGroupCmd(const Ufe::PathComponent& name) const override;
#endif

private:
	UsdSceneItem::Ptr fItem;
	UsdPrim fPrim;

}; // UsdHierarchy

} // namespace ufe
} // namespace MayaUsd
