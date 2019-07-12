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

#include "ufe/path.h"
#include "ufe/sceneItemOps.h"

#include "pxr/usd/usd/prim.h"

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
