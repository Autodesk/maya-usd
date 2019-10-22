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

#include "UsdRotateUndoableCommand.h"
#include "private/Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdRotateUndoableCommand::UsdRotateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item)
	: Ufe::RotateUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fNoRotateOp(false)
{
	// Since we want to change xformOp:rotateXYZ, and we need to store the prevRotate for 
	// undo purpose, we need to make sure we convert it to common API xformOps (In case we have 
	// rotateX, rotateY or rotateZ ops)
	try {
		convertToCompatibleCommonAPI(prim);
	}
	catch (...) {
		// Since Maya cannot catch this error at this moment, store it until we actually rotate
		fFailedInit = std::current_exception(); // capture
		return;
	}

	// Prim does not have a rotateXYZ attribute
	const TfToken xrot("xformOp:rotateXYZ");
	if (!fPrim.HasAttribute(xrot))
	{
		rotateOp(fPrim, fPath, 0, 0, 0);	// Add an empty rotate
		fNoRotateOp = true;
	}

	fRotateAttrib = fPrim.GetAttribute(xrot);
	fRotateAttrib.Get<GfVec3f>(&fPrevRotateValue);
}

UsdRotateUndoableCommand::~UsdRotateUndoableCommand()
{
}

/*static*/
UsdRotateUndoableCommand::Ptr UsdRotateUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item)
{
	return std::make_shared<UsdRotateUndoableCommand>(prim, ufePath, item);
}

void UsdRotateUndoableCommand::undo()
{
	// Check if initialization went ok.
	if (!fFailedInit)
	{
		fRotateAttrib.Set(fPrevRotateValue);
	}
	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdRotateUndoableCommand::redo()
{
	perform();
}

void UsdRotateUndoableCommand::perform()
{
	// No-op, use rotate to move the object
	// The Maya rotate command directly invokes our rotate() method in its
	// redoIt(), which is invoked both for the inital rotate and the redo.
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
	rotateOp(fPrim, fPath, x, y, z);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
