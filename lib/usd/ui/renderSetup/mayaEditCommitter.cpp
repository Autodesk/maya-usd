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

#include "mayaEditCommitter.h"

#include <mayaUsd/undo/MayaUsdUndoBlock.h>
#include <mayaUsdUI/ui/undoChunkUtils.h>

#include <usdUfe/undo/UsdUndoUtils.h>

#include <pxr/usd/sdf/changeBlock.h>

namespace MayaUsdRenderSetup {

MayaEditCommitter::MayaEditCommitter(QObject* parent)
    : QObject(parent)
{
}

void MayaEditCommitter::setStages(const std::vector<AdskUsdRenderSetup::HostStage>& stages)
{
    _stages.clear();
    _stages.reserve(stages.size());
    for (const auto& hostStage : stages) {
        if (hostStage.stage) {
            _stages.push_back(hostStage.stage);
        }
    }
}

void MayaEditCommitter::commit(const std::string& undoLabel, std::function<void()> doEdit)
{
    if (!doEdit || _stages.empty()) {
        return;
    }

    UsdUfe::trackStagesEditTargets(_stages);

    const MayaUsdUI::UndoChunkGuard undoChunkGuard(undoLabel);
    MayaUsd::MayaUsdUndoBlock       block;
    {
        doEdit();
    }
}

} // namespace MayaUsdRenderSetup
