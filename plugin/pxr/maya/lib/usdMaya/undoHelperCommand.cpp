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
#include "usdMaya/undoHelperCommand.h"

#include "pxr/base/tf/errorMark.h"

#include <maya/MSyntax.h>

PXR_NAMESPACE_OPEN_SCOPE

const UsdMayaUndoHelperCommand::UndoableFunction*
UsdMayaUndoHelperCommand::_dgModifierFunc = nullptr;

UsdMayaUndoHelperCommand::UsdMayaUndoHelperCommand() : _undoable(false)
{
}

UsdMayaUndoHelperCommand::~UsdMayaUndoHelperCommand()
{
}

MStatus
UsdMayaUndoHelperCommand::doIt(const MArgList& /*args*/)
{
    if (!_dgModifierFunc) {
        _undoable = false;
        return MS::kFailure;
    }

    TfErrorMark errorMark;
    errorMark.SetMark();
    (*_dgModifierFunc)(_modifier);
    _dgModifierFunc = nullptr;
    _undoable = errorMark.IsClean();
    return MS::kSuccess;
}

MStatus
UsdMayaUndoHelperCommand::redoIt()
{
    return _modifier.doIt();
}

MStatus
UsdMayaUndoHelperCommand::undoIt()
{
    return _modifier.undoIt();
}

bool
UsdMayaUndoHelperCommand::isUndoable() const {
    return _undoable;
};

MSyntax
UsdMayaUndoHelperCommand::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

void*
UsdMayaUndoHelperCommand::creator()
{
    return new UsdMayaUndoHelperCommand();
}

/* static */
void
UsdMayaUndoHelperCommand::ExecuteWithUndo(const UndoableFunction& func)
{
    int cmdExists = 0;
    MGlobal::executeCommand("exists usdUndoHelperCmd", cmdExists);
    if (!cmdExists) {
        TF_WARN("usdUndoHelperCmd is unavailable; "
                "function will run without undo support");
        MDGModifier modifier;
        func(modifier);
        return;
    }

    // Run function through command if it is available to get undo support!
    _dgModifierFunc = &func;
    MGlobal::executeCommand("usdUndoHelperCmd", false, true);
}

PXR_NAMESPACE_CLOSE_SCOPE
