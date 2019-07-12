// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdUndoCreateGroupCommand.h"

#include "ufe/scene.h"
#include "ufe/hierarchy.h"
#include "ufe/sceneNotification.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoCreateGroupCommand::UsdUndoCreateGroupCommand(const UsdSceneItem::Ptr& parentItem, const Ufe::PathComponent& name)
	: Ufe::UndoableCommand()
	, fParentItem(parentItem)
	, fName(name)
{
}

UsdUndoCreateGroupCommand::~UsdUndoCreateGroupCommand()
{
}

/*static*/
UsdUndoCreateGroupCommand::Ptr UsdUndoCreateGroupCommand::create(const UsdSceneItem::Ptr& parentItem, const Ufe::PathComponent& name)
{
	return std::make_shared<UsdUndoCreateGroupCommand>(parentItem, name);
}

Ufe::SceneItem::Ptr UsdUndoCreateGroupCommand::group() const
{
	return fGroup;
}

//------------------------------------------------------------------------------
// UsdUndoCreateGroupCommand overrides
//------------------------------------------------------------------------------

void UsdUndoCreateGroupCommand::undo()
{
	if (!fGroup) return;

	// See UsdUndoDuplicateCommand.undo() comments.
	auto notification = Ufe::ObjectPreDelete(fGroup);
	Ufe::Scene::notifyObjectDelete(notification);

	auto prim = fGroup->prim();
	auto stage = prim.GetStage();
	auto usdPath = prim.GetPath();
	stage->RemovePrim(usdPath);

	fGroup.reset();
}

void UsdUndoCreateGroupCommand::redo()
{
	auto hierarchy = Ufe::Hierarchy::hierarchy(fParentItem);
	// See MAYA-92264: redo doesn't work.  PPT, 19-Nov-2018.
	Ufe::SceneItem::Ptr group = hierarchy->createGroup(fName);
	fGroup = std::dynamic_pointer_cast<UsdSceneItem>(group);
}

} // namespace ufe
} // namespace MayaUsd
