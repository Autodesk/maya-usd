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
class LAYEREDITOR_PUBLIC SessionState : public QObject
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

    virtual bool displayLayerContents() const { return _displayLayerContents; }
    virtual void setDisplayLayerContents(bool show);
    virtual bool displayLayerExpandAllValues() const { return _displayLayerExpandAllValues; }
    virtual void setDisplayLayerExpandAllValues(bool expand);
    virtual bool displayLayerHideIndices() const { return _displayLayerHideIndices; }
    virtual void setDisplayLayerHideIndices(bool hide);

    // Edit-forwarding state lives here, on the SessionState, rather than in the
    // layerEditorDCCFunctions registry because it depends on this instance's current stage. The
    // rule: stage/instance-dependent state is a SessionState virtual; stateless DCC capabilities
    // (e.g. supportsEditForwarding, handleEFEditTargetUpdate) live in the registry.
    virtual bool                   isEditForwardMode() const { return false; }
    virtual PXR_NS::SdfLayerRefPtr  effectiveTargetLayer() const;

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
    void editForwardingFallbackTargetChanged();

protected:
    StageEntry _currentStageEntry;
    bool       _autoHideSessionLayer = true;
    bool       _displayLayerContents { true };
    bool       _displayLayerExpandAllValues { false };
    bool       _displayLayerHideIndices { false };
};

} // namespace UsdLayerEditor

#endif // USDLAYEREDITOR_SESSIONSTATE_H
