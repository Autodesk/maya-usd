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

#include <ufe/hierarchy.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#if UFE_PREVIEW_VERSION_NUM >= 2017
#include <mayaUsd/ufe/UsdUndoAddNewPrimCommand.h>
#endif

MAYAUSD_NS_DEF {
namespace ufe {

#if UFE_PREVIEW_VERSION_NUM >= 2017

UsdUndoCreateGroupCommand::UsdUndoCreateGroupCommand(const UsdSceneItem::Ptr& parentItem, const Ufe::Selection& selection, const Ufe::PathComponent& name)
	: Ufe::CompositeUndoableCommand()
	, _parentItem(parentItem)
	, _name(name)
	, _selection(selection)
{
}

UsdUndoCreateGroupCommand::~UsdUndoCreateGroupCommand()
{
}

UsdUndoCreateGroupCommand::Ptr UsdUndoCreateGroupCommand::create(const UsdSceneItem::Ptr& parentItem, const Ufe::Selection& selection, const Ufe::PathComponent& name)
{
	return std::make_shared<UsdUndoCreateGroupCommand>(parentItem, selection, name);
}

Ufe::SceneItem::Ptr UsdUndoCreateGroupCommand::group() const
{
	return _group;
}

//------------------------------------------------------------------------------
// UsdUndoCreateGroupCommand overrides
//------------------------------------------------------------------------------

void UsdUndoCreateGroupCommand::execute()
{
	auto addPrimCmd =  UsdUndoAddNewPrimCommand::create(_parentItem, _name.string(), "Xform");
	append(addPrimCmd);
	addPrimCmd->execute();
	
	_group = UsdSceneItem::create(addPrimCmd->newUfePath(), addPrimCmd->newPrim());

	auto newParentHierarchy = Ufe::Hierarchy::hierarchy(_group);
	if (newParentHierarchy) {
		for (auto child : _selection) {
			auto parentCmd = newParentHierarchy->appendChildCmd(child);
			parentCmd->execute();
			append(parentCmd);
		}
	}
}

#else // UFE_PREVIEW_VERSION_NUM

UsdUndoCreateGroupCommand::UsdUndoCreateGroupCommand(const UsdSceneItem::Ptr& parentItem, const Ufe::PathComponent& name)
	: Ufe::UndoableCommand()
	, _parentItem(parentItem)
	, _name(name)
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
	return _group;
}

//------------------------------------------------------------------------------
// UsdUndoCreateGroupCommand overrides
//------------------------------------------------------------------------------

void UsdUndoCreateGroupCommand::undo()
{
	if (!_group) return;

	// See UsdUndoDuplicateCommand.undo() comments.
	auto notification = Ufe::ObjectPreDelete(_group);
	Ufe::Scene::notifyObjectDelete(notification);

	auto prim = _group->prim();
	auto stage = prim.GetStage();
	auto usdPath = prim.GetPath();
	stage->RemovePrim(usdPath);

	_group.reset();
}

void UsdUndoCreateGroupCommand::redo()
{
	auto hierarchy = Ufe::Hierarchy::hierarchy(_parentItem);
	// See MAYA-92264: redo doesn't work.  PPT, 19-Nov-2018.
	Ufe::SceneItem::Ptr group = hierarchy->createGroup(_name);
	_group = std::dynamic_pointer_cast<UsdSceneItem>(group);
}

#endif // #if UFE_PREVIEW_VERSION_NUM < 2017
} // namespace ufe
} // namespace MayaUsd
