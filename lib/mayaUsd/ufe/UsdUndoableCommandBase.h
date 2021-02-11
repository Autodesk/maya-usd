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

#include <mayaUsd/base/api.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <ufe/baseUndoableCommands.h>
#include <ufe/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Templated helper class to factor out common code for undoable commands.
//
// Implement the execute, undo and redo of the UFE command interfaces,
// declaring the UsdUndoBlock during the execution.
//
// Sub-classes only need to implement the executeImpl() function.
//
// Sub-classes can override set() if the command support interactive
// manipulations. Since each set() in specific to the specific Cmd base
// class, we cannot provide a generic implementation in this template.
//
// A typical set() implementation should preserve the value to be set
// and then call execute() to actually set the value.
template <typename Cmd> class UsdUndoableCommandBase : public Cmd
{
public:
#ifdef UFE_V2_FEATURES_AVAILABLE
    UsdUndoableCommandBase(const Ufe::Path& path);
#else
    UsdUndoableCommandBase(const UsdSceneItem::Ptr& item);
#endif

    // Ufe::UndoableCommand overrides.

    // Declares a UsdUndoBlock and calls executeImpl()
    void execute() override;
    // Calls undo on the undoable item.
    void undo() override;
    // Calls redo on the undoable item.
    void redo() override;

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

    // Actual implementation of the execution of the command,
    // executed "within" a UsdUndoBlock to capture undo data,
    // to be implemented by the sub-class.
    virtual void executeImpl(State prevState, State newState) = 0;

private:
    UsdUndoableItem _undoableItem;
    State           _state = State::Initial;
};

// Implementation of the command base class.

template <typename Cmd>
#ifdef UFE_V2_FEATURES_AVAILABLE
inline UsdUndoableCommandBase<Cmd>::UsdUndoableCommandBase(const Ufe::Path& path)
    : Cmd(path)
#else
inline UsdUndoableCommandBase<Cmd>::UsdUndoableCommandBase(const UsdSceneItem::Ptr& item)
    : Cmd(item)
#endif
{
}

template <typename Cmd> inline void UsdUndoableCommandBase<Cmd>::execute()
{
    // Note: see the notes on the State enum why we're redoing instead
    //       of executing when in the undone state.
    if (State::Undone == _state)
        return redo();

    // Note: set new state before call in case setting the value causes feedback
    //       that end-up calling this again.
    const State prevState = _state;
    _state = State::Done;

    UsdUndoBlock undoBlock(&_undoableItem);
    executeImpl(prevState, _state);
}
template <typename Cmd> inline void UsdUndoableCommandBase<Cmd>::undo()
{
    // Note: protect against early undo before execute() has been called.
    if (State::Done != _state)
        return;

    // Note: set new state before call in case setting the value causes feedback
    //       that end-up calling this again.
    _state = State::Undone;

    _undoableItem.undo();
}

template <typename Cmd> inline void UsdUndoableCommandBase<Cmd>::redo()
{
    // Note: protect against early redo before execute() has been called.
    if (State::Undone != _state)
        return;

    // Note: set new state before call in case setting the value causes feedback
    //       that end-up calling this again.
    _state = State::Done;

    _undoableItem.redo();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
