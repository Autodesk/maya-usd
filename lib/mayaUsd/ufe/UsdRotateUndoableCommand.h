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

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdTRSUndoableCommandBase.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Absolute rotation command of the given prim.
/*!
	Ability to perform undo to restore the original rotation value.
 */
class MAYAUSD_CORE_PUBLIC UsdRotateUndoableCommand : public Ufe::RotateUndoableCommand, public UsdTRSUndoableCommandBase<GfVec3f>
{
public:
	typedef std::shared_ptr<UsdRotateUndoableCommand> Ptr;

	UsdRotateUndoableCommand(const UsdRotateUndoableCommand&) = delete;
	UsdRotateUndoableCommand& operator=(const UsdRotateUndoableCommand&) = delete;
	UsdRotateUndoableCommand(UsdRotateUndoableCommand&&) = delete;
	UsdRotateUndoableCommand& operator=(UsdRotateUndoableCommand&&) = delete;

	#ifdef UFE_V2_FEATURES_AVAILABLE
	//! Create a UsdRotateUndoableCommand from a UFE scene path.  The command is
    //! not executed.
	static UsdRotateUndoableCommand::Ptr create(
        const Ufe::Path& path, double x, double y, double z);
	#else
	//! Create a UsdRotateUndoableCommand from a UFE scene item.  The command is
    //! not executed.
	static UsdRotateUndoableCommand::Ptr create(
        const UsdSceneItem::Ptr& item, double x, double y, double z);
	#endif

	// Ufe::RotateUndoableCommand overrides.  rotate() sets the command's
	// rotation value and executes the command.
	void undo() override;
	void redo() override;
#if UFE_PREVIEW_VERSION_NUM >= 2025
	bool set(double x, double y, double z) override;
#else
	bool rotate(double x, double y, double z) override;
#endif

	#ifdef UFE_V2_FEATURES_AVAILABLE
	Ufe::Path getPath() const override { return path(); }
	#endif

protected:

    //! Construct a UsdRotateUndoableCommand.  The command is not executed.
	#ifdef UFE_V2_FEATURES_AVAILABLE
	UsdRotateUndoableCommand(const Ufe::Path& path, double x, double y, double z);
	#else
	UsdRotateUndoableCommand(const UsdSceneItem::Ptr& item, double x, double y, double z);
	#endif
	~UsdRotateUndoableCommand() override;

private:

    static TfToken rotXYZ;

    TfToken attributeName() const override { return rotXYZ; }
    void performImp(double x, double y, double z) override;
    void addEmptyAttribute() override;
    bool cannotInit() const override { return bool(fFailedInit); }

	std::exception_ptr fFailedInit;

}; // UsdRotateUndoableCommand

} // namespace ufe
} // namespace MayaUsd
