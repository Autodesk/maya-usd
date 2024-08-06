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
#include "UsdUndoSetDefaultPrimCommand.h"

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/utils/editRouter.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/usd/usdGeom/imageable.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoSetDefaultPrimCommand);

UsdUndoSetDefaultPrimCommand::UsdUndoSetDefaultPrimCommand(const UsdPrim& prim)
    : Ufe::UndoableCommand()
    , _prim(prim)
{
}

void UsdUndoSetDefaultPrimCommand::execute()
{
    const PXR_NS::UsdStageRefPtr stage = _prim.GetStage();
    if (!stage)
        return;

    // Check if the default prim can be set.
    applyRootLayerMetadataRestriction(stage, "set default prim");

    // Set the stage's default prim to the given prim.
    UsdUndoBlock undoBlock(&_undoableItem);
    stage->SetDefaultPrim(_prim);
}

void UsdUndoSetDefaultPrimCommand::redo() { _undoableItem.redo(); }

void UsdUndoSetDefaultPrimCommand::undo() { _undoableItem.undo(); }

} // namespace USDUFE_NS_DEF
