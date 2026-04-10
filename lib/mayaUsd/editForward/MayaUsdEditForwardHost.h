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
 * Maya-level rule provider for Edit Forwarding mode.
 *
 * Extends StageRuleProvider by appending an in-memory catch-all "fallback"
 * rule that targets a layer chosen by the caller. The rule lives purely in
 * memory and never touches the root layer custom data.
 *
 * Whether forwarding is active is stored in the session layer's custom data
 * under the key "adsk_forward_active", making it stage-attached and
 * inspectable from Python without a separate in-memory flag.
 *
 * IsContinuous() always returns true so forwarding runs continuously.
 */
class MAYAUSD_CORE_PUBLIC MayaForwardRuleProvider : public AdskUsdEditForward::StageRuleProvider
{
public:
    using Ptr = std::shared_ptr<MayaForwardRuleProvider>;

    explicit MayaForwardRuleProvider(const PXR_NS::UsdStageRefPtr& stage);

    // IRuleProvider overrides
    std::vector<AdskUsdEditForward::RuleDef::Ptr> GetRules() const override;
    bool IsContinuous() const override { return true; }

    // Enable/disable forwarding. Reads/writes "adsk_forward_active" on the
    // stage's session layer custom data — no separate in-memory flag.
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Set/clear the catch-all fallback target layer.
    void                         setFallbackTarget(const PXR_NS::SdfLayerRefPtr& layer);
    void                         clearFallbackTarget();
    PXR_NS::SdfLayerRefPtr       fallbackTarget() const { return _fallbackTarget; }

    // Registry — look up the provider for a specific stage without going
    // through proxyShapeBase.
    static Ptr  GetForStage(const PXR_NS::UsdStageRefPtr& stage);
    static void RegisterForStage(const PXR_NS::UsdStageRefPtr& stage, const Ptr& provider);

private:
    static std::string regexEscapeLayerPath(const std::string& input);

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _fallbackTarget;

    static std::mutex                                                                s_registryMutex;
    static std::unordered_map<const PXR_NS::UsdStage*, std::weak_ptr<MayaForwardRuleProvider>>
                                                                                     s_registry;
};

} // namespace MayaUsd

#endif // MAYAUSD_EDITFORWARDHOST_H
