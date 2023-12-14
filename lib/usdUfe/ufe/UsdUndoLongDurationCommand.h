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
#pragma once

#include <usdUfe/base/api.h>

#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoLongDurationCommand
//
// A composite command that wraps all sub-commands within a user-visible wait cursor.
class USDUFE_PUBLIC UsdUndoLongDurationCommand : public Ufe::CompositeUndoableCommand
{
public:
    using Parent = Ufe::CompositeUndoableCommand;

    //! Create the long-duration composite command and append the argument commands to it.
    //! \return Pointer to the long-duration composite undoable command.
    static std::shared_ptr<UsdUndoLongDurationCommand>
    create(std::initializer_list<Ptr> undoableCommands);

    //@{
    //! Constructors.
    UsdUndoLongDurationCommand();
    UsdUndoLongDurationCommand(std::initializer_list<Ptr> undoableCommands);
    UsdUndoLongDurationCommand(const std::list<Ptr>& undoableCommands);

    //@}
    //! Destructor.
    ~UsdUndoLongDurationCommand() override;

    //! Calls execute() on each command in the list, in forward order.
    void execute() override;

    //! Calls undo() on each command in the list, in reverse order.
    void undo() override;

    //! Calls redo() on each command in the list, in forward order.
    void redo() override;
};

} // namespace USDUFE_NS_DEF
