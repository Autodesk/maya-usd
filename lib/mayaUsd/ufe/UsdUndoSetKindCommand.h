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
#ifndef MAYAUSD_UFE_USD_UNDO_SET_KIND_COMMAND_H
#define MAYAUSD_UFE_USD_UNDO_SET_KIND_COMMAND_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

#include <memory>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! Undoable command for setting the kind metadata of a UsdPrim.
class MAYAUSD_CORE_PUBLIC UsdUndoSetKindCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<UsdUndoSetKindCommand>;

    UsdUndoSetKindCommand(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& kind);
    ~UsdUndoSetKindCommand() override = default;

    UsdUndoSetKindCommand(const UsdUndoSetKindCommand&) = delete;
    UsdUndoSetKindCommand& operator=(const UsdUndoSetKindCommand&) = delete;
    UsdUndoSetKindCommand(UsdUndoSetKindCommand&&) = delete;
    UsdUndoSetKindCommand& operator=(UsdUndoSetKindCommand&&) = delete;

    //! Create a UsdUndoSetKindCommand object
    static UsdUndoSetKindCommand::Ptr
    create(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& kind);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdPrim _prim;

    TfToken _kind;

    UsdUndoableItem _undoableItem;

}; // UsdUndoSetKindCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif
