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

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/log.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>
#else
#include <cassert>
#endif

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/base/tf/token.h>

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoRenameCommand::UsdUndoRenameCommand(const UsdSceneItem::Ptr& srcItem, const Ufe::PathComponent& newName)
	: Ufe::UndoableCommand()
{
	const UsdPrim& prim = srcItem->prim();
	fStage = prim.GetStage();
	fUfeSrcItem = srcItem;
	fUsdSrcPath = prim.GetPath();
	// Every call to rename() (through execute(), undo() or redo()) removes
	// a prim, which becomes expired.  Since USD UFE scene items contain a
	// prim, we must recreate them after every call to rename.
	fUsdDstPath = prim.GetParent().GetPath().AppendChild(TfToken(newName.string()));
	fLayer = defPrimSpecLayer(prim);
	if (!fLayer) {
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

bool UsdUndoRenameCommand::renameRedo()
{
    // Copy the source path using CopySpec, and inactivate the source.

	// We use the source layer as the destination.  An alternate workflow
	// would be the edit target layer be the destination:
	// layer = self.fStage.GetEditTarget().GetLayer()
	bool status = SdfCopySpec(fLayer, fUsdSrcPath, fLayer, fUsdDstPath);
	if (status)
	{
        auto srcPrim = fStage->GetPrimAtPath(fUsdSrcPath);
#ifdef UFE_V2_FEATURES_AVAILABLE
        UFE_ASSERT_MSG(srcPrim, "Invalid prim cannot be inactivated.");
#else
        assert(srcPrim);
#endif
        status = srcPrim.SetActive(false);

        if (status) {
            // The renamed scene item is a "sibling" of its original name.
            auto ufeSrcPath = fUfeSrcItem->path();
            fUfeDstItem = createSiblingSceneItem(
                ufeSrcPath, fUsdDstPath.GetElementString());

            Ufe::ObjectRename notification(fUfeDstItem, ufeSrcPath);
            Ufe::Scene::notifyObjectPathChange(notification);
        }
    }
    else {
        UFE_LOG(std::string("Warning: SdfCopySpec(") +
                fUsdSrcPath.GetString() + std::string(") failed."));
    }

	return status;
}

bool UsdUndoRenameCommand::renameUndo()
{
    bool status{false};
    {
        // Regardless of where the edit target is currently set, switch to the
        // layer where we copied the source prim into the destination, then
        // restore the edit target.
        UsdEditContext ctx(fStage, fLayer);
        status = fStage->RemovePrim(fUsdDstPath);
    }
    if (status) {
        auto srcPrim = fStage->GetPrimAtPath(fUsdSrcPath);
#ifdef UFE_V2_FEATURES_AVAILABLE
        UFE_ASSERT_MSG(srcPrim, "Invalid prim cannot be activated.");
#else
        assert(srcPrim);
#endif
        status = srcPrim.SetActive(true);

        if (status) {
            Ufe::ObjectRename notification(fUfeSrcItem, fUfeDstItem->path());
            Ufe::Scene::notifyObjectPathChange(notification);
            fUfeDstItem = nullptr;
        }
	}
    else {
        UFE_LOG(std::string("Warning: RemovePrim(") +
                fUsdDstPath.GetString() + std::string(") failed."));
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
        InPathChange pc;
		if (!renameUndo()) {
            UFE_LOG("rename undo failed");
        }
	}
	catch (const std::exception& e) {
		UFE_LOG(e.what());
		throw;	// re-throw the same exception
	}
}

void UsdUndoRenameCommand::redo()
{
    InPathChange pc;
	if (!renameRedo()) {
        UFE_LOG("rename redo failed");
    }
}

} // namespace ufe
} // namespace MayaUsd
