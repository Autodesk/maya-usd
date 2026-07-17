//
// Copyright 2026 Autodesk
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
#ifndef USD_UFE_ADD_REF_OR_PAYLOAD_TO_NEW_PRIM_COMMAND
#define USD_UFE_ADD_REF_OR_PAYLOAD_TO_NEW_PRIM_COMMAND

#include <usdUfe/base/api.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

#include <memory>
#include <string>

namespace USDUFE_NS_DEF {

//! \brief Command that creates a new child "def" prim and adds a reference or payload arc to
//! it, as a single undoable operation. The new prim's type is composed through the arc, so it
//! takes on the referenced default prim's type.
//!
//! The work is orchestrated as a composite of the existing UsdUndoAddNewPrimCommand and the
//! UsdUndoAddReferenceCommand / UsdUndoAddPayloadCommand (plus load/unload) commands.
class USDUFE_PUBLIC UsdUndoAddRefOrPayloadToNewPrimCommand : public Ufe::UndoableCommand
{
public:
    UsdUndoAddRefOrPayloadToNewPrimCommand(
        const PXR_NS::UsdPrim& parentPrim,
        const std::string&     newPrimName,
        const std::string&     filePath,
        const std::string&     primPath,
        bool                   prepend,
        bool                   isPayload = false,
        bool                   preload   = false);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoAddRefOrPayloadToNewPrimCommand);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    PXR_NS::UsdPrim _parentPrim;
    std::string     _newPrimName;
    std::string     _filePath;
    std::string     _primPath;
    bool            _prepend;
    bool            _isPayload;
    bool            _preload;

    std::shared_ptr<Ufe::CompositeUndoableCommand> _compositeCmd;
};

} // namespace USDUFE_NS_DEF

#endif /* USD_UFE_ADD_REF_OR_PAYLOAD_TO_NEW_PRIM_COMMAND */
