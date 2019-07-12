// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdUndoDeleteCommand.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDeleteCommand::UsdUndoDeleteCommand(const UsdPrim& prim)
	: Ufe::UndoableCommand()
	, fPrim(prim)
{
}

UsdUndoDeleteCommand::~UsdUndoDeleteCommand()
{
}

/*static*/
UsdUndoDeleteCommand::Ptr UsdUndoDeleteCommand::create(const UsdPrim& prim)
{
	return std::make_shared<UsdUndoDeleteCommand>(prim);
}

void UsdUndoDeleteCommand::perform(bool state)
{
	fPrim.SetActive(state);
}

//------------------------------------------------------------------------------
// UsdUndoDeleteCommand overrides
//------------------------------------------------------------------------------

void UsdUndoDeleteCommand::undo()
{
	perform(true);
}

void UsdUndoDeleteCommand::redo()
{
	perform(false);
}

} // namespace ufe
} // namespace MayaUsd
