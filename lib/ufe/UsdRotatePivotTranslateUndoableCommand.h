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

//! \brief Absolute translation command of the given prim's rotate pivot.
/*!
	Ability to perform undo to restore the original pivot value.
 */
class MAYAUSD_CORE_PUBLIC UsdRotatePivotTranslateUndoableCommand : public Ufe::TranslateUndoableCommand
{
public:
	typedef std::shared_ptr<UsdRotatePivotTranslateUndoableCommand> Ptr;

	UsdRotatePivotTranslateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);
	~UsdRotatePivotTranslateUndoableCommand() override;

	// Delete the copy/move constructors assignment operators.
	UsdRotatePivotTranslateUndoableCommand(const UsdRotatePivotTranslateUndoableCommand&) = delete;
	UsdRotatePivotTranslateUndoableCommand& operator=(const UsdRotatePivotTranslateUndoableCommand&) = delete;
	UsdRotatePivotTranslateUndoableCommand(UsdRotatePivotTranslateUndoableCommand&&) = delete;
	UsdRotatePivotTranslateUndoableCommand& operator=(UsdRotatePivotTranslateUndoableCommand&&) = delete;

	//! Create a UsdRotatePivotTranslateUndoableCommand from a USD prim, UFE path and UFE scene item.
	static UsdRotatePivotTranslateUndoableCommand::Ptr create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item);

	// Ufe::TranslateUndoableCommand overrides
	void undo() override;
	void redo() override;
	bool translate(double x, double y, double z) override;

private:
	UsdPrim fPrim;
	UsdAttribute fPivotAttrib;
	GfVec3f fPrevPivotValue;
	Ufe::Path fPath;
	bool fNoPivotOp;

}; // UsdRotatePivotTranslateUndoableCommand

} // namespace ufe
} // namespace MayaUsd
