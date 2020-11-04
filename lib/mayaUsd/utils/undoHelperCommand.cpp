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
#include "undoHelperCommand.h"

#include <pxr/base/tf/errorMark.h>

#include <maya/MSyntax.h>

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

namespace {
#define CMD_NAME "usdUndoHelperCmd"

int _registrationCount = 0;

// Name of the plugin registering the command.
MString _registrantPluginName;
} // namespace

PXR_NAMESPACE_OPEN_SCOPE

const UsdMayaUndoHelperCommand::UndoableFunction* UsdMayaUndoHelperCommand::_dgModifierFunc
    = nullptr;

/* static */
MStatus UsdMayaUndoHelperCommand::initialize(MFnPlugin& plugin)
{
    if (_registrationCount < 0) {
        MGlobal::displayError("Illegal initialization of " CMD_NAME);
        return MS::kFailure;
    }

    // If we're already registered, do nothing.
    if (_registrationCount++ > 0) {
        return MS::kSuccess;
    }

    _registrantPluginName = plugin.name();

    return plugin.registerCommand(
        CMD_NAME, UsdMayaUndoHelperCommand::creator, UsdMayaUndoHelperCommand::createSyntax);
}

/* static */
MStatus UsdMayaUndoHelperCommand::finalize(MFnPlugin& plugin)
{
    if (_registrationCount <= 0) {
        MGlobal::displayError("Illegal finalization of " CMD_NAME);
        return MS::kFailure;
    }

    // If more than one plugin still has us registered, do nothing.
    if (_registrationCount-- > 1) {
        return MS::kSuccess;
    }

    // Maya requires deregistration to be done by the same plugin that
    // performed the registration.  If this isn't possible, warn and don't
    // deregister.
    if (plugin.name() != _registrantPluginName) {
        MGlobal::displayWarning(
            CMD_NAME " cannot be deregistered, registering plugin " + _registrantPluginName
            + " is unloaded.");
        return MS::kSuccess;
    }

    return plugin.deregisterCommand(CMD_NAME);
}

/* static */
const char* UsdMayaUndoHelperCommand::name() { return CMD_NAME; }

UsdMayaUndoHelperCommand::UsdMayaUndoHelperCommand()
    : _undoable(false)
{
}

UsdMayaUndoHelperCommand::~UsdMayaUndoHelperCommand() { }

MStatus UsdMayaUndoHelperCommand::doIt(const MArgList& /*args*/)
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

MStatus UsdMayaUndoHelperCommand::redoIt() { return _modifier.doIt(); }

MStatus UsdMayaUndoHelperCommand::undoIt() { return _modifier.undoIt(); }

bool UsdMayaUndoHelperCommand::isUndoable() const { return _undoable; };

MSyntax UsdMayaUndoHelperCommand::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

void* UsdMayaUndoHelperCommand::creator() { return new UsdMayaUndoHelperCommand(); }

/* static */
void UsdMayaUndoHelperCommand::ExecuteWithUndo(const UndoableFunction& func)
{
    int cmdExists = 0;
    MGlobal::executeCommand("exists " CMD_NAME, cmdExists);
    if (!cmdExists) {
        TF_WARN(CMD_NAME " is unavailable; "
                         "function will run without undo support");
        MDGModifier modifier;
        func(modifier);
        return;
    }

    // Run function through command if it is available to get undo support!
    _dgModifierFunc = &func;
    MGlobal::executeCommand(CMD_NAME, false, true);
}

PXR_NAMESPACE_CLOSE_SCOPE
