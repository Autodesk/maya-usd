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

namespace UsdLayerEditor {

OldEditorStubSessionState::OldEditorStubSessionState()
    : _commandHookImpl(this)
{
    // Create a couple of test stages to populate the layer editor.
    for (int i = 0; i < 2; ++i) {
        auto stage    = PXR_NS::UsdStage::CreateInMemory();
        auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sublayer" + std::to_string(i));
        stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
        _stages.push_back(makeEntry(stage, "stub_stage_" + std::to_string(i)));
    }
    setStageEntry(_stages[0]);
}

AbstractCommandHook* OldEditorStubSessionState::commandHook()
{
    return &_commandHookImpl;
}

std::vector<SessionState::StageEntry> OldEditorStubSessionState::allStages() const
{
    return _stages;
}

std::string OldEditorStubSessionState::defaultLoadPath() const
{
    return "/tmp";
}

std::vector<std::string> OldEditorStubSessionState::loadLayersUI(
    const QString& /*title*/, const std::string& /*default_path*/) const
{
    ++_loadLayersCallCount;
    if (!_stubbedLoadPath.empty()) {
        return { _stubbedLoadPath };
    }
    return {};
}

bool OldEditorStubSessionState::saveLayerUI(
    QWidget* /*parent*/,
    std::string* /*out_filePath*/,
    const PXR_NS::SdfLayerRefPtr& /*parentLayer*/) const
{
    ++_saveLayerCallCount;
    // Simulates user cancelling the save dialog.
    return false;
}

void OldEditorStubSessionState::printLayer(const PXR_NS::SdfLayerRefPtr& /*layer*/) const
{
    ++_printLayerCallCount;
}

void OldEditorStubSessionState::setupCreateMenu(QMenu* menu)
{
    if (menu) {
        menu->addAction("Stub Create Action");
    }
}

void OldEditorStubSessionState::rootLayerPathChanged(std::string const& /*path*/) { }

SessionState::StageEntry
OldEditorStubSessionState::makeEntry(PXR_NS::UsdStageRefPtr stage, const std::string& id)
{
    StageEntry entry;
    entry._id             = id;
    entry._stage          = stage;
    entry._displayName    = id;
    entry._proxyShapePath = id;
    return entry;
}

void OldEditorStubSessionState::switchToCustomStage(
    PXR_NS::UsdStageRefPtr stage,
    const std::string&     id)
{
    auto entry = makeEntry(stage, id);
    _stages.push_back(entry);
    setStageEntry(entry);
}

void OldEditorStubSessionState::setProxyShapePath(int index, const std::string& path)
{
    if (index < 0 || index >= static_cast<int>(_stages.size()))
        return;
    _stages[index]._proxyShapePath = path;
    // Refresh the base-class active entry copy when this index is the current stage.
    if (stageEntry()._id == _stages[index]._id)
        SessionState::setStageEntry(_stages[index]);
}

} // namespace UsdLayerEditor
