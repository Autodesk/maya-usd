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
#include "UsdTranslateUndoableCommand.h"

#include "private/Utils.h"

MAYAUSD_NS_DEF {
namespace ufe {

TfToken UsdTranslateUndoableCommand::xlate("xformOp:translate");

UsdTranslateUndoableCommand::UsdTranslateUndoableCommand(
    const UsdSceneItem::Ptr& item, double x, double y, double z
) : Ufe::TranslateUndoableCommand(item),
    UsdTRSUndoableCommandBase(item, x, y, z)
{}

UsdTranslateUndoableCommand::~UsdTranslateUndoableCommand()
{}

/*static*/
UsdTranslateUndoableCommand::Ptr UsdTranslateUndoableCommand::create(
    const UsdSceneItem::Ptr& item, double x, double y, double z
)
{
    auto cmd = std::make_shared<MakeSharedEnabler<UsdTranslateUndoableCommand>>(
        item, x, y, z);
    cmd->initialize();
    return cmd;
}

void UsdTranslateUndoableCommand::undo()
{
    undoImp();
}

void UsdTranslateUndoableCommand::redo()
{
    redoImp();
}

void UsdTranslateUndoableCommand::addEmptyAttribute()
{
    performImp(0, 0, 0);    // Add an empty translate
}

void UsdTranslateUndoableCommand::performImp(double x, double y, double z)
{
    translateOp(prim(), path(), x, y, z);
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdTranslateUndoableCommand::translate(double x, double y, double z)
{
    perform(x, y, z);
    return true;
}

} // namespace ufe
} // namespace MayaUsd
