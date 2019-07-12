// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdSceneItemOps.h"
#include "UsdUndoDeleteCommand.h"
#include "UsdUndoDuplicateCommand.h"
#include "UsdUndoRenameCommand.h"
#include "Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdSceneItemOps::UsdSceneItemOps()
	: Ufe::SceneItemOps()
{
}

UsdSceneItemOps::~UsdSceneItemOps()
{
}

/*static*/
UsdSceneItemOps::Ptr UsdSceneItemOps::create()
{
	return std::make_shared<UsdSceneItemOps>();
}

void UsdSceneItemOps::setItem(const UsdSceneItem::Ptr& item)
{
	fPrim = item->prim();
	fItem = item;
}

const Ufe::Path& UsdSceneItemOps::path() const
{
	return fItem->path();
}

//------------------------------------------------------------------------------
// Ufe::SceneItemOps overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdSceneItemOps::sceneItem() const
{
	return fItem;
}

Ufe::UndoableCommand::Ptr UsdSceneItemOps::deleteItemCmd()
{
	auto deleteCmd = UsdUndoDeleteCommand::create(fPrim);
	deleteCmd->execute();
	return deleteCmd;
}

bool UsdSceneItemOps::deleteItem()
{
	return fPrim.SetActive(false);
}

Ufe::Duplicate UsdSceneItemOps::duplicateItemCmd()
{
	auto duplicateCmd = UsdUndoDuplicateCommand::create(fPrim, fItem->path());
	duplicateCmd->execute();
	auto item = createSiblingSceneItem(path(), duplicateCmd->usdDstPath().GetElementString());
	return Ufe::Duplicate(item, duplicateCmd);
}

Ufe::SceneItem::Ptr UsdSceneItemOps::duplicateItem()
{
	SdfPath usdDstPath;
	SdfLayerHandle layer;
	UsdUndoDuplicateCommand::primInfo(fPrim, usdDstPath, layer);
	bool status = UsdUndoDuplicateCommand::duplicate(layer, fPrim.GetPath(), usdDstPath);

	// The duplicate is a sibling of the source.
	if (status)
		return createSiblingSceneItem(path(), usdDstPath.GetElementString());

	return nullptr;
}

Ufe::SceneItem::Ptr UsdSceneItemOps::renameItem(const Ufe::PathComponent& newName)
{
	auto renameCmd = UsdUndoRenameCommand::create(fItem, newName);
	renameCmd->execute();
	return renameCmd->renamedItem();
}

Ufe::Rename UsdSceneItemOps::renameItemCmd(const Ufe::PathComponent& newName)
{
	auto renameCmd = UsdUndoRenameCommand::create(fItem, newName);
	renameCmd->execute();
	return Ufe::Rename(renameCmd->renamedItem(), renameCmd);
}

} // namespace ufe
} // namespace MayaUsd
