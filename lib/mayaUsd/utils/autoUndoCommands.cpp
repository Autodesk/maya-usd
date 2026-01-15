//
// Copyright 2024 Autodesk
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

#include "autoUndoCommands.h"

#include <maya/MGlobal.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

AutoUndoCommands::AutoUndoCommands(const char* scriptName, const std::string& commands)
    : _scriptName(scriptName)
{
    _executeCommands(commands);
}

AutoUndoCommands::~AutoUndoCommands()
{
    try {
        _undoCommands();
    } catch (const std::exception&) {
    }
}

void AutoUndoCommands::undo() { _undoCommands(); }

void AutoUndoCommands::disableUndo() { _needUndo = false; }

void AutoUndoCommands::_executeCommands(const std::string& commands)
{
    // If no commands were provided, we do nothing.
    // This allow sub-classes to cancel the execution if needed by providing
    // no commands.
    if (commands.empty()) {
        _success = true;
        return;
    }

    // Put all the requested commands inside a function to isolate any
    // variable they migh declare.
    static const char scriptPrefix[] = "proc _executeCommandsToBeUndone() {\n";

    // Wrap all commands in a undo block (undo chunk in Maya parlance).
    static const char scriptSuffix[]
        = "}\n"
          "proc int _executeCommandsWithUndo() {\n"
          // We need to re-enable undo for this because we may be executed
          // in a context that disbaled undo.
          "    int $undoWereActive = `undoInfo -query -state`;\n"
          "    undoInfo -stateWithoutFlush 1;\n"
          // Open the undo block. to make all commands undoable as a unit.
          "    undoInfo -openChunk;\n"
          // Execute the commands.
          "    if (catchQuiet(_executeCommandsToBeUndone())) {;\n"
          "        return 0;\n"
          "    }\n"
          // Close the undo bloack to make the whole process undoable as a unit.
          "    undoInfo -closeChunk;\n"
          // Restore the undo active flag.
          "    undoInfo -stateWithoutFlush $undoWereActive;\n"
          "    return 1;\n"
          "}\n"
          "_executeCommandsWithUndo();\n";

    std::string fullScript = std::string(scriptPrefix) + commands + std::string(scriptSuffix);

    const bool displayEnabled = false;
    const bool undoEnabled = true;

    int result = 0;
    if (!MGlobal::executeCommand(fullScript.c_str(), result, displayEnabled, undoEnabled)
        || result != 1) {
        _success = false;
        _needUndo = true;
        MString errMsg;
        errMsg.format("Failed to ^1s.", _scriptName.c_str());
        MGlobal::displayWarning(errMsg);
        return;
    }

    _success = true;
    _needUndo = true;
}

void AutoUndoCommands::_undoCommands()
{
    if (!_needUndo)
        return;

    // Make sure undo will not be done twice even if there are exceptions.
    _needUndo = false;

    static const char fullScript[] = "proc _undoCommands() {\n"
                                     // We need to re-enable undo for this because we may be
                                     // executed in a context that disbaled undo.
                                     "    int $undoWereActive = `undoInfo -query -state`;\n"
                                     "    undoInfo -stateWithoutFlush 1;\n"
                                     // Undo the commands.
                                     "    undo;\n"
                                     // Restore the undo active flag.
                                     "    undoInfo -stateWithoutFlush $undoWereActive;\n"
                                     "}\n"
                                     "_undoCommands();\n";

    const bool displayEnabled = false;
    const bool undoEnabled = true;

    if (!MGlobal::executeCommand(fullScript, displayEnabled, undoEnabled)) {
        MString errMsg;
        errMsg.format("Failed to undo ^1s.", _scriptName.c_str());
        MGlobal::displayWarning(errMsg);
    }
}
} // namespace MAYAUSD_NS_DEF