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
#ifndef UFE_USD_SELECT_AFTER_COMMAND
#define UFE_USD_SELECT_AFTER_COMMAND

#include <usdUfe/base/api.h>

#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/observableSelection.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief Add post-command selection to another command.
template <class OTHER_COMMAND> class UsdUndoSelectAfterCommand : public OTHER_COMMAND
{
public:
    using OtherCommand = OTHER_COMMAND;
    using Ptr = std::shared_ptr<UsdUndoSelectAfterCommand<OtherCommand>>;

    // Bring in the base class constructors.
    using OtherCommand::OtherCommand;

    ~UsdUndoSelectAfterCommand() override { }

    void execute() override
    {
        _previousSelection = *Ufe::GlobalSelection::get();
        OtherCommand::execute();
        // Note: each class that wish to be able to have the selection set after
        //       must provide an overload of this function. See for example the
        //       implementation for UsdUndoAddNewPrimCommand in its header file.
        //       It is usually a two-line function.
        _newSelection = getNewSelectionFromCommand(*this);
        setSelection(_newSelection);
    }

    void undo() override
    {
        OtherCommand::undo();
        setSelection(_previousSelection);
    }

    void redo() override
    {
        OtherCommand::redo();
        setSelection(_newSelection);
    }

    template <class... ARGS>
    static UsdUndoSelectAfterCommand<OtherCommand>::Ptr create(ARGS... args)
    {
        return std::make_shared<UsdUndoSelectAfterCommand<OtherCommand>>(args...);
    }

private:
    static void setSelection(const Ufe::Selection& newSelection)
    {
        if (newSelection.empty()) {
            Ufe::GlobalSelection::get()->clear();
        } else {
            Ufe::GlobalSelection::get()->replaceWith(newSelection);
        }
    }

    Ufe::Selection _previousSelection;
    Ufe::Selection _newSelection;
};

//! \brief Retrieve the desired selection after the command has executed.
Ufe::Selection USDUFE_PUBLIC getNewSelectionFromCommand(const Ufe::InsertChildCommand& cmd);

} // namespace USDUFE_NS_DEF

#endif /* UFE_USD_SELECT_AFTER_COMMAND */
