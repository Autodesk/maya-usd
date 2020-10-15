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

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdTRSUndoableCommandBase.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Translation command of the given prim.
/*!
	Ability to perform undo to restore the original translate value.
 */
class MAYAUSD_CORE_PUBLIC UsdTranslateUndoableCommand : public Ufe::TranslateUndoableCommand, public UsdTRSUndoableCommandBase<GfVec3d>
{
public:
	typedef std::shared_ptr<UsdTranslateUndoableCommand> Ptr;

	UsdTranslateUndoableCommand(const UsdTranslateUndoableCommand&) = delete;
	UsdTranslateUndoableCommand& operator=(const UsdTranslateUndoableCommand&) = delete;
	UsdTranslateUndoableCommand(UsdTranslateUndoableCommand&&) = delete;
	UsdTranslateUndoableCommand& operator=(UsdTranslateUndoableCommand&&) = delete;

	#ifdef UFE_V2_FEATURES_AVAILABLE
	//! Create a UsdTranslateUndoableCommand from a UFE scene path. The
	//! command is not executed.
	static UsdTranslateUndoableCommand::Ptr create(
        const Ufe::Path& path, double x, double y, double z);
	#else
	//! Create a UsdTranslateUndoableCommand from a UFE scene item.  The
	//! command is not executed.
	static UsdTranslateUndoableCommand::Ptr create(
        const UsdSceneItem::Ptr& item, double x, double y, double z);
	#endif

	// Ufe::TranslateUndoableCommand overrides.  set() sets the command's
	// translation value and executes the command.
	void undo() override;
	void redo() override;
#if UFE_PREVIEW_VERSION_NUM >= 2025
//#ifdef UFE_V2_FEATURES_AVAILABLE
	bool set(double x, double y, double z) override;
#else
	bool translate(double x, double y, double z) override;
#endif

	#ifdef UFE_V2_FEATURES_AVAILABLE
	Ufe::Path getPath() const override { return path(); }
	#endif

protected:

    //! Construct a UsdTranslateUndoableCommand.  The command is not executed.
	#ifdef UFE_V2_FEATURES_AVAILABLE
	UsdTranslateUndoableCommand(const Ufe::Path& path, double x, double y, double z);
	#else
	UsdTranslateUndoableCommand(const UsdSceneItem::Ptr& item, double x, double y, double z);
	#endif

	~UsdTranslateUndoableCommand() override;

private:

    static TfToken xlate;

    TfToken attributeName() const override { return xlate; }
    void performImp(double x, double y, double z) override;
    void addEmptyAttribute() override;

}; // UsdTranslateUndoableCommand

} // namespace ufe
} // namespace MayaUsd
