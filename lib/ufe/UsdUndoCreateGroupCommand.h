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

#include <ufe/undoableCommand.h>
#include <ufe/pathComponent.h>

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
