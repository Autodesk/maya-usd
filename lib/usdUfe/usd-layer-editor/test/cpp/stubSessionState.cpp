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

#include "stubSessionState.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QMenu>

#include <algorithm>

namespace UsdLayerEditor {

StubSessionState::StubSessionState()
    : _commandHookImpl(this)
{
    // Two in-memory stages, each with one anonymous sublayer.
    for (int i = 0; i < 2; ++i) {
        auto stage    = PXR_NS::UsdStage::CreateInMemory();
        auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sublayer" + std::to_string(i));
        stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
        _stages.push_back(makeEntry(stage, "stub_stage_" + std::to_string(i)));
    }
    setStageEntry(_stages[0]);
}

AbstractCommandHook* StubSessionState::commandHook()
{
    return &_commandHookImpl;
}

std::vector<SessionState::StageEntry> StubSessionState::allStages() const
{
    return _stages;
}

std::vector<SessionState::StageEntry> StubSessionState::selectedStages() const
{
    return { _currentStageEntry };
}

std::string StubSessionState::defaultLoadPath() const
{
    return "/tmp";
}

std::vector<std::string> StubSessionState::loadLayersUI(
    const QString& /*title*/, const std::string& /*default_path*/) const
{
    ++_loadLayersCallCount;
    if (!_stubbedLoadPath.empty()) {
        return { _stubbedLoadPath };
    }
    return {};
}

bool StubSessionState::saveLayerUI(
    QWidget* /*parent*/,
    std::string* out_filePath,
    const PXR_NS::SdfLayerRefPtr& /*parentLayer*/) const
{
    ++_saveLayerCallCount;
    // Simulates user cancelling the save dialog. Returning false prevents
    // saveAnonymousLayer() from calling Ufe::PathString::path() with a stub path
    // that has no valid UFE representation, which would throw in a test environment.
    return false;
}

void StubSessionState::printLayer(const PXR_NS::SdfLayerRefPtr& /*layer*/) const
{
    ++_printLayerCallCount;
}

void StubSessionState::refreshCurrentStageEntry() { }
void StubSessionState::refreshStageEntry(std::string const& /*dccObjectPath*/) { }

void StubSessionState::setupCreateMenu(QMenu* menu)
{
    if (menu) {
        menu->addAction("Stub Create Action");
    }
}

void StubSessionState::rootLayerPathChanged(std::string const& /*path*/) { }

void StubSessionState::addStage(PXR_NS::UsdStageRefPtr stage)
{
    auto entry = makeEntry(stage, "added_stage_" + std::to_string(_stages.size()));
    _stages.push_back(entry);
    Q_EMIT stageListChangedSignal(entry);
}

void StubSessionState::removeStage(const std::string& id)
{
    _stages.erase(
        std::remove_if(
            _stages.begin(),
            _stages.end(),
            [&id](const StageEntry& e) { return e._id == id; }),
        _stages.end());
    Q_EMIT stageListChangedSignal();
}

SessionState::StageEntry
StubSessionState::makeEntry(PXR_NS::UsdStageRefPtr stage, const std::string& id)
{
    StageEntry e;
    e._id            = id;
    e._stage         = stage;
    e._displayName   = id;
    e._dccObjectPath = id;
    return e;
}

} // namespace UsdLayerEditor
