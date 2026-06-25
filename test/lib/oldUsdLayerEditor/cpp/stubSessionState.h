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

class OldEditorStubSessionState : public SessionState
{
    Q_OBJECT
public:
    OldEditorStubSessionState();

    AbstractCommandHook*     commandHook() override;
    std::vector<StageEntry>  allStages() const override;
    std::string              defaultLoadPath() const override;
    std::vector<std::string> loadLayersUI(
        const QString& title, const std::string& default_path) const override;
    bool saveLayerUI(
        QWidget*                      parent,
        std::string*                  out_filePath,
        const PXR_NS::SdfLayerRefPtr& parentLayer) const override;
    void printLayer(const PXR_NS::SdfLayerRefPtr& layer) const override;
    void setupCreateMenu(QMenu* menu) override;
    void rootLayerPathChanged(std::string const& path) override;
    bool autoHideSessionLayer() const override { return false; }
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
    bool isEditForwardMode()    const override { return _isEFModeActive; }
#endif

    // No-op setter: old editor SessionState has no editForwardingChanged signal,
    // so we just update the flag. The EF-active styling test is guarded to new editor only.
    void setIsEditForwardMode(bool v) { _isEFModeActive = v; }

    // Patches _stages[index]._proxyShapePath and refreshes the base-class
    // active entry if it matches.  Called by LayerEditorTestFixture::SetUp
    // after real Maya proxy shape nodes are created.
    void setProxyShapePath(int index, const std::string& path);

    // Pushes a new stage entry and makes it the active one. Mirrors the new
    // editor's StubSessionState so shared tests can switch the editor's stage
    // without a compile-time editor guard.
    void switchToCustomStage(PXR_NS::UsdStageRefPtr stage, const std::string& id = "custom_stage");

    // Used by LayerEditorWithEFFixture; has no effect on old editor widget since
    // it uses a compile-time #ifdef guard rather than a runtime check.
    bool _supportsEditForwarding { false };
    bool _isEFModeActive         { false };

    // Call counters — member names match new StubSessionState exactly.
    mutable int _saveLayerCallCount  { 0 };
    mutable int _printLayerCallCount { 0 };
    mutable int _loadLayersCallCount { 0 };

    std::string _stubbedLoadPath;

    OldEditorStubCommandHook _commandHookImpl;

private:
    std::vector<StageEntry> _stages;
    StageEntry makeEntry(PXR_NS::UsdStageRefPtr stage, const std::string& id);
};

} // namespace UsdLayerEditor
