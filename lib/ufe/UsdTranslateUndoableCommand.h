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
