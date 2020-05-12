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
#include "UsdRotatePivotTranslateUndoableCommand.h"

#include "private/Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdRotatePivotTranslateUndoableCommand::UsdRotatePivotTranslateUndoableCommand(
    const UsdSceneItem::Ptr& item, GfVec3f rotPivot
) : Ufe::TranslateUndoableCommand(item),
    UsdTRSUndoableCommandBase(item, std::move(rotPivot), UsdGeomXformOp::Type::TypeTranslate)
{}

UsdRotatePivotTranslateUndoableCommand::~UsdRotatePivotTranslateUndoableCommand()
{}

/*static*/
UsdRotatePivotTranslateUndoableCommand::Ptr UsdRotatePivotTranslateUndoableCommand::create(
    const UsdSceneItem::Ptr& item, GfVec3f rotPivot
)
{
	auto cmd = std::make_shared<MakeSharedEnabler<UsdRotatePivotTranslateUndoableCommand>>(
        item, std::move(rotPivot));
    cmd->initialize();
    return cmd;
}

void UsdRotatePivotTranslateUndoableCommand::undo()
{
    UsdTRSUndoableCommandBase::undo();
}

void UsdRotatePivotTranslateUndoableCommand::redo()
{
    UsdTRSUndoableCommandBase::redo();
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdRotatePivotTranslateUndoableCommand::translate(double x, double y, double z)
{
	perform(x, y, z);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
