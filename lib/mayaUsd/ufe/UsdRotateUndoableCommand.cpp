//
// Copyright 2020 Autodesk
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
#include "UsdRotateUndoableCommand.h"

#include "private/Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

TfToken UsdRotateUndoableCommand::rotXYZ("xformOp:rotateXYZ");

UsdRotateUndoableCommand::UsdRotateUndoableCommand(
    const UsdSceneItem::Ptr& item, double x, double y, double z)
	: Ufe::RotateUndoableCommand(item),
      UsdTRSUndoableCommandBase(item, x, y, z)
{
	// Since we want to change xformOp:rotateXYZ, and we need to store the
	// prevRotate for undo purposes, we need to make sure we convert it to
	// common API xformOps (In case we have rotateX, rotateY or rotateZ ops)
	try {
        if(!UsdGeomXformCommonAPI(prim()))
            convertToCompatibleCommonAPI(prim());
	}
	catch (...) {
		// Since Maya cannot catch this error at this moment, store it until we
		// actually rotate.
		fFailedInit = std::current_exception(); // capture
	}
}

UsdRotateUndoableCommand::~UsdRotateUndoableCommand()
{}

/*static*/
UsdRotateUndoableCommand::Ptr UsdRotateUndoableCommand::create(
    const UsdSceneItem::Ptr& item, double x, double y, double z)
{
	auto cmd = std::make_shared<MakeSharedEnabler<UsdRotateUndoableCommand>>(
        item, x, y, z);
    cmd->initialize();
    return cmd;
}

void UsdRotateUndoableCommand::undo()
{
	// Check if initialization went ok.
	if (!fFailedInit)
	{
        UsdTRSUndoableCommandBase::undoImp();
	}
}

void UsdRotateUndoableCommand::redo()
{
    redoImp();
}

void UsdRotateUndoableCommand::addEmptyAttribute()
{
    performImp(0, 0, 0);	// Add an empty rotate
}

void UsdRotateUndoableCommand::performImp(double x, double y, double z)
{
	rotateOp(prim(), path(), x, y, z);
}

//------------------------------------------------------------------------------
// Ufe::RotateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdRotateUndoableCommand::rotate(double x, double y, double z)
{
	// Fail early - Initialization did not go as expected.
	if (fFailedInit)
	{
		std::rethrow_exception(fFailedInit);
	}
	perform(x, y, z);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
