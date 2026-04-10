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
#ifndef MAYAUSD_EDITFORWARDHOST_H
#define MAYAUSD_EDITFORWARDHOST_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <AdskUsdEditForward/Host.h>
#include <AdskUsdEditForward/RuleDef.h>
#include <AdskUsdEditForward/StageRuleProvider.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/// \class MayaUsdEditForwardHost
/// \brief Maya-specific implementation of the USD Edit Forward host interface.
class MAYAUSD_CORE_PUBLIC MayaUsdEditForwardHost : public AdskUsdEditForward::Host
{
public:
    MayaUsdEditForwardHost() = default;
    ~MayaUsdEditForwardHost() = default;

    void ExecuteInCmd(std::function<void()> callback, bool immediate) override;

    bool IsEditForwardingPaused() const override;
    void PauseEditForwarding(bool pause) override;
    void TrackLayerStates(const pxr::SdfLayerHandle& layer) override;

private:
    bool _paused = false;
};

namespace MayaUsd {

/**
 * PROTOTYPE rule provider for the Layer Editor's Edit Forwarding mode.
 *
 * Extends StageRuleProvider by appending an in-memory "preview" rule that
 * targets whatever layer the user has selected in the Layer Editor while in
 * EF mode.  The rule lives purely in memory — it is never written to the
 * stage's root layer custom data — and disappears when EF mode is exited.
 *
 * IsContinuous() always returns true so that forwarding runs continuously
 * as edits arrive on the session layer.
 */
class MAYAUSD_CORE_PUBLIC LayerEditorRuleProvider : public AdskUsdEditForward::StageRuleProvider
{
public:
    using Ptr = std::shared_ptr<LayerEditorRuleProvider>;

    explicit LayerEditorRuleProvider(const PXR_NS::UsdStageRefPtr& stage);

    // IRuleProvider overrides
    std::vector<AdskUsdEditForward::RuleDef::Ptr> GetRules() const override;
    bool IsContinuous() const override { return true; }

    // Set the layer that the in-memory preview rule should forward to.
    void setPreviewTarget(const PXR_NS::SdfLayerRefPtr& layer);

    // Remove the in-memory preview rule (call when exiting EF mode).
    void clearPreviewTarget();

    // Registry — look up the provider created for a specific stage so that
    // the layer editor can reach it without going through proxyShapeBase.
    static Ptr  GetForStage(const PXR_NS::UsdStageRefPtr& stage);
    static void RegisterForStage(const PXR_NS::UsdStageRefPtr& stage, const Ptr& provider);

private:
    static std::string regexEscapeLayerPath(const std::string& input);

    PXR_NS::SdfLayerRefPtr _previewTarget;

    static std::mutex                                                                   s_registryMutex;
    static std::unordered_map<const PXR_NS::UsdStage*, std::weak_ptr<LayerEditorRuleProvider>>
                                                                                        s_registry;
};

} // namespace MayaUsd

#endif // MAYAUSD_EDITFORWARDHOST_H
