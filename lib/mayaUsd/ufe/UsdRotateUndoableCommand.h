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
#pragma once

#include "../base/api.h"

#include <ufe/transform3dUndoableCommands.h>

#include <pxr/usd/usd/attribute.h>

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
