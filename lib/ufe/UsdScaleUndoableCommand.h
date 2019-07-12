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

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Absolute scale command of the given prim.
/*!
	Ability to perform undo to restore the original scale value.
	As of 06/07/2018, redo is a no op as Maya re-does the operation for redo
 */
class MAYAUSD_CORE_PUBLIC UsdScaleUndoableCommand : public Ufe::ScaleUndoableCommand
{
public:
	typedef std::shared_ptr<UsdScaleUndoableCommand> Ptr;

	UsdScaleUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);
	~UsdScaleUndoableCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdScaleUndoableCommand(const UsdScaleUndoableCommand&) = delete;
	UsdScaleUndoableCommand& operator=(const UsdScaleUndoableCommand&) = delete;
	UsdScaleUndoableCommand(UsdScaleUndoableCommand&&) = delete;
	UsdScaleUndoableCommand& operator=(UsdScaleUndoableCommand&&) = delete;

	//! Create a UsdScaleUndoableCommand from a USD prim, UFE path and UFE scene item.
	static UsdScaleUndoableCommand::Ptr create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);

	// Ufe::ScaleUndoableCommand overrides
	void undo() override;
	void redo() override;
	bool scale(double x, double y, double z) override;

private:
	void perform();

private:
	UsdPrim fPrim;
	UsdAttribute fScaleAttrib;
	GfVec3f fPrevScaleValue;
	Ufe::Path fPath;
	bool fNoScaleOp;
}; // UsdScaleUndoableCommand

} // namespace ufe
} // namespace MayaUsd
