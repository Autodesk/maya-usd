//
// Copyright 2019 Autodesk
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
#include "UsdUndoDeleteCommand.h"

#include "private/UfeNotifGuard.h"

#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editContext.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdAttributes.h>
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDeleteCommand::UsdUndoDeleteCommand(const PXR_NS::UsdPrim& prim)
    : Ufe::UndoableCommand()
    , _prim(prim)
{
}

UsdUndoDeleteCommand::~UsdUndoDeleteCommand() { }

UsdUndoDeleteCommand::Ptr UsdUndoDeleteCommand::create(const PXR_NS::UsdPrim& prim)
{
    return std::make_shared<UsdUndoDeleteCommand>(prim);
}

void UsdUndoDeleteCommand::execute()
{
    if (!_prim.IsValid())
        return;

    enforceMutedLayer(_prim, "remove");

    UsdUfe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

#ifdef MAYA_ENABLE_NEW_PRIM_DELETE
    const auto& stage = _prim.GetStage();
    auto        targetPrimSpec = stage->GetEditTarget().GetPrimSpecForScenePath(_prim.GetPath());

    if (UsdUfe::applyCommandRestrictionNoThrow(_prim, "delete")) {
#ifdef UFE_V4_FEATURES_AVAILABLE
        UsdAttributes::removeAttributesConnections(_prim);
#endif
        // Let removeAttributesConnections be run first as it will also cleanup
        // attributes that were authored only to be the destination of a connection.
        if (!UsdUfe::cleanReferencedPath(_prim)) {
            const std::string error = TfStringPrintf(
                "Failed to cleanup references to prim \"%s\".", _prim.GetPath().GetText());
            TF_WARN("%s", error.c_str());
            throw std::runtime_error(error);
        }
        PrimSpecFunc deleteFunc
            = [stage](const UsdPrim& prim, const SdfPrimSpecHandle& primSpec) -> void {
            PXR_NS::UsdEditContext ctx(stage, primSpec->GetLayer());
            if (!stage->RemovePrim(prim.GetPath())) {
                const std::string error
                    = TfStringPrintf("Failed to delete prim \"%s\".", prim.GetPath().GetText());
                TF_WARN("%s", error.c_str());
                throw std::runtime_error(error);
            }
        };
        applyToAllPrimSpecs(_prim, deleteFunc);
    }
#else
    _prim.SetActive(false);
#endif
}

void UsdUndoDeleteCommand::undo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoDeleteCommand::redo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
