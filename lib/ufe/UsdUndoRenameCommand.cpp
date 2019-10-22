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

#include "UsdUndoRenameCommand.h"
#include "Utils.h"
#include "private/InPathChange.h"

#include "ufe/scene.h"
#include "ufe/sceneNotification.h"
#include "ufe/log.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/copyUtils.h"
#include "pxr/base/tf/token.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoRenameCommand::UsdUndoRenameCommand(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
	: Ufe::UndoableCommand()
{
	const UsdPrim& prim = srcItem->prim();
	fStage = prim.GetStage();
	fUfeSrcPath = srcItem->path();
	fUsdSrcPath = prim.GetPath();
	// Every call to rename() (through execute(), undo() or redo()) removes
	// a prim, which becomes expired.  Since USD UFE scene items contain a
	// prim, we must recreate them after every call to rename.
	fUsdDstPath = prim.GetParent().GetPath().AppendChild(TfToken(newName.string()));
	fLayer = defPrimSpecLayer(prim);
	if (fLayer) {
		std::string err = TfStringPrintf("No prim found at %s", prim.GetPath().GetString().c_str());
		throw std::runtime_error(err.c_str());
	}
}

UsdUndoRenameCommand::~UsdUndoRenameCommand()
{
}

/*static*/
UsdUndoRenameCommand::Ptr UsdUndoRenameCommand::create(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
{
	return std::make_shared<UsdUndoRenameCommand>(srcItem, newName);
}

UsdSceneItem::Ptr UsdUndoRenameCommand::renamedItem() const
{
	return fUfeDstItem;
}

bool UsdUndoRenameCommand::rename(SdfLayerHandle layer, const Ufe::Path& ufeSrcPath, const SdfPath& usdSrcPath, const SdfPath& usdDstPath)
{
	InPathChange pc;
	return internalRename(layer, ufeSrcPath, usdSrcPath, usdDstPath);
}

bool UsdUndoRenameCommand::internalRename(SdfLayerHandle layer, const Ufe::Path& ufeSrcPath, const SdfPath& usdSrcPath, const SdfPath& usdDstPath)
{
	// We use the source layer as the destination.  An alternate workflow
	// would be the edit target layer be the destination:
	// layer = self._stage.GetEditTarget().GetLayer()
	bool status = SdfCopySpec(layer, usdSrcPath, layer, usdDstPath);
	if (status)
	{
		fStage->RemovePrim(usdSrcPath);
		// The renamed scene item is a "sibling" of its original name.
		fUfeDstItem = createSiblingSceneItem(
			ufeSrcPath, usdDstPath.GetElementString());
		// USD sends two ResyncedPaths() notifications, one for the CopySpec
		// call, the other for the RemovePrim call (new name added, old name
		// removed).  Unfortunately, the rename semantics are lost: there is
		// no notion that the two notifications belong to the same atomic
		// change.  Provide a single Rename notification ourselves here.
		Ufe::ObjectRename notification(fUfeDstItem, ufeSrcPath);
		Ufe::Scene::notifyObjectPathChange(notification);
	}

	return status;
}

//------------------------------------------------------------------------------
// UsdUndoRenameCommand overrides
//------------------------------------------------------------------------------

void UsdUndoRenameCommand::undo()
{
	// MAYA-92264: Pixar bug prevents undo from working.  Try again with USD
	// version 0.8.5 or later.  PPT, 7-Jul-2018.
	try {
		rename(fLayer, fUfeDstItem->path(), fUsdDstPath, fUsdSrcPath);
	}
	catch (const std::exception& e) {
		UFE_LOG(e.what());
		throw;	// re-throw the same exception
	}
}

void UsdUndoRenameCommand::redo()
{
	rename(fLayer, fUfeSrcPath, fUsdSrcPath, fUsdDstPath);
}

} // namespace ufe
} // namespace MayaUsd
