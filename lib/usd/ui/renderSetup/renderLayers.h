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

#ifndef MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERS_H
#define MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERS_H

#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsdUI/ui/api.h>

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>

namespace AdskUsdRenderSetup {
class ActiveRenderLayerChangedNotice;
}

namespace MayaUsdRenderSetup {

//! Keeps the render layer manager tracking the stage of every proxy shape ("attaching"). Stages are
//! never detached: the manager drops the ones that were destroyed. It also registers the render
//! layer callbacks of the layer editor.
class MAYAUSD_UI_PUBLIC RenderLayers : public PXR_NS::TfWeakBase
{
public:
    static void initialize();
    static void finalize();

    RenderLayers();
    ~RenderLayers();

    RenderLayers(const RenderLayers&) = delete;
    RenderLayers& operator=(const RenderLayers&) = delete;

private:
    void onStagesChanged(const PXR_NS::MayaUsdStagesChangedNotice& notice);

    void onActiveRenderLayerChanged(
        const AdskUsdRenderSetup::ActiveRenderLayerChangedNotice& notice);

    void attachStages();

    PXR_NS::TfNotice::Key _stagesChangedKey;
    PXR_NS::TfNotice::Key _activeRenderLayerChangedKey;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERS_H
