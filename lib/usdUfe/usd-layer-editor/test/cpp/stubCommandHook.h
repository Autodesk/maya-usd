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
#pragma once

#include "abstractCommandHook.h"

#include <string>
#include <string_view>
#include <vector>

namespace UsdLayerEditor {

struct CommandCall {
    std::string              name;
    std::vector<std::string> args;
};

class StubCommandHook : public AbstractCommandHook
{
public:
    explicit StubCommandHook(SessionState* sessionState);

    void     setEditTarget(UsdLayer layer) override;
    void     insertSubLayerPath(UsdLayer layer, Path path, int index) override;
    void     removeSubLayerPath(UsdLayer layer, Path path) override;
    void     replaceSubLayerPath(UsdLayer layer, Path oldPath, Path newPath) override;
    void     moveSubLayerPath(Path path, UsdLayer oldParent, UsdLayer newParent, int index) override;
    void     discardEdits(UsdLayer layer) override;
    void     clearLayer(UsdLayer layer) override;
    void     flattenLayer(UsdLayer layer) override;
    UsdLayer addAnonymousSubLayer(UsdLayer layer, std::string newName) override;
    void     muteSubLayer(UsdLayer layer, bool muteIt) override;
    void     lockLayer(UsdLayer layer, LayerLockType lockState, bool includeSubLayers) override;
    void     refreshLayerSystemLock(UsdLayer layer, bool refreshSubLayers = false) override;
    void     stitchLayers(const std::vector<PXR_NS::SdfLayerRefPtr>& layers) override;
    void     openUndoBracket(const QString& name) override;
    void     closeUndoBracket() override;
    void     showLayerEditorHelp() override;
    void     selectPrimsWithSpec(UsdLayer layer) override;
    bool     isDccObjectSharedStage(const std::string& /*dccObjectPath*/) override { return false; }

    void               clearCalls();
    bool               hasCall(std::string_view method) const;
    int                callCount(std::string_view method) const;
    const CommandCall& lastCall() const;

    std::vector<CommandCall> _calls;

protected:
    void executeDelayedCommands() override { }
};

} // namespace UsdLayerEditor
