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

#ifndef MAYASESSIONSTATE_H
#define MAYASESSIONSTATE_H

#include "mayaCommandHook.h"
#include "sessionState.h"

#include <mayaUsd/listeners/proxyShapeNotice.h>

#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/notice.h>

#include <maya/MMessage.h>

#include <vector>

namespace UsdLayerEditor {
PXR_NAMESPACE_USING_DIRECTIVE

/**
 * @brief Implements the SessionState virtual class to wraps the Maya stage and Maya-specific UI
 * for the Layer Editor
 *
 */
class MayaSessionState
    : public SessionState
    , public PXR_NS::TfWeakBase

{
    Q_OBJECT
public:
    typedef SessionState PARENT_CLASS;

    MayaSessionState();
    ~MayaSessionState();

    // API implementation
    void                    setStage(PXR_NS::UsdStageRefPtr const& in_stage) override;
    void                    setAutoHideSessionLayer(bool hide) override;
    AbstractCommandHook*    commandHook() override;
    std::vector<StageEntry> allStages() const override;
    // path to default load layer dialogs to
    std::string defaultLoadPath() const override;
    // ui that returns a list of paths to load
    std::vector<std::string>
    loadLayersUI(const QString& title, const std::string& default_path) const override;
    // ui to save a layer. returns the path
    bool saveLayerUI(QWidget* in_parent, std::string* out_filePath) const override;
    void printLayer(const PXR_NS::SdfLayerRefPtr& layer) const override;

    // main API
    void setupCreateMenu(QMenu* in_menu) override;
    // called when an anonymous root layer has been saved to a file
    // in this case, the stage needs to be re-created on the new file
    void rootLayerPathChanged(std::string const& in_path) override;

    std::string proxyShapePath() { return _currentProxyShapePath; }

Q_SIGNALS:
    void clearUIOnSceneResetSignal();

public:
    void registerNotifications();
    void unregisterNotifications();

protected:
    // get the stage and proxy name for a path
    static bool getStageEntry(StageEntry* out_stageEntry, const MString& shapePath);

    // maya callback handers
    static void proxyShapeAddedCB(MObject& node, void* clientData);
    static void proxyShapeRemovedCB(MObject& node, void* clientData);
    static void nodeRenamedCB(MObject& node, const MString& oldName, void* clientData);
    static void sceneClosingCB(void* clientData);

    void proxyShapeAddedCBOnIdle(const MObject& node);
    void nodeRenamedCBOnIdle(const MObject& obj);

    // Notice listener method for proxy stage set
    void mayaUsdStageReset(const MayaUsdProxyStageSetNotice& notice);
    void mayaUsdStageResetCBOnIdle(const std::string& shapePath, UsdStageRefPtr const& stage);

    std::vector<MCallbackId> _callbackIds;
    TfNotice::Key            _stageResetNoticeKey;
    std::string              _currentProxyShapePath;
    MayaCommandHook          _mayaCommandHook;
};

} // namespace UsdLayerEditor

#endif // MAYASESSIONSTATE_H
