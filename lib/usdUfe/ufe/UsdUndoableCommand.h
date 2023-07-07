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

#include <usdUfe/base/api.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <ufe/path.h>

#include <functional>

namespace USDUFE_NS_DEF {

// Class that captures USD changes using a UsdUndoableItem.
//
// This class purpose is to handle the capture of USD data changes
// and undo and redo them. It is not meant to be used directly,
// but via the template class below. It avoids having every
// instantiations of the template have a copy of the same code
// and simplifies debugging by having a central point for all
// USD commands.
//
//
// Sub-classes only need to implement the executeImplementation() function.
// This function does the real work of modifying values, and these changes
// will be captured in the UsdUndoableItem via the UsdUndoBlock declared
// in execute().
class USDUFE_PUBLIC UsdUndoCapture
{
public:
    UsdUndoCapture();
    ~UsdUndoCapture();

protected:
    // This is the function sub-classes need to implement as their
    // command execution. It is called with the necessary setup to
    // capture all changes made in USD by using a UsdUndoableItem.
    virtual void executeImplementation() = 0;

    // This is the optional function sub-classes need to implement as their
    // command set. It is called with the necessary setup to
    // capture all changes made in USD by using a UsdUndoableItem.
    //
    // By default, calls executeImplenentation().
    virtual bool setImplementation();

    // Calls executeImplementation with the UsdUndoableItem
    // already setup. Should be called in a UFE command's execute().
    void executeWithUndoCapture();

    // Calls setImplementation with the UsdUndoableItem
    // already setup. Should be called in a UFE command's set().
    bool setWithUndoCapture();

    // Undo all USD changes captured during executeImplementation().
    // Should be called in a UFE command's undo().
    void undoUsdChanges();

    // Redo all USD changes captured during executeImplementation().
    // Should be called in a UFE command's redo().
    void redoUsdChanges();

private:
    UsdUndoableItem _undoableItem;
};

// Templated helper class to factor out common code for USD undoable commands.
//
// Sub-classes only need to implement the executeImplementation() function.
// This function does the real work of modifying values, and these changes
// will be captured in the UsdUndoableItem via the UsdUndoBlock declared
// in execute().

// This version wraps Ufe::UndoableCommand and derived classes.
template <typename Cmd>
class UsdUndoableCommand
    : public UsdUndoCapture
    , public Cmd
{
public:
    // This constructor allows passing arguments to the command bae class.
    // The magic of templated function will elide this if not used.
    template <class... ARGS>
    UsdUndoableCommand(const ARGS&... values)
        : Cmd(values...)
    {
    }

    // Ufe::UndoableCommand overrides.
    // Implemented by the UsdUndoCapture base class.

    void execute() override { executeWithUndoCapture(); }
    void undo() override { undoUsdChanges(); }
    void redo() override { redoUsdChanges(); }
};

// Templated helper class for USD implementations of UFE commands
// where the implementation is in a function.
//
// This avoids having to write a whole class just to implement the
// single executeImplementation() virtual function.
template <typename Cmd> class UsdFunctionUndoableCommand : public UsdUndoableCommand<Cmd>
{
public:
    // The function signature that implements the command.
    using Function = std::function<void()>;

    // This constructor allows passing arguments to the command bae class.
    // The magic of templated function will elide this if not used.
    template <class... ARGS>
    UsdFunctionUndoableCommand(const ARGS&... values, Function&& func)
        : UsdUndoableCommand<Cmd>(values...)
        , _func(func)
    {
    }

    // Implementation of UsdUndoCapture API.

    void executeImplementation() override { _func(); }

private:
    Function _func;
};

// Templated helper class to factor out common code for USD undoable commands.
//
// Sub-classes only need to implement the executeImplementation() function.
// This function does the real work of modifying values, and these changes
// will be captured in the UsdUndoableItem via the UsdUndoBlock declared
// in execute().

// This version wraps Ufe::UndoableCommand and derived classes.
template <typename Cmd>
class UsdUndoableSetCommand
    : public UsdUndoCapture
    , public Cmd
{
public:
    using ValueType = typename Cmd::ValueType;

    // This constructor allows passing arguments to the command bae class.
    // The magic of templated function will elide this if not used.
    template <class... ARGS>
    UsdUndoableSetCommand(const ARGS&... values)
        : Cmd(values...)
    {
    }

    // Ufe::UndoableCommand overrides.
    // Implemented by the UsdUndoCapture base class.

    void execute() override { executeWithUndoCapture(); }
    bool set(ValueType value) override
    {
        _value = value;
        return setWithUndoCapture();
    }
    void undo() override { undoUsdChanges(); }
    void redo() override { redoUsdChanges(); }

private:
    ValueType _value {};
};

// Templated helper class for USD implementations of UFE commands
// where the implementation is in a function.
//
// This avoids having to write a whole class just to implement the
// single executeImplementation() virtual function.
template <typename Cmd> class UsdFunctionUndoableSetCommand : public UsdUndoableSetCommand<Cmd>
{
public:
    // The function signature that implements the command.
    using Function = std::function<bool()>;

    // This constructor allows passing arguments to the command bae class.
    // The magic of templated function will elide this if not used.
    template <class... ARGS>
    UsdFunctionUndoableSetCommand(Function&& func, const ARGS&... values)
        : UsdUndoableSetCommand<Cmd>(values...)
        , _func(func)
    {
    }

    // Implementations of UsdUndoCapture API.

    void executeImplementation() override { _func(); }
    bool setImplementation() override { return _func(); }

private:
    Function _func;
};

} // namespace USDUFE_NS_DEF
