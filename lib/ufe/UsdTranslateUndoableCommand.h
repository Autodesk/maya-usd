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

//! \brief Translation command of the given prim.
/*!
	Ability to perform undo to restore the original translate value.
	As of 06/07/2018, redo is a no op as Maya re-does the operation for redo
 */
class MAYAUSD_CORE_PUBLIC UsdTranslateUndoableCommand : public Ufe::TranslateUndoableCommand
{
public:
	typedef std::shared_ptr<UsdTranslateUndoableCommand> Ptr;

	UsdTranslateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);
	~UsdTranslateUndoableCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdTranslateUndoableCommand(const UsdTranslateUndoableCommand&) = delete;
	UsdTranslateUndoableCommand& operator=(const UsdTranslateUndoableCommand&) = delete;
	UsdTranslateUndoableCommand(UsdTranslateUndoableCommand&&) = delete;
	UsdTranslateUndoableCommand& operator=(UsdTranslateUndoableCommand&&) = delete;

	//! Create a UsdTranslateUndoableCommand from a USD prim, UFE path and UFE scene item.
	static UsdTranslateUndoableCommand::Ptr create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);

	// Ufe::TranslateUndoableCommand overrides
	void undo() override;
	void redo() override;
	bool translate(double x, double y, double z) override;

private:
	void perform();

private:
	UsdPrim fPrim;
	UsdAttribute fTranslateAttrib;
	GfVec3d fPrevTranslateValue;
	Ufe::Path fPath;
	bool fNoTranslateOp;
}; // UsdTranslateUndoableCommand

} // namespace ufe
} // namespace MayaUsd
