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

#include "UsdUndoCreateGroupCommand.h"

#include <ufe/scene.h>
#include <ufe/hierarchy.h>
#include <ufe/sceneNotification.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

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
