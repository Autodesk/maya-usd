//
// Copyright 2020 Autodesk
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
#ifndef USDUFE_USDUNDOREORDERCOMMAND_H
#define USDUFE_USDUNDOREORDERCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

namespace USDUFE_NS_DEF {

//! \brief UsdUndoReorderCommand
class USDUFE_PUBLIC UsdUndoReorderCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoReorderCommand> Ptr;

    UsdUndoReorderCommand(
        const PXR_NS::UsdPrim&              parentPrim,
        const std::vector<PXR_NS::TfToken>& orderedTokens);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoReorderCommand);

    //! Create a UsdUndoReorderCommand
    static UsdUndoReorderCommand::Ptr
    create(const PXR_NS::UsdPrim& parentPrim, const std::vector<PXR_NS::TfToken>& orderedTokens);

private:
    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "Reorder"; })

    PXR_NS::UsdPrim              _parentPrim;
    std::vector<PXR_NS::TfToken> _orderedTokens;
    UsdUndoableItem              _undoableItem;

}; // UsdUndoReorderCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDOREORDERCOMMAND_H
