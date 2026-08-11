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

#ifndef MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERSAVEPROVIDER_H
#define MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERSAVEPROVIDER_H

#include <mayaUsd/utils/utilSerialization.h>
#include <mayaUsdUI/ui/api.h>

namespace MayaUsdRenderSetup {

//! Lets the layer editor save flow see render layers that the stage's layer stack does
//! not expose, and keeps the render layer registry pointing at the right identifier once
//! an anonymous render layer has been written to disk.
//!
//! Every method is a no-op unless the render layer API is available, so a build without
//! it leaves the save flow untouched.
class MAYAUSD_UI_PUBLIC RenderLayerSaveProvider : public MayaUsd::utils::RenderLayerSaveProvider
{
public:
    //! Registers the process-wide instance with the save flow.
    static void initialize();

    //! Unregisters it. Safe to call when initialize() was never called.
    static void finalize();

    void getRenderLayersToSave(
        const std::string&            proxyPath,
        const PXR_NS::UsdStageRefPtr& stage,
        MayaUsd::utils::StageLayersToSave& layersInfo) override;

    void onRenderLayerSaved(
        const PXR_NS::UsdStageRefPtr& stage,
        const std::string&            renderLayerName,
        const std::string&            newIdentifier) override;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_RENDERLAYERSAVEPROVIDER_H
