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

#include <ufe/path.h>
#include <ufe/pathComponent.h>
#include <ufe/undoableCommand.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoRenameCommand
class MAYAUSD_CORE_PUBLIC UsdUndoRenameCommand : public Ufe::UndoableCommand
{
public:
	typedef std::shared_ptr<UsdUndoRenameCommand> Ptr;

	UsdUndoRenameCommand(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName);
	~UsdUndoRenameCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdUndoRenameCommand(const UsdUndoRenameCommand&) = delete;
	UsdUndoRenameCommand& operator=(const UsdUndoRenameCommand&) = delete;
	UsdUndoRenameCommand(UsdUndoRenameCommand&&) = delete;
	UsdUndoRenameCommand& operator=(UsdUndoRenameCommand&&) = delete;

	//! Create a UsdUndoRenameCommand from a USD scene item and UFE pathcomponent.
	static UsdUndoRenameCommand::Ptr create(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName);

	UsdSceneItem::Ptr renamedItem() const;

	//! Rename the prim hierarchy at usdSrcPath to usdDstPath.
	bool rename(SdfLayerHandle layer, const Ufe::Path& ufeSrcPath, const SdfPath& usdSrcPath, const SdfPath& usdDstPath);

	// UsdUndoRenameCommand overrides
	void undo() override;
	void redo() override;

private:
	bool internalRename(SdfLayerHandle layer, const Ufe::Path& ufeSrcPath, const SdfPath& usdSrcPath, const SdfPath& usdDstPath);

private:
	UsdStageWeakPtr fStage;
	SdfLayerHandle fLayer;
	Ufe::Path fUfeSrcPath;
	SdfPath fUsdSrcPath;
	UsdSceneItem::Ptr fUfeDstItem;
	SdfPath fUsdDstPath;

}; // UsdUndoRenameCommand

} // namespace ufe
} // namespace MayaUsd
