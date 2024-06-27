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
#include "SetVariantSelectionCommand.h"

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/usd/usd/variantSets.h>

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, SetVariantSelectionCommand);

SetVariantSelectionCommand::Ptr SetVariantSelectionCommand::create(
    const Ufe::Path&       path,
    const PXR_NS::UsdPrim& prim,
    const std::string&     variantName,
    const std::string&     variantSelection)
{
    return std::make_shared<SetVariantSelectionCommand>(path, prim, variantName, variantSelection);
}

SetVariantSelectionCommand::SetVariantSelectionCommand(
    const Ufe::Path&       path,
    const PXR_NS::UsdPrim& prim,
    const std::string&     variantName,
    const std::string&     variantSelection)
    : _path(path)
    , _prim(prim)
    , _varSet(prim.GetVariantSets().GetVariantSet(variantName))
    , _oldSelection(_varSet.GetVariantSelection())
    , _newSelection(variantSelection)
{
}

void SetVariantSelectionCommand::redo()
{
    const PXR_NS::TfToken metadataKeyPath(_varSet.GetName());

    PrimMetadataEditRouterContext ctx(
        _prim, PXR_NS::SdfFieldKeys->VariantSelection, metadataKeyPath);

    std::string errMsg;
    if (!UsdUfe::isPrimMetadataEditAllowed(
            _prim, PXR_NS::SdfFieldKeys->VariantSelection, metadataKeyPath, &errMsg)) {
        throw std::runtime_error(errMsg.c_str());
    }

    // Backup the destination layer for consistent undo.
    _dstLayer = _prim.GetStage()->GetEditTarget().GetLayer();

    // Make a copy of the global selection, to restore it on undo.
    auto globalSn = Ufe::GlobalSelection::get();
    _savedSn.replaceWith(*globalSn);
    // Filter the global selection, removing items below our prim.
    globalSn->replaceWith(UsdUfe::removeDescendants(_savedSn, _path));
    _varSet.SetVariantSelection(_newSelection);
}

void SetVariantSelectionCommand::undo()
{
    PrimMetadataEditRouterContext ctx(_prim.GetStage(), _dstLayer);

    std::string errMsg;
    if (!UsdUfe::isPrimMetadataEditAllowed(
            _prim,
            PXR_NS::SdfFieldKeys->VariantSelection,
            PXR_NS::TfToken(_varSet.GetName()),
            &errMsg)) {
        throw std::runtime_error(errMsg.c_str());
    }

    _varSet.SetVariantSelection(_oldSelection);
    // Restore the saved selection to the global selection.  If a saved
    // selection item started with the prim's path, re-create it.
    auto globalSn = Ufe::GlobalSelection::get();
    globalSn->replaceWith(UsdUfe::recreateDescendants(_savedSn, _path));
}

} // namespace USDUFE_NS_DEF
