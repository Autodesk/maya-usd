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

#ifndef MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERSTAGETRACKER_H
#define MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERSTAGETRACKER_H

#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsdUI/ui/api.h>

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MCallbackIdArray.h>

#include <set>

namespace MayaUsdRenderSetup {

//! Keeps the render layer manager attached to exactly the stages the proxy shapes
//! currently own, so render layers follow a stage through creation, recreation and
//! deletion.
//!
//! Attachment is reconciled as a set rather than driven by individual notices.
//! MayaUsdProxyStageInvalidateNotice cannot report the outgoing stage - GetStage() calls
//! getUsdStage(), which triggers a compute, and the notice is sent during dirty
//! propagation - so the outgoing stage is found by diffing against the live set instead.
//!
//! Does nothing unless the render layer API is available.
class MAYAUSD_UI_PUBLIC RenderLayerStageTracker : public PXR_NS::TfWeakBase
{
public:
    //! Starts tracking. Attaches any stage that already exists.
    static void initialize();

    //! Detaches every attached stage and stops tracking.
    static void finalize();

private:
    RenderLayerStageTracker();
    ~RenderLayerStageTracker();

    RenderLayerStageTracker(const RenderLayerStageTracker&) = delete;
    RenderLayerStageTracker& operator=(const RenderLayerStageTracker&) = delete;

    void onStageSet(const PXR_NS::MayaUsdProxyStageSetNotice& notice);

    //! Attaches stages that appeared and detaches those that went away.
    void reconcile();

    void detachAll();

    static void onSceneReset(void* clientData);

    std::set<PXR_NS::UsdStageWeakPtr> _attached;
    PXR_NS::TfNotice::Key             _stageSetKey;
    MCallbackIdArray                  _callbackIds;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERSTAGETRACKER_H
