//
// Copyright 2026 Autodesk
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

#include "UsdUndoUtils.h"

#include <usdUfe/undo/UsdUndoManager.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

void trackStagesEditTargets(const std::vector<UsdStageRefPtr>& stages)
{
    for (const auto& stage : stages) {
        if (!stage) {
            continue;
        }
        UsdUndoManager::instance().trackLayerStates(stage->GetEditTarget().GetLayer());
    }
}

} // namespace USDUFE_NS_DEF
