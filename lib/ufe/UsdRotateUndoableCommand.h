// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "ufe/transform3dUndoableCommands.h"

#include "pxr/usd/usd/attribute.h"

#include <exception>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Absolute rotation command of the given prim.
/*!
	Ability to perform undo to restore the original rotation value.
	As of 06/07/2018, redo is a no op as Maya re-does the operation for redo
 */
class MAYAUSD_CORE_PUBLIC UsdRotateUndoableCommand : public Ufe::RotateUndoableCommand
{
public:
	typedef std::shared_ptr<UsdRotateUndoableCommand> Ptr;

	UsdRotateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);
	~UsdRotateUndoableCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdRotateUndoableCommand(const UsdRotateUndoableCommand&) = delete;
	UsdRotateUndoableCommand& operator=(const UsdRotateUndoableCommand&) = delete;
	UsdRotateUndoableCommand(UsdRotateUndoableCommand&&) = delete;
	UsdRotateUndoableCommand& operator=(UsdRotateUndoableCommand&&) = delete;

	//! Create a UsdRotateUndoableCommand from a USD prim, UFE path and UFE scene item.
	static UsdRotateUndoableCommand::Ptr create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);

	// Ufe::RotateUndoableCommand overrides
	void undo() override;
	void redo() override;
	bool rotate(double x, double y, double z) override;

private:
	void perform();

private:
	UsdPrim fPrim;
	Ufe::Path fPath;
	UsdAttribute fRotateAttrib;
	GfVec3f fPrevRotateValue;
	std::exception_ptr fFailedInit;
	bool fNoRotateOp;
}; // UsdRotateUndoableCommand

} // namespace ufe
} // namespace MayaUsd
