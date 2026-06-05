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
#ifndef USDLAYEREDITOR_SESSIONSTATE_H
#define USDLAYEREDITOR_SESSIONSTATE_H

#include "layerEditorAPI.h"
#include "abstractCommandHook.h"

#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/stage.h>

#include <QtCore/QObject>
#include <cstddef>
#include <string>
#include <vector>

class QMenu;
class QWidget;

namespace UsdLayerEditor {

/**
 * @brief Abstract class that wraps the editing session stage, including the stage list, the current
 * stage, and app-specific UI
 *
 */
class LayerEditorAPI SessionState : public QObject
{
    Q_OBJECT
public:
    virtual ~SessionState() { }

    struct StageEntry
    {
        std::string            _id;
        PXR_NS::UsdStageRefPtr _stage;
        std::string            _displayName;
        std::string            _dccObjectPath;

        StageEntry()
        {
            _id = "";
            _stage = PXR_NS::UsdStageRefPtr();
            _displayName = "";
            _dccObjectPath = "";
        }

        bool operator==(const StageEntry& entry) const
        {
            return (
                _id == entry._id && _stage == entry._stage && _displayName == entry._displayName
                && _dccObjectPath == entry._dccObjectPath);
        }

        bool operator!=(const StageEntry& entry) const { return !(*this == entry); }

        void clear()
        {
            _stage = PXR_NS::UsdStageRefPtr();
            _displayName = "";
            _dccObjectPath = "";
        }
    };

    // properties
    virtual bool                    autoHideSessionLayer() const { return _autoHideSessionLayer; }
    virtual void                    setAutoHideSessionLayer(bool hide);
    virtual bool                    autoObserveUfeSelection() const { return true; }

    // Layer-contents display options. Default implementations store the value
    // in protected members; DCC integrations (e.g. MayaSessionState) override
    // these setters to persist the value in their preference store (optionVar
    // etc.). The default getters return the cached value.
    virtual bool displayLayerContents() const { return _displayLayerContents; }
    virtual void setDisplayLayerContents(bool show);
    virtual bool displayLayerExpandAllValues() const { return _displayLayerExpandAllValues; }
    virtual void setDisplayLayerExpandAllValues(bool expand);
    virtual bool displayLayerHideIndices() const { return _displayLayerHideIndices; }
    virtual void setDisplayLayerHideIndices(bool hide);

    // Edit Forwarding hooks. The shared component does not depend on any
    // particular EF implementation (which lives in DCC-specific code, e.g.
    // AdskUsdEditForward / MayaUsdEditForwardHost on the Maya side). The
    // shared layer-editor widget only needs to know whether the current
    // stage has active edit forwarding so it can show/hide the banner. DCC
    // integrations override hasEditForwarding() to drive the real state and
    // emit editForwardingChanged() when it changes. echoEditForwarding /
    // setEchoEditForwarding are no-ops in the shared base; DCC integrations
    // wire them through to whatever EF-host preference exists.
    // supportsEditForwarding() returns true when the DCC integration is
    // built with EF support — used by the UI to decide whether to show the
    // Echo Edit Forwarding menu item.
    virtual bool supportsEditForwarding() const { return false; }
    virtual bool hasEditForwarding() const { return false; }
    virtual bool echoEditForwarding() const { return false; }
    virtual void setEchoEditForwarding(bool /*echo*/) { /* no-op */ }
    virtual bool isEditForwardMode() const { return false; }
    virtual PXR_NS::SdfLayerRefPtr effectiveTargetLayer() const;

    // Component Creator hooks. The shared component does not depend on any
    // particular Component Creator implementation (Maya's MayaUsd::ComponentUtils
    // / MayaComponentManager Python package). The shared save flow asks the
    // session state whether a stage is a component, whether the initial save
    // dialog should be displayed, queries the default scene folder for the
    // component widget, and asks the session state to move (rename/relocate)
    // a component on disk. Default implementations are no-ops / false / empty
    // so a DCC integration without component support simply skips the CC
    // branches.
    virtual bool isStageAComponent(const std::string& /*dccObjectPath*/) const { return false; }
    virtual bool isUnsavedComponent(const PXR_NS::UsdStageRefPtr& /*stage*/) const { return false; }
    virtual bool shouldDisplayComponentInitialSaveDialog(
        const PXR_NS::UsdStageRefPtr& /*stage*/,
        const std::string& /*dccObjectPath*/) const
    {
        return false;
    }
    virtual std::string sceneFolder() const { return {}; }
    // Move (rename / relocate) the component at dccObjectPath to saveLocation
    // under componentName. Returns the new root layer path on success, or an
    // empty string on failure. Default implementation is a no-op.
    virtual std::string moveComponent(
        const std::string& /*saveLocation*/,
        const std::string& /*componentName*/,
        const std::string& /*dccObjectPath*/)
    {
        return {};
    }
    // Preview the structure of a component at dccObjectPath when saved at
    // saveLocation under componentName. Returns a JSON-encoded hierarchy or
    // empty string when not supported. Default implementation returns empty.
    virtual std::string previewComponentSave(
        const std::string& /*saveLocation*/,
        const std::string& /*componentName*/,
        const std::string& /*dccObjectPath*/) const
    {
        return {};
    }
    // Returns the list of layer ids that should be considered when saving the
    // component at dccObjectPath. The default implementation returns an empty
    // vector and the caller falls back to normal layer counting.
    virtual std::vector<std::string>
    getComponentLayersToSave(const std::string& /*dccObjectPath*/) const
    {
        return {};
    }

    PXR_NS::UsdStageRefPtr const&   stage() const { return _currentStageEntry._stage; }
    StageEntry const&               stageEntry() const { return _currentStageEntry; }
    PXR_NS::SdfLayerRefPtr          targetLayer() const;
    virtual void                    setStageEntry(StageEntry const& in_entry);
    virtual AbstractCommandHook*    commandHook() = 0;
    virtual std::vector<StageEntry> allStages() const = 0;
    virtual std::vector<StageEntry> selectedStages() const = 0;

    // path to default load layer dialogs to
    virtual std::string defaultLoadPath() const = 0;
    // ui that returns a list of paths to load
    virtual std::vector<std::string>
    loadLayersUI(const QString& title, const std::string& default_path) const = 0;
    // ui to save a layer. returns the path
    virtual bool saveLayerUI(
        QWidget*                      in_parent,
        std::string*                  out_filePath,
        const PXR_NS::SdfLayerRefPtr& parentLayer) const
        = 0;
    virtual void printLayer(const PXR_NS::SdfLayerRefPtr& layer) const = 0;

    virtual void refreshCurrentStageEntry() = 0;
    virtual void refreshStageEntry(std::string const& dccObjectPath) = 0;

    // main API
    virtual void setupCreateMenu(QMenu* in_menu) = 0;
    // called when an anonymous root layer has been saved to a file
    // in this case, the stage needs to be re-created on the new file
    virtual void rootLayerPathChanged(std::string const& in_path) = 0;

    bool isValid()
    {
        return _currentStageEntry._stage && _currentStageEntry._stage->GetRootLayer();
    }

Q_SIGNALS:
    void currentStageChangedSignal();
    void stageListChangedSignal(StageEntry const& toSelect = StageEntry());
    void stageRenamedSignal(StageEntry const& renamedEntry);
    void autoHideSessionLayerSignal(bool hideIt);
    void stageResetSignal(StageEntry const& entry);
    void dccSelectionChangedSignal();
    void showDisplayLayerContents(bool showIt);
    // Emitted by DCC integrations when the EF state of the current stage changes.
    void editForwardingChanged();
    // Emitted by DCC integrations when the EF fallback target layer changes.
    void editForwardingFallbackTargetChangedSignal();

protected:
    StageEntry _currentStageEntry;
    bool       _autoHideSessionLayer = true;
    bool       _displayLayerContents { true };
    bool       _displayLayerExpandAllValues { false };
    bool       _displayLayerHideIndices { false };
};

} // namespace UsdLayerEditor

#endif // SESSIONSTATE_H
