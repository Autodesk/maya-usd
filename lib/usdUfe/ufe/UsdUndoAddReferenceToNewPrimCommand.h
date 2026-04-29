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
#ifndef USD_UFE_ADD_REFERENCE_TO_NEW_PRIM_COMMAND
#define USD_UFE_ADD_REFERENCE_TO_NEW_PRIM_COMMAND

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>

#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

#include <string>

namespace USDUFE_NS_DEF {

//! \brief Command that creates a new child prim and adds a reference (or payload) arc to
//! it in a single atomic undo/redo operation.
class USDUFE_PUBLIC UsdUndoAddReferenceToNewPrimCommand
    : public UsdUndoableCommand<Ufe::UndoableCommand>
{
public:
    UsdUndoAddReferenceToNewPrimCommand(
        const PXR_NS::UsdPrim& parentPrim,
        const std::string&     newPrimName,
        const std::string&     newPrimType,
        const std::string&     filePath,
        const std::string&     primPath,
        bool                   prepend,
        bool                   isPayload = false,
        bool                   preload   = false);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoAddReferenceToNewPrimCommand);

protected:
    void executeImplementation() override;

private:
    static PXR_NS::UsdListPosition getListPosition(bool prepend);

    PXR_NS::UsdPrim _parentPrim;
    std::string     _newPrimName;
    std::string     _newPrimType;
    std::string     _filePath;
    std::string     _primPath;
    bool            _prepend;
    bool            _isPayload;
    bool            _preload;
};

} // namespace USDUFE_NS_DEF

#endif /* USD_UFE_ADD_REFERENCE_TO_NEW_PRIM_COMMAND */
