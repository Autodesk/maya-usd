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

USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoClearDefaultPrimCommand);

UsdUndoClearDefaultPrimCommand::UsdUndoClearDefaultPrimCommand(const UsdPrim& prim)
    : Ufe::UndoableCommand()
    , _stage(prim.GetStage())
{
}

UsdUndoClearDefaultPrimCommand::UsdUndoClearDefaultPrimCommand(const PXR_NS::UsdStageRefPtr& stage)
    : _stage(stage)
{
}

void UsdUndoClearDefaultPrimCommand::execute()
{
    if (!_stage)
        return;

    // Check if the default prim can be cleared.
    applyRootLayerMetadataRestriction(_stage, "clear default prim");

    // Clear the stage's default prim.
    UsdUndoBlock undoBlock(&_undoableItem);
    _stage->ClearDefaultPrim();
}

void UsdUndoClearDefaultPrimCommand::redo() { _undoableItem.redo(); }

void UsdUndoClearDefaultPrimCommand::undo() { _undoableItem.undo(); }

} // namespace USDUFE_NS_DEF
