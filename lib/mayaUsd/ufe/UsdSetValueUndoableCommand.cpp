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

#include "UsdSetValueUndoableCommand.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

////////////////////////////////////////////////////////////////////////////
//
// Declare the internal state classes, but don't implement them
// because their functions need to refer to each others.

template <typename Cmd> struct UsdSetValueUndoableCmdBase<Cmd>::State
{
    using CmdBase = typename UsdSetValueUndoableCmdBase<Cmd>;

    virtual const char* name() const = 0;
    virtual void        handleUndo(CmdBase*) const;
    virtual void        handleSet(CmdBase*, const VtValue&) const;
};

template <typename Cmd>
struct UsdSetValueUndoableCmdBase<Cmd>::InitialState : public UsdSetValueUndoableCmdBase<Cmd>::State
{
    using CmdBase = typename UsdSetValueUndoableCmdBase<Cmd>;

    const char* name() const override { return "initial"; }
    void        handleUndo(CmdBase* cmd) const override;
    void        handleSet(CmdBase* cmd, const VtValue& v) const override;

    static const InitialState* get();
};

template <typename Cmd>
struct UsdSetValueUndoableCmdBase<Cmd>::InitialUndoCalledState
    : public UsdSetValueUndoableCmdBase<Cmd>::State
{
    using CmdBase = typename UsdSetValueUndoableCmdBase<Cmd>;

    const char* name() const override { return "initial undo called"; }
    void        handleSet(CmdBase* cmd, const VtValue&) const override;

    static const InitialUndoCalledState* get();
};

template <typename Cmd>
struct UsdSetValueUndoableCmdBase<Cmd>::ExecuteState : public UsdSetValueUndoableCmdBase<Cmd>::State
{
    using CmdBase = typename UsdSetValueUndoableCmdBase<Cmd>;

    const char* name() const override { return "execute"; }
    void        handleUndo(CmdBase* cmd) const override;
    void        handleSet(CmdBase* cmd, const VtValue& v) const override;

    static const ExecuteState* get();
};

template <typename Cmd>
struct UsdSetValueUndoableCmdBase<Cmd>::UndoneState : public UsdSetValueUndoableCmdBase<Cmd>::State
{
    using CmdBase = typename UsdSetValueUndoableCmdBase<Cmd>;

    const char* name() const override { return "undone"; }
    void        handleSet(CmdBase* cmd, const VtValue&) const override;

    static const UndoneState* get();
};

template <typename Cmd>
struct UsdSetValueUndoableCmdBase<Cmd>::RedoneState : public UsdSetValueUndoableCmdBase<Cmd>::State
{
    using CmdBase = typename UsdSetValueUndoableCmdBase<Cmd>;

    const char* name() const override { return "redone"; }
    void        handleUndo(CmdBase* cmd) const override;

    static const RedoneState* get();
};

////////////////////////////////////////////////////////////////////////////
//
// Implement each state's static instance getter.
//
// The reason to have these static instances in functions is that
// the C++ standard gives guaranteed about the initialization order
// of static variable declared in functions, unlike global statics.
//
// We are guaranteed that the static variable will be initialized
// when the function is called.
//
// Plus, it is simpler for templates.

template <typename Cmd>
const typename UsdSetValueUndoableCmdBase<Cmd>::InitialState*
UsdSetValueUndoableCmdBase<Cmd>::InitialState::get()
{
    static const InitialState _instance;
    return &_instance;
}

template <typename Cmd>
const typename UsdSetValueUndoableCmdBase<Cmd>::InitialUndoCalledState*
UsdSetValueUndoableCmdBase<Cmd>::InitialUndoCalledState::get()
{
    static const InitialUndoCalledState _instance;
    return &_instance;
}

template <typename Cmd>
const typename UsdSetValueUndoableCmdBase<Cmd>::ExecuteState*
UsdSetValueUndoableCmdBase<Cmd>::ExecuteState::get()
{
    static const ExecuteState _instance;
    return &_instance;
}

template <typename Cmd>
const typename UsdSetValueUndoableCmdBase<Cmd>::UndoneState*
UsdSetValueUndoableCmdBase<Cmd>::UndoneState::get()
{
    static const UndoneState _instance;
    return &_instance;
}

template <typename Cmd>
const typename UsdSetValueUndoableCmdBase<Cmd>::RedoneState*
UsdSetValueUndoableCmdBase<Cmd>::RedoneState::get()
{
    static const RedoneState _instance;
    return &_instance;
}


////////////////////////////////////////////////////////////////////////////
//
// Implement each state functions.

template <typename Cmd> void UsdSetValueUndoableCmdBase<Cmd>::State::handleUndo(CmdBase*) const
{
    TF_CODING_ERROR(
        "Illegal handleUndo() call in UsdSetValueUndoableCmdBase for state '%s'.", name());
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::State::handleSet(CmdBase*, const VtValue&) const
{
    TF_CODING_ERROR(
        "Illegal handleSet() call in UsdSetValueUndoableCmdBase for state '%s'.", name());
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::InitialState::handleUndo(CmdBase* cmd) const
{
    // Maya triggers an undo on command creation, ignore it.
    cmd->_state = CmdBase::InitialUndoCalledState::get();
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::InitialState::handleSet(CmdBase* cmd, const VtValue& v) const
{
    // Add undoblock to capture edits
    UsdUndoBlock undoBlock(&cmd->_undoableItem);

    // Going from initial to executing / executed state, save value.
    cmd->_op = cmd->_opFunc(*cmd);
    cmd->_newOpValue = v;
    cmd->setValue(v);
    cmd->_state = CmdBase::ExecuteState::get();
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::InitialUndoCalledState::handleSet(
    CmdBase* cmd,
    const VtValue&) const
{
    // Maya triggers a redo on command creation, ignore it.
    cmd->_state = CmdBase::InitialState::get();
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::ExecuteState::handleUndo(CmdBase* cmd) const
{
    // Undo
    cmd->_undoableItem.undo();

    cmd->_state = UsdSetValueUndoableCmdBase::UndoneState::get();
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::ExecuteState::handleSet(CmdBase* cmd, const VtValue& v) const
{
    cmd->_newOpValue = v;
    cmd->setValue(v);
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::UndoneState::handleSet(CmdBase* cmd, const VtValue&) const
{
    // Redo
    cmd->_undoableItem.redo();

    cmd->_state = CmdBase::RedoneState::get();
}

template <typename Cmd>
void UsdSetValueUndoableCmdBase<Cmd>::RedoneState::handleUndo(CmdBase* cmd) const
{
    // Undo
    cmd->_undoableItem.undo();

    cmd->_state = CmdBase::UndoneState::get();
}


////////////////////////////////////////////////////////////////////////////
//
// Implementation of the command base class.

template <typename Cmd>
UsdSetValueUndoableCmdBase<Cmd>::UsdSetValueUndoableCmdBase(
    const VtValue&     newOpValue,
    const Ufe::Path&   path,
    OpFunc             opFunc,
    const UsdTimeCode& writeTime_)
    : Cmd(path)
    // Always read from proxy shape time.
    , _state(InitialState::get())
    , _readTime(getTime(path))
    , _writeTime(writeTime_)
    , _newOpValue(newOpValue)
    , _op()
    , _opFunc(std::move(opFunc))
{
}

template <typename Cmd> void UsdSetValueUndoableCmdBase<Cmd>::execute()
{
    handleSet(_newOpValue);
}

template <typename Cmd> void UsdSetValueUndoableCmdBase<Cmd>::undo()
{
    _state->handleUndo(this);
}

template <typename Cmd> void UsdSetValueUndoableCmdBase<Cmd>::redo()
{
    handleSet(_newOpValue);
}

template <typename Cmd> void UsdSetValueUndoableCmdBase<Cmd>::handleSet(const VtValue& v)
{
    _state->handleSet(this, v);
}

template <typename Cmd> void UsdSetValueUndoableCmdBase<Cmd>::setValue(const VtValue& v)
{
    _op.GetAttr().Set(v, _writeTime);
}

template <typename Cmd> UsdTimeCode UsdSetValueUndoableCmdBase<Cmd>::readTime() const
{
    return _readTime;
}
template <typename Cmd> UsdTimeCode UsdSetValueUndoableCmdBase<Cmd>::writeTime() const
{
    return _writeTime;
}

template class UsdSetValueUndoableCmdBase<Ufe::SetVector3dUndoableCommand>;

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
