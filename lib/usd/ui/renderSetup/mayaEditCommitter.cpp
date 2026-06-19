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

#include "../undoChunkUtils.h"

#include <mayaUsd/undo/MayaUsdUndoBlock.h>

#include <usdUfe/undo/UsdUndoUtils.h>

#include <pxr/usd/sdf/changeBlock.h>

#include <QtCore/QTimer>

namespace MayaUsdRenderSetup {

MayaEditCommitter::MayaEditCommitter(QObject* parent)
    : QObject(parent)
{
}

void MayaEditCommitter::setStages(const std::vector<Adsk::HostStage>& stages)
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

    ++_inFlightCount;
    // Schedule the decrement before doEdit() so it fires even if doEdit() throws.
    // The one-event-loop-cycle delay keeps isLocalEditInFlight() true through the
    // USD-notice burst that follows the edit, so the notice bridge treats the
    // resulting refresh as self-originated and skips re-entrancy.
    QTimer::singleShot(0, this, [this]() {
        if (_inFlightCount > 0) {
            --_inFlightCount;
        }
    });

    UsdUfe::trackStagesEditTargets(_stages);

    const MayaUsdUI::UndoChunkGuard undoChunkGuard(undoLabel);
    MayaUsd::MayaUsdUndoBlock       block;
    {
        PXR_NS::SdfChangeBlock changeBlock;
        doEdit();
    }
}

} // namespace MayaUsdRenderSetup
