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

#include <ufe/transform3dUndoableCommands.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdTRSUndoableCommandBase.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {


//! \brief Translation command of the given prim.
/*!
	Ability to perform undo to restore the original translate value.
 */
class MAYAUSD_CORE_PUBLIC UsdTranslateUndoableCommand : public Ufe::TranslateUndoableCommand
{
public:
	typedef std::shared_ptr<UsdTranslateUndoableCommand> Ptr;

	UsdTranslateUndoableCommand(const UsdTranslateUndoableCommand&) = delete;
	UsdTranslateUndoableCommand& operator=(const UsdTranslateUndoableCommand&) = delete;
	UsdTranslateUndoableCommand(UsdTranslateUndoableCommand&&) = delete;
	UsdTranslateUndoableCommand& operator=(UsdTranslateUndoableCommand&&) = delete;

	//! Create a UsdTranslateUndoableCommand from a UFE scene item.  The
	//! command is not executed.
	static UsdTranslateUndoableCommand::Ptr create(
        const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode);

	// Ufe::TranslateUndoableCommand overrides.  translate() sets the command's
	// translation value and executes the command.
	void undo() override;
	void redo() override;
	bool translate(double x, double y, double z) override;
    // Overridden from Ufe::Observer
    //void operator()(const Ufe::Notification& notification) override;

protected:

	UsdPrim fPrim;
	UsdGeomXformOp fOp;
	GfVec3d fPrevValue;
	GfVec3d fNewValue;
	Ufe::Path fPath;
	UsdTimeCode fTimeCode;

    //! Construct a UsdTranslateUndoableCommand.  The command is not executed.
	UsdTranslateUndoableCommand(const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode);
	~UsdTranslateUndoableCommand() override;

}; // UsdTranslateUndoableCommand

} // namespace ufe
} // namespace MayaUsd
