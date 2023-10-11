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

//! \brief UsdUndoClearDefaultPrimCommand
class USDUFE_PUBLIC UsdUndoClearDefaultPrimCommand : public Ufe::UndoableCommand
{
public:
    // Public for std::make_shared() access, use create() instead.
    UsdUndoClearDefaultPrimCommand(const PXR_NS::UsdPrim& prim);
    ~UsdUndoClearDefaultPrimCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoClearDefaultPrimCommand(const UsdUndoClearDefaultPrimCommand&) = delete;
    UsdUndoClearDefaultPrimCommand& operator=(const UsdUndoClearDefaultPrimCommand&) = delete;
    UsdUndoClearDefaultPrimCommand(UsdUndoClearDefaultPrimCommand&&) = delete;
    UsdUndoClearDefaultPrimCommand& operator=(UsdUndoClearDefaultPrimCommand&&) = delete;

private:
    void execute() override;
    void undo() override;
    void redo() override;

    PXR_NS::UsdPrim         _prim;
    PXR_NS::UsdStageWeakPtr _stage;

    UsdUndoableItem _undoableItem;

}; // UsdUndoClearDefaultPrimCommand

} // namespace USDUFE_NS_DEF
