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

#include "renderLayerStageTracker.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>

#include <maya/MSceneMessage.h>

#if defined(ADSK_ABI) && ADSK_ABI >= 2027
#include <AdskUsdRenderSetup/RenderLayerManager.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace MayaUsdRenderSetup {

namespace {
RenderLayerStageTracker* _instance = nullptr;
}

RenderLayerStageTracker::RenderLayerStageTracker()
{
    auto me = TfCreateWeakPtr(this);
    _stageSetKey = TfNotice::Register(me, &RenderLayerStageTracker::onStageSet);

    // A new or opened scene tears every stage down at once, and no stage-set notice
    // follows for the stages that disappear. Same registrations LayerDatabase uses.
    _callbackIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeNew, onSceneReset, this));
    _callbackIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, onSceneReset, this));
}

RenderLayerStageTracker::~RenderLayerStageTracker()
{
    if (_stageSetKey.IsValid()) {
        TfNotice::Revoke(_stageSetKey);
    }

    for (unsigned int i = 0; i < _callbackIds.length(); ++i) {
        MSceneMessage::removeCallback(_callbackIds[i]);
    }
    _callbackIds.clear();
}

/* static */
void RenderLayerStageTracker::initialize()
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    if (_instance) {
        return;
    }
    _instance = new RenderLayerStageTracker();

    // Stages may already exist, for instance when the plugin is loaded into a scene that
    // is already populated.
    _instance->reconcile();
#endif
}

/* static */
void RenderLayerStageTracker::finalize()
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    if (!_instance) {
        return;
    }
    _instance->detachAll();
    delete _instance;
    _instance = nullptr;
#endif
}

void RenderLayerStageTracker::onStageSet(const MayaUsdProxyStageSetNotice& notice)
{
    // The notice identifies one proxy shape, but a single stage change can invalidate
    // others, so the whole set is reconciled rather than just the sender's stage.
    (void)notice;
    reconcile();
}

/* static */
void RenderLayerStageTracker::onSceneReset(void* clientData)
{
    if (auto* self = static_cast<RenderLayerStageTracker*>(clientData)) {
        self->detachAll();
    }
}

void RenderLayerStageTracker::reconcile()
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    // Proxy shapes recompute re-entrantly while a stage is being set; act only on the
    // outermost call, as MayaStagesSubject::setupListeners does.
    if (MayaUsdProxyShapeBase::in_compute > 1) {
        return;
    }

    auto& manager = AdskUsdRenderSetup::RenderLayerManager::instance();

    const std::vector<UsdStageRefPtr> liveStages
        = MayaUsd::ufe::ProxyShapeHandler::getAllStages();

    std::set<UsdStageWeakPtr> live;
    for (const auto& stage : liveStages) {
        if (stage) {
            live.insert(UsdStageWeakPtr(stage));
        }
    }

    for (auto it = _attached.begin(); it != _attached.end();) {
        if (!*it) {
            // Destroyed without passing through here. The manager drops its own entry
            // when it prunes expired stages, so only our bookkeeping needs cleaning.
            it = _attached.erase(it);
        } else if (live.count(*it) == 0) {
            manager.detach(UsdStageRefPtr(*it));
            it = _attached.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& stage : liveStages) {
        if (stage && _attached.insert(UsdStageWeakPtr(stage)).second) {
            manager.attach(stage);
        }
    }
#endif
}

void RenderLayerStageTracker::detachAll()
{
#if defined(ADSK_ABI) && ADSK_ABI >= 2027
    auto& manager = AdskUsdRenderSetup::RenderLayerManager::instance();
    for (const auto& stage : _attached) {
        if (stage) {
            manager.detach(UsdStageRefPtr(stage));
        }
    }
#endif
    _attached.clear();
}

} // namespace MayaUsdRenderSetup
