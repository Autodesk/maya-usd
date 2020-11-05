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

#include <pxr/usd/usd/prim.h>

#include <ufe/undoableCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoReorderCommand
class MAYAUSD_CORE_PUBLIC UsdUndoReorderCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoReorderCommand> Ptr;

    UsdUndoReorderCommand(const UsdPrim& parentPrim, const std::vector<TfToken>& orderedTokens);
    ~UsdUndoReorderCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoReorderCommand(const UsdUndoReorderCommand&) = delete;
    UsdUndoReorderCommand& operator=(const UsdUndoReorderCommand&) = delete;
    UsdUndoReorderCommand(UsdUndoReorderCommand&&) = delete;
    UsdUndoReorderCommand& operator=(UsdUndoReorderCommand&&) = delete;

    //! Create a UsdUndoReorderCommand
    static UsdUndoReorderCommand::Ptr
    create(const UsdPrim& parentPrim, const std::vector<TfToken>& orderedTokens);

private:
    bool reorder();

    void undo() override;
    void redo() override;

    UsdPrim _parentPrim;

    std::vector<TfToken> _orderedTokens;

}; // UsdUndoReorderCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
