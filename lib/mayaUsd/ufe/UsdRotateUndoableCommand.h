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
#pragma once

#include <exception>

#include <ufe/transform3dUndoableCommands.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdTRSUndoableCommandBase.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Absolute rotation command of the given prim.
/*!
	Ability to perform undo to restore the original rotation value.
 */
class MAYAUSD_CORE_PUBLIC UsdRotateUndoableCommand : public Ufe::RotateUndoableCommand
{
public:
	typedef std::shared_ptr<UsdRotateUndoableCommand> Ptr;

	UsdRotateUndoableCommand(const UsdRotateUndoableCommand&) = delete;
	UsdRotateUndoableCommand& operator=(const UsdRotateUndoableCommand&) = delete;
	UsdRotateUndoableCommand(UsdRotateUndoableCommand&&) = delete;
	UsdRotateUndoableCommand& operator=(UsdRotateUndoableCommand&&) = delete;

	//! Create a UsdRotateUndoableCommand from a UFE scene item.  The
	//! command is not executed.
	static UsdRotateUndoableCommand::Ptr create(
        const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode);

	// Ufe::RotateUndoableCommand overrides.  rotate() sets the command's
	// rotation value and executes the command.
	void undo() override;
	void redo() override;
	bool rotate(double x, double y, double z) override;

protected:


    //! Construct a UsdRotateUndoableCommand.  The command is not executed.
	UsdRotateUndoableCommand(const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode);
	~UsdRotateUndoableCommand() override;
	
	UsdPrim fPrim;
	UsdGeomXformOp fOp;
	GfQuatd fPrevValue;
	GfQuatd fNewValue;
	Ufe::Path fPath;
	UsdTimeCode fTimeCode;

}; // UsdRotateUndoableCommand

} // namespace ufe
} // namespace MayaUsd
