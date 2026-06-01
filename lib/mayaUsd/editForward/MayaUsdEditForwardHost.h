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

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/notice.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/stage.h>

#include <AdskUsdEditForward/Host.h>
#include <AdskUsdEditForward/RuleDef.h>
#include <AdskUsdEditForward/StageRuleProvider.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/// \class MayaUsdEditForwardHost
/// \brief Maya-specific implementation of the USD Edit Forward host interface.
class MAYAUSD_CORE_PUBLIC MayaUsdEditForwardHost : public AdskUsdEditForward::Host
{
public:
    MayaUsdEditForwardHost();
    ~MayaUsdEditForwardHost() = default;

    void ExecuteInCmd(std::function<void()> callback, bool immediate) override;

    bool IsEditForwardingPaused() const override;
    void PauseEditForwarding(bool pause) override;
    void TrackLayerStates(const pxr::SdfLayerHandle& layer) override;

    bool WantsEcho() const override;
    void SetWantsEcho(bool echo) override;
    void Echo(const AdskUsdEditForward::Record& record) override;

private:
    bool _paused = false;
    bool _wantsEcho = false;
};

/// Notice sent when a stage's edit-forward fallback target changes.
class MAYAUSD_CORE_PUBLIC MayaUsdEFFallbackTargetChangedNotice : public PXR_NS::TfNotice
{
public:
    explicit MayaUsdEFFallbackTargetChangedNotice(const PXR_NS::UsdStageRefPtr& stage)
        : _stage(stage)
    {
    }

    PXR_NS::UsdStageRefPtr GetStage() const { return _stage; }

private:
    PXR_NS::UsdStageRefPtr _stage;
};

/**
 * Maya-level controller for Edit Forwarding mode.
 *
 * Acts as the edit forwarding rule provider for a stage, while also managing the
 * edit forwarding state in maya-usd.
 *
 * As a rule provider it extends StageRuleProvider by appending an in-memory
 * catch-all "fallback" rule that targets a layer chosen by the caller. That rule
 * lives purely in memory and never touches the root layer custom data.
 *
 * EF mode is considered active whenever GetRules() returns a non-empty list and
 * IsContinuous() returns true.
 *
 * The controller activates and deactivates EF mode by listening for layer changes
 * on the root layer. On activation the stage edit target is moved to the session
 * layer; on deactivation the last fallback target is restored as the stage edit target.
 */
class MAYAUSD_CORE_PUBLIC MayaUsdEditForwardController
    : public AdskUsdEditForward::StageRuleProvider
    , public PXR_NS::TfWeakBase
{
public:
    using Ptr = std::shared_ptr<MayaUsdEditForwardController>;

    explicit MayaUsdEditForwardController(const PXR_NS::UsdStageRefPtr& stage);
    ~MayaUsdEditForwardController();

    // IRuleProvider overrides
    std::vector<AdskUsdEditForward::RuleDef::Ptr> GetRules() const override;

    // Set/clear the catch-all fallback target layer. Does not affect whether EF is active.
    void                         setFallbackTarget(const PXR_NS::SdfLayerRefPtr& layer);
    void                         clearFallbackTarget();
    PXR_NS::SdfLayerRefPtr       fallbackTarget() const { return _fallbackTarget; }

    // EF active state — true when continuous rules are present.
    bool                   isForwardingActive() const { return _efActive; }

    // Re-evaluate whether EF mode should be on or off.
    void syncEditForwardMode();

    static Ptr  GetForStage(const PXR_NS::UsdStageRefPtr& stage);
    static void RegisterForStage(const PXR_NS::UsdStageRefPtr& stage, const Ptr& controller);

private:
    void _onLayerChanged(
        const PXR_NS::SdfNotice::LayersDidChangeSentPerLayer& notice,
        const PXR_NS::TfWeakPtr<PXR_NS::SdfLayer>&            sender);
    void _onEditTargetChanged(
        const PXR_NS::UsdNotice::StageEditTargetChanged& notice,
        const PXR_NS::TfWeakPtr<PXR_NS::UsdStage>&       sender);

    void _setFallbackTarget(const PXR_NS::SdfLayerRefPtr& layer);

    PXR_NS::UsdStageRefPtr  _stage;
    PXR_NS::SdfLayerRefPtr  _fallbackTarget;
    bool                    _efActive { false };

    PXR_NS::TfNotice::Keys  _noticeKeys;
};

#endif // MAYAUSD_EDITFORWARDHOST_H
