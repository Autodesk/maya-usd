//
// Copyright 2023 Autodesk
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
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoSetDefaultPrimCommand
class USDUFE_PUBLIC UsdUndoSetDefaultPrimCommand : public Ufe::UndoableCommand
{
public:
    // Public for std::make_shared() access, use create() instead.
    UsdUndoSetDefaultPrimCommand(const PXR_NS::UsdPrim& prim);
    ~UsdUndoSetDefaultPrimCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoSetDefaultPrimCommand(const UsdUndoSetDefaultPrimCommand&) = delete;
    UsdUndoSetDefaultPrimCommand& operator=(const UsdUndoSetDefaultPrimCommand&) = delete;
    UsdUndoSetDefaultPrimCommand(UsdUndoSetDefaultPrimCommand&&) = delete;
    UsdUndoSetDefaultPrimCommand& operator=(UsdUndoSetDefaultPrimCommand&&) = delete;

    void execute() override;
    void undo() override;
    void redo() override;

private:
    PXR_NS::UsdPrim _prim;

    UsdUndoableItem _undoableItem;

}; // UsdUndoSetDefaultPrimCommand

} // namespace USDUFE_NS_DEF
