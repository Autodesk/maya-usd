// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

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
