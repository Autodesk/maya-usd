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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoVisibleCommand
class MAYAUSD_CORE_PUBLIC UsdUndoVisibleCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoVisibleCommand> Ptr;

    UsdUndoVisibleCommand(const PXR_NS::UsdPrim& prim, bool vis);
    ~UsdUndoVisibleCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoVisibleCommand(const UsdUndoVisibleCommand&) = delete;
    UsdUndoVisibleCommand& operator=(const UsdUndoVisibleCommand&) = delete;
    UsdUndoVisibleCommand(UsdUndoVisibleCommand&&) = delete;
    UsdUndoVisibleCommand& operator=(UsdUndoVisibleCommand&&) = delete;

    //! Create a UsdUndoVisibleCommand object
    static UsdUndoVisibleCommand::Ptr create(const PXR_NS::UsdPrim& prim, bool vis);

private:
    void execute() override;
    void undo() override;
    void redo() override;

    PXR_NS::UsdPrim _prim;

    bool _visible;

    UsdUndoableItem _undoableItem;

}; // UsdUndoVisibleCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
