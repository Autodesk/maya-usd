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
#ifndef USDUFE_USDUNDOCLEARDEFAULTPRIMCOMMAND_H
#define USDUFE_USDUNDOCLEARDEFAULTPRIMCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
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
    UsdUndoClearDefaultPrimCommand(const PXR_NS::UsdStageRefPtr& stage);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoClearDefaultPrimCommand);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "ClearDefaultPrim"; })

private:
    PXR_NS::UsdStageRefPtr _stage;

    UsdUndoableItem _undoableItem;

}; // UsdUndoClearDefaultPrimCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDOCLEARDEFAULTPRIMCOMMAND_H
