//
// Copyright 2023 Autodesk
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

#include "UsdUndoLongDurationCommand.h"

#include <usdUfe/ufe/Utils.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::CompositeUndoableCommand, UsdUndoLongDurationCommand);

/* static */ std::shared_ptr<UsdUndoLongDurationCommand>
UsdUndoLongDurationCommand::create(std::initializer_list<Ptr> undoableCommands)
{
    return std::make_shared<UsdUndoLongDurationCommand>(undoableCommands);
}

UsdUndoLongDurationCommand::UsdUndoLongDurationCommand(std::initializer_list<Ptr> undoableCommands)
    : Parent(undoableCommands)
{
}

UsdUndoLongDurationCommand::UsdUndoLongDurationCommand(const std::list<Ptr>& undoableCommands)
    : Parent(undoableCommands)
{
}

void UsdUndoLongDurationCommand::execute()
{
    WaitCursor waitCursor;
    Parent::execute();
}

void UsdUndoLongDurationCommand::undo()
{
    WaitCursor waitCursor;
    Parent::undo();
}

void UsdUndoLongDurationCommand::redo()
{
    WaitCursor waitCursor;
    Parent::redo();
}

}; // namespace USDUFE_NS_DEF
