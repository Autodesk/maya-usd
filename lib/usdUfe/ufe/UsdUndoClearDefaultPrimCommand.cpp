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
#include "UsdUndoClearDefaultPrimCommand.h"

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/usd/usdGeom/imageable.h>

namespace USDUFE_NS_DEF {

UsdUndoClearDefaultPrimCommand::UsdUndoClearDefaultPrimCommand(const UsdPrim& prim)
    : Ufe::UndoableCommand()
    , _prim(prim)
{
    _stage = _prim.GetStage();
    if (!_stage) {
        return;
    }
}

UsdUndoClearDefaultPrimCommand::~UsdUndoClearDefaultPrimCommand() { }

void UsdUndoClearDefaultPrimCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);

    // Check if the layer selected is the root layer.
    if (_stage->GetRootLayer() != _stage->GetEditTarget().GetLayer()) {
        TF_WARN(
            "Stage metadata [defaultPrim] can only be modified when the root layer is targeted "
            "[%s]",
            _stage->GetRootLayer()->GetDisplayName().c_str());
        return;
    }
    // Clear the stage's default prim.
    _stage->ClearDefaultPrim();
}

void UsdUndoClearDefaultPrimCommand::redo() { _undoableItem.redo(); }

void UsdUndoClearDefaultPrimCommand::undo() { _undoableItem.undo(); }

} // namespace USDUFE_NS_DEF
