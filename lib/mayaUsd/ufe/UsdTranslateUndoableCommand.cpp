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

#include "UsdTranslateUndoableCommand.h"
#include "private/Utils.h"
#include "Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdTranslateUndoableCommand::UsdTranslateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item)
	: Ufe::TranslateUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fNoTranslateOp(false)
{
	// Prim does not have a translate attribute
	const TfToken xlate("xformOp:translate");
	if (!fPrim.HasAttribute(xlate))
	{
		fNoTranslateOp = true;
		translateOp(fPrim, fPath, 0, 0, 0);	// Add an empty translate
	}

	fTranslateAttrib = fPrim.GetAttribute(xlate);
	fTranslateAttrib.Get<GfVec3d>(&fPrevTranslateValue);
}

UsdTranslateUndoableCommand::~UsdTranslateUndoableCommand()
{
}

/*static*/
UsdTranslateUndoableCommand::Ptr UsdTranslateUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item)
{
	return std::make_shared<UsdTranslateUndoableCommand>(prim, ufePath, item);
}

void UsdTranslateUndoableCommand::undo()
{
	fTranslateAttrib.Set(fPrevTranslateValue);
	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdTranslateUndoableCommand::redo()
{
	perform();
}

void UsdTranslateUndoableCommand::perform()
{
	// No-op, use translate to move the object.
	// The Maya move command directly invokes our translate() method in its
	// redoIt(), which is invoked both for the inital move and the redo.
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdTranslateUndoableCommand::translate(double x, double y, double z)
{
	translateOp(fPrim, fPath, x, y, z);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
