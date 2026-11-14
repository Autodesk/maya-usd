//
// Copyright 2026 Autodesk
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
#include "UsdUndoInsertChildAndSelectCommand.h"

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/selection.h>
#include <ufe/selectionUndoableCommands.h>

namespace USDUFE_NS_DEF {

UsdUndoInsertChildAndSelectCommand::UsdUndoInsertChildAndSelectCommand(
    const Ufe::InsertChildCommand::Ptr& creationCmd)
    : Ufe::CompositeUndoableCommand({ creationCmd })
{
}

void UsdUndoInsertChildAndSelectCommand::execute()
{
    auto insertChildCmd = std::dynamic_pointer_cast<Ufe::InsertChildCommand>(cmdsList().front());
    insertChildCmd->execute();
    // Create the selection command only if the creation succeeded:
    if (insertChildCmd->insertedChild()) {
        Ufe::Selection newSelection;
        newSelection.append(insertChildCmd->insertedChild());
        append(
            Ufe::SelectionReplaceWith::createAndExecute(Ufe::GlobalSelection::get(), newSelection));
    }
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string UsdUndoInsertChildAndSelectCommand::commandString() const
{
    return cmdsList().front()->commandString();
}
#endif

} // namespace USDUFE_NS_DEF
