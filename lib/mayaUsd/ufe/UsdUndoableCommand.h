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

#include <pxr/base/vt/value.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <ufe/baseUndoableCommands.h>
#include <ufe/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Templated helper class to factor out common code for setting USD values.
//
// Implement the execute, undo and redo of the UFE command interfaces, with
// common code proetecting against early undo/redo preceeding the initial
// execution and declaring the UsdUndoBlock during the execution.
//
// Sub-classes should *not* have to override execute(), undo() nor redo().
//
// Sub-classes only need to implement the handleSet() function.
//
// Sub-classes can override set() if the command support interactive
// manipulations. Since each set() in specific to the specific Cmd base
// class, we cannot provide a generic implementation in this template.
//
// A typical set() implementation should call setNewValue() with the
// new value and then execute() to actually set the value on the USD
// attribute.
template <typename Cmd> class UsdUndoableCmdBase : public Cmd
{
public:
    UsdUndoableCmdBase(
        const VtValue&     newOpValue,
        const Ufe::Path&   path,
        const UsdTimeCode& writeTime_);

    // Ufe::UndoableCommand overrides. Do not override further.
    void execute() override;
    void undo() override;
    void redo() override;

    UsdTimeCode readTime() const;
    UsdTimeCode writeTime() const;

protected:
    // Update the new value that will be set by execute().
    void setNewValue(const VtValue& v);

    // State of the undo/redo.
    //
    // Note: unfortunately, we need to track the initial/done/undo
    //       state ourselves because, for some reason, UFE does *not*
    //       call redo() to redo, but instead calls set() again.
    //
    //       So, if we want to use the USD undo system, we have to
    //       track the state of the undo /redo ourselves so that when
    //       set() is called when we're in a undone state, we do redo()
    //       instead...
    enum class State
    {
        Initial,
        Done,
        Undone
    };

    // Overridable method for derived classes to actually set the value on
    // the USD attribute. The handleSet() call will be within a USD undo block
    // as necessary. You don't need to declare such a block.
    //
    // We provide the previous and new state in case they need to take special
    // actions on a given transition.
    virtual void handleSet(State previousState, State newState, const VtValue& v) = 0;

private:
    // Concrete implementation of execute, undo and redo.
    //
    // Execute creates an undo block with the undoable item and calls handleSet().
    // Undo calls undo on the undoable item.
    // Redo calls redo on the undoable item.
    void executeImpl();
    void undoImpl();
    void redoImpl();

    State             _state = State::Initial;
    const UsdTimeCode _readTime;
    const UsdTimeCode _writeTime;
    VtValue           _newValue;
    UsdUndoableItem   _undoableItem;
};

// Implementation of the command base class.

template <typename Cmd>
inline UsdUndoableCmdBase<Cmd>::UsdUndoableCmdBase(
    const VtValue&     newOpValue,
    const Ufe::Path&   path,
    const UsdTimeCode& writeTime_)
    : Cmd(path)
    // Always read from proxy shape time.
    , _readTime(getTime(path))
    , _writeTime(writeTime_)
    , _newValue(newOpValue)
{
}

template <typename Cmd> inline void UsdUndoableCmdBase<Cmd>::execute()
{
    // Note: set the notes on the State enum why we're redoing instead
    //       of executing when in the undone state.
    if (State::Undone == _state)
        return redoImpl();

    executeImpl();
}
template <typename Cmd> inline void UsdUndoableCmdBase<Cmd>::undo()
{
    // Note: protect against early undo before execute() has been called.
    if (State::Done != _state)
        return;

    undoImpl();
}

template <typename Cmd> inline void UsdUndoableCmdBase<Cmd>::redo()
{
    // Note: protect against early redo before execute() has been called.
    if (State::Undone != _state)
        return;

    redoImpl();
}

template <typename Cmd> inline void UsdUndoableCmdBase<Cmd>::executeImpl()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Note: set new state before call in case setting the value causes feedback
    //       that end-up calling this again.
    const State prevState = _state;
    _state = State::Done;
    handleSet(prevState, _state, _newValue);
}

template <typename Cmd> inline void UsdUndoableCmdBase<Cmd>::undoImpl()
{
    // Note: set new state before call in case setting the value causes feedback
    //       that end-up calling this again.
    _state = State::Undone;
    _undoableItem.undo();
}

template <typename Cmd> inline void UsdUndoableCmdBase<Cmd>::redoImpl()
{
    // Note: set new state before call in case setting the value causes feedback
    //       that end-up calling this again.
    _state = State::Done;
    _undoableItem.redo();
}

template <typename Cmd> inline void UsdUndoableCmdBase<Cmd>::setNewValue(const VtValue& v)
{
    _newValue = v;
}

template <typename Cmd> inline UsdTimeCode UsdUndoableCmdBase<Cmd>::readTime() const
{
    return _readTime;
}

template <typename Cmd> inline UsdTimeCode UsdUndoableCmdBase<Cmd>::writeTime() const
{
    return _writeTime;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
