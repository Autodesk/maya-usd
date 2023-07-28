//
// Copyright 2022 Autodesk
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

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/variantSets.h>

#include <ufe/path.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>

#include <string>

namespace USDUFE_NS_DEF {

//! \brief Command to change a variant selection.
class USDUFE_PUBLIC SetVariantSelectionCommand : public Ufe::UndoableCommand
{
public:
    using Ptr = std::shared_ptr<SetVariantSelectionCommand>;

    //! Create a SetVariantSelectionCommand. Does not execute it.
    static SetVariantSelectionCommand::Ptr create(
        const Ufe::Path&       path,
        const PXR_NS::UsdPrim& prim,
        const std::string&     variantName,
        const std::string&     variantSelection);

    SetVariantSelectionCommand(
        const Ufe::Path&       path,
        const PXR_NS::UsdPrim& prim,
        const std::string&     variantName,
        const std::string&     variantSelection);

    ~SetVariantSelectionCommand() override;

    void redo() override;
    void undo() override;

private:
    // Delete the copy/move constructors assignment operators.
    SetVariantSelectionCommand(const SetVariantSelectionCommand&) = delete;
    SetVariantSelectionCommand& operator=(const SetVariantSelectionCommand&) = delete;
    SetVariantSelectionCommand(SetVariantSelectionCommand&&) = delete;
    SetVariantSelectionCommand& operator=(SetVariantSelectionCommand&&) = delete;

    const Ufe::Path       _path;
    PXR_NS::UsdPrim       _prim;
    PXR_NS::UsdVariantSet _varSet;
    const std::string     _oldSelection;
    const std::string     _newSelection;
    Ufe::Selection        _savedSn; // For global selection save and restore.
};

} // namespace USDUFE_NS_DEF
