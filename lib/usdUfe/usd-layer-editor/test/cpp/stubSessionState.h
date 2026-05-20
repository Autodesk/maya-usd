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

#include "sessionState.h"
#include "stubCommandHook.h"

#include <pxr/usd/usd/stage.h>

#include <string>
#include <vector>

class QMenu;
class QWidget;

namespace UsdLayerEditor {

class StubSessionState : public SessionState
{
    Q_OBJECT
public:
    StubSessionState();

    AbstractCommandHook*     commandHook() override;
    std::vector<StageEntry>  allStages() const override;
    std::vector<StageEntry>  selectedStages() const override;
    std::string              defaultLoadPath() const override;
    std::vector<std::string> loadLayersUI(
        const QString& title, const std::string& default_path) const override;
    bool saveLayerUI(
        QWidget*                      parent,
        std::string*                  out_filePath,
        const PXR_NS::SdfLayerRefPtr& parentLayer) const override;
    void printLayer(const PXR_NS::SdfLayerRefPtr& layer) const override;
    void refreshCurrentStageEntry() override;
    void refreshStageEntry(std::string const& dccObjectPath) override;
    void setupCreateMenu(QMenu* menu) override;
    void rootLayerPathChanged(std::string const& path) override;

    // Test helpers
    void addStage(PXR_NS::UsdStageRefPtr stage);
    void removeStage(const std::string& id);

    // Call counters
    mutable int _saveLayerCallCount { 0 };
    mutable int _printLayerCallCount { 0 };
    mutable int _loadLayersCallCount { 0 };

    // Pre-baked path returned by loadLayersUI (set in tests)
    std::string _stubbedLoadPath;

    StubCommandHook _commandHookImpl;

private:
    std::vector<StageEntry> _stages;
    StageEntry              makeEntry(PXR_NS::UsdStageRefPtr stage, const std::string& id);
};

} // namespace UsdLayerEditor
