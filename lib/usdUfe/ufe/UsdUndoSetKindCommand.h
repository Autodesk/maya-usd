//
// Copyright 2021 Autodesk
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
#ifndef USDUFE_UNDO_SET_KIND_COMMAND_H
#define USDUFE_UNDO_SET_KIND_COMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

#include <memory>

namespace USDUFE_NS_DEF {

//! Undoable command for setting the kind metadata of a UsdPrim.
class USDUFE_PUBLIC UsdUndoSetKindCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdUndoSetKindCommand>;

    UsdUndoSetKindCommand(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& kind);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoSetKindCommand);

    //! Create a UsdUndoSetKindCommand object
    static UsdUndoSetKindCommand::Ptr
    create(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& kind);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "SetKind"; })

private:
    PXR_NS::UsdPrim _prim;

    PXR_NS::TfToken _kind;

    UsdUndoableItem _undoableItem;

}; // UsdUndoSetKindCommand

} // namespace USDUFE_NS_DEF

#endif
