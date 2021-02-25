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

#include <mayaUsd/ufe/UsdUndoableCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Templated helper class to factor out common code for interactive
// undoable commands.
//
// Protects against undo/redo/execute being called in unexpected order.
// This happens during interactive manipulations.
template <typename Cmd> class UsdUndoableInteractiveCommand : public UsdUndoableCommand<Cmd>
{
public:
    UsdUndoableInteractiveCommand(const Ufe::Path& path)
        : UsdUndoableCommand<Cmd>(path)
    {
    }

    // Ufe::UndoableCommand overrides.

    // Declares a UsdUndoBlock and calls executeUndoBlock()
    void execute() override
    {
        // Note: see the notes on the State enum why we're redoing instead
        //       of executing when in the undone state.
        if (State::Undone == _state)
            return redo();

        UsdUndoableCommand<Cmd>::execute();
        _state = State::Done;
    }

    // Calls undo on the undoable item.
    void undo() override
    {
        // Note: protect against early undo before execute() has been called.
        if (State::Done != _state)
            return;

        UsdUndoableCommand<Cmd>::undo();
        _state = State::Undone;
    }

    // Calls redo on the undoable item.
    void redo() override
    {
        // Note: protect against early redo before execute() has been called.
        if (State::Undone != _state)
            return;

        UsdUndoableCommand<Cmd>::redo();
        _state = State::Done;
    }

protected:
    // State of the undo/redo.
    //
    // Note: unfortunately, we need to track the initial/done/undo
    //       state ourselves because UFE does *not* call redo() to
    //       redo, but instead calls set() again.
    //
    //       So, if we want to use the USD undo system, we have to
    //       track the state of the undo /redo ourselves so that when
    //       set() is called when we're in a undone state, we do
    //       redo() instead...
    enum class State
    {
        Initial,
        Done,
        Undone
    };

    State _state = State::Initial;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
