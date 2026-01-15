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
//

#ifndef MAYAUSD_AUTOUNDOCOMMANDS_H
#define MAYAUSD_AUTOUNDOCOMMANDS_H

#include <mayaUsd/base/api.h>

#include <string>

namespace MAYAUSD_NS_DEF {

/// Automatically undo a group of MEL script commands when destroyed or cleaned-up.
///
/// Sub-classes provide the undoable MEL script to execute and which will be undone.
/// Users can call cleanup() to do the cleanup early.
class MAYAUSD_CORE_PUBLIC AutoUndoCommands
{
public:
    /// Execute the given commands in an undo block.
    ///
    /// If no commands are provided, we do nothing.
    /// This allow sub-classes to cancel the execution if needed by providing
    /// no commands.
    AutoUndoCommands(const char* scriptName, const std::string& commands);

    /// Undo the executed command if undo has not been disabled.
    ~AutoUndoCommands();

    /// Undo immediately if not already done and not disabled.
    void undo();

    /// Disable undo of the commands.
    /// Can be used if undo is no longer deemed necessary.
    void disableUndo();

    /// Verify ifthe command executed successfully.
    bool commandExecutedSuccessfully() const { return _success; }

private:
    /// Execute the given commands in an undo block.
    ///
    /// If no commands are provided, we do nothing.
    /// This allow sub-classes to cancel the execution if needed by providing
    /// no commands.
    void _executeCommands(const std::string& commands);

    /// Undo the executed command if undo has not been disabled.
    void _undoCommands();

    std::string _scriptName;
    bool        _needUndo { false };
    bool        _success { false };
};

} // namespace MAYAUSD_NS_DEF

#endif
