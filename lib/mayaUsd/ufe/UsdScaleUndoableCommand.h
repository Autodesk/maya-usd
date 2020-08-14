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

#include <ufe/transform3dUndoableCommands.h>

#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdTRSUndoableCommandBase.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Absolute scale command of the given prim.
/*!
	Ability to perform undo to restore the original scale value.
 */
class MAYAUSD_CORE_PUBLIC UsdScaleUndoableCommand : public Ufe::ScaleUndoableCommand
{
public:
	typedef std::shared_ptr<UsdScaleUndoableCommand> Ptr;

	UsdScaleUndoableCommand(const UsdScaleUndoableCommand&) = delete;
	UsdScaleUndoableCommand& operator=(const UsdScaleUndoableCommand&) = delete;
	UsdScaleUndoableCommand(UsdScaleUndoableCommand&&) = delete;
	UsdScaleUndoableCommand& operator=(UsdScaleUndoableCommand&&) = delete;

	//! Create a UsdScaleUndoableCommand from a UFE scene item.  The command is
    //! not executed.
	static UsdScaleUndoableCommand::Ptr create(
        const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode);

	// Ufe::ScaleUndoableCommand overrides
	void undo() override;
	void redo() override;
	bool scale(double x, double y, double z) override;

protected:

    //! Construct a UsdScaleUndoableCommand.  The command is not executed.
	UsdScaleUndoableCommand(const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode);
	~UsdScaleUndoableCommand() override;

	UsdPrim fPrim;
	UsdGeomXformOp fOp;
	GfVec3d fPrevValue;
	GfVec3d fNewValue;
	Ufe::Path fPath;
	UsdTimeCode fTimeCode;

}; // UsdScaleUndoableCommand

} // namespace ufe
} // namespace MayaUsd
