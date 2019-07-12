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

#include "ufe/undoableCommand.h"
#include "ufe/pathComponent.h"

//PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoCreateGroupCommand
class MAYAUSD_CORE_PUBLIC UsdUndoCreateGroupCommand : public Ufe::UndoableCommand
{
public:
	typedef std::shared_ptr<UsdUndoCreateGroupCommand> Ptr;

	UsdUndoCreateGroupCommand(const UsdSceneItem::Ptr& parentItem, const Ufe::PathComponent& name);
	~UsdUndoCreateGroupCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdUndoCreateGroupCommand(const UsdUndoCreateGroupCommand&) = delete;
	UsdUndoCreateGroupCommand& operator=(const UsdUndoCreateGroupCommand&) = delete;
	UsdUndoCreateGroupCommand(UsdUndoCreateGroupCommand&&) = delete;
	UsdUndoCreateGroupCommand& operator=(UsdUndoCreateGroupCommand&&) = delete;

	//! Create a UsdUndoCreateGroupCommand from a USD scene item and a UFE path component.
	static UsdUndoCreateGroupCommand::Ptr create(const UsdSceneItem::Ptr& parentItem, const Ufe::PathComponent& name);

	Ufe::SceneItem::Ptr group() const;

	// UsdUndoCreateGroupCommand overrides
	void undo() override;
	void redo() override;

private:
	UsdSceneItem::Ptr fParentItem;
	Ufe::PathComponent fName;
	UsdSceneItem::Ptr fGroup;

}; // UsdUndoCreateGroupCommand

} // namespace ufe
} // namespace MayaUsd
