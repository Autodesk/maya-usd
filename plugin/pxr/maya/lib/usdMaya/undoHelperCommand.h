//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_UNDO_HELPER_CMD_H
#define PXRUSDMAYA_UNDO_HELPER_CMD_H

/// \file usdMaya/undoHelperCommand.h

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include "usdMaya/api.h"
#include "usdMaya/adaptor.h"

#include <maya/MDGModifier.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>

PXR_NAMESPACE_OPEN_SCOPE

/// This is an internal helper command to provide undo support for operations
/// performed through the OpenMaya API. Use the ExecuteWithUndo() functions to
/// run functions that take an MDGModifier; the command will use the MDGModifier
/// for future undo and redo. Do not run the command directly (it will fail and
/// do nothing in that case).
class UsdMayaUndoHelperCommand : public MPxCommand
{
public:
    template <typename T>
    using UndoableResultFunction = std::function<T (MDGModifier&)>;
    using UndoableFunction = UndoableResultFunction<void>;

    PXRUSDMAYA_API
    UsdMayaUndoHelperCommand();
    PXRUSDMAYA_API
    ~UsdMayaUndoHelperCommand() override;

    PXRUSDMAYA_API
    MStatus doIt(const MArgList& args) override;
    PXRUSDMAYA_API
    MStatus redoIt() override;
    PXRUSDMAYA_API
    MStatus undoIt() override;
    PXRUSDMAYA_API
    bool isUndoable() const override;

    PXRUSDMAYA_API
    static MSyntax createSyntax();
    PXRUSDMAYA_API
    static void* creator();

    /// Calls \p func with an MDGModifier, saving the modifier for future undo
    /// and redo operations. If the \c usdUndoHelperCmd is unavailable, runs
    /// \p func directly without undo support and issues a warning. If \p func
    /// raises any Tf errors when it is called, it will not be added to Maya's
    /// undo stack.
    PXRUSDMAYA_API
    static void ExecuteWithUndo(const UndoableFunction& func);

    /// This overload of ExecuteWithUndo() supports a \p func that returns a
    /// value of type \p T.
    template <typename T>
    static T ExecuteWithUndo(const UndoableResultFunction<T>& func)
    {
        T result;
        ExecuteWithUndo([&result, &func](MDGModifier& modifier) {
            result = func(modifier);
        });
        return result;
    }

private:
    MDGModifier _modifier;
    bool _undoable;

    static const UndoableFunction* _dgModifierFunc;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
