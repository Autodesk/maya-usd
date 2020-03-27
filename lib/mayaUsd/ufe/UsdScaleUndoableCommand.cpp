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

#include "UsdScaleUndoableCommand.h"
#include "private/Utils.h"
#include "Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdScaleUndoableCommand::UsdScaleUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item)
	: Ufe::ScaleUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fNoScaleOp(false)
{
	// Prim does not have a scale attribute
	const TfToken xscale("xformOp:scale");
	if (!fPrim.HasAttribute(xscale))
	{
		fNoScaleOp = true;
		scaleOp(fPrim, fPath, 1, 1, 1);	// Add a neutral scale xformOp.
	}

	fScaleAttrib = fPrim.GetAttribute(xscale);
	fScaleAttrib.Get<GfVec3f>(&fPrevScaleValue);
}

UsdScaleUndoableCommand::~UsdScaleUndoableCommand()
{
}

/*static*/
UsdScaleUndoableCommand::Ptr UsdScaleUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item)
{
	return std::make_shared<UsdScaleUndoableCommand>(prim, ufePath, item);
}

void UsdScaleUndoableCommand::undo()
{
	fScaleAttrib.Set(fPrevScaleValue);
	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdScaleUndoableCommand::redo()
{
	perform();
}

void UsdScaleUndoableCommand::perform()
{
	// No-op, use scale to scale the object.
	// The Maya scale command directly invokes our scale() method in its
	// redoIt(), which is invoked both for the inital scale and the redo.
}

//------------------------------------------------------------------------------
// Ufe::ScaleUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdScaleUndoableCommand::scale(double x, double y, double z)
{
	scaleOp(fPrim, fPath, x, y, z);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
