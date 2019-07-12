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

#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoDeleteCommand
class MAYAUSD_CORE_PUBLIC UsdUndoDeleteCommand : public Ufe::UndoableCommand
{
public:
	typedef std::shared_ptr<UsdUndoDeleteCommand> Ptr;

	UsdUndoDeleteCommand(const UsdPrim& prim);
	~UsdUndoDeleteCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdUndoDeleteCommand(const UsdUndoDeleteCommand&) = delete;
	UsdUndoDeleteCommand& operator=(const UsdUndoDeleteCommand&) = delete;
	UsdUndoDeleteCommand(UsdUndoDeleteCommand&&) = delete;
	UsdUndoDeleteCommand& operator=(UsdUndoDeleteCommand&&) = delete;

	//! Create a UsdUndoDeleteCommand from a USD prim.
	static UsdUndoDeleteCommand::Ptr create(const UsdPrim& prim);

	// UsdUndoDeleteCommand overrides
	void undo() override;
	void redo() override;

private:
	void perform(bool state);

private:
	UsdPrim fPrim;

}; // UsdUndoDeleteCommand

} // namespace ufe
} // namespace MayaUsd
