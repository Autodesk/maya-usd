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

#include "UsdUndoManager.h"
#include "UsdUndoBlock.h"
#include "UsdUndoStateDelegate.h"

#include <pxr/usd/sdf/layer.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

UsdUndoManager& UsdUndoManager::instance()
{
    static UsdUndoManager undoManager;
    return undoManager;
}

void UsdUndoManager::trackLayerStates(const SdfLayerHandle& layer) 
{
    layer->SetStateDelegate(UsdUndoStateDelegate::New());
}

void UsdUndoManager::addInverse(InvertFunc func) 
{
    if (UsdUndoBlock::depth() == 0){
        TF_CODING_ERROR("Collecting invert functions outside of undoblock is not allowed!");
        return;
    }

    _invertFuncs.emplace_back(func); 
}

void UsdUndoManager::transferEdits(UsdUndoableItem& undoableItem)
{
    // transfer the edits
    undoableItem._invertFuncs = std::move(_invertFuncs);
}

} // namespace MAYAUSD_NS_DEF
