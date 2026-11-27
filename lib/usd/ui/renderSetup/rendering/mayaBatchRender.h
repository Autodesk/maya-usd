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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDER_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDER_H

#include <pxr/usd/usd/stage.h>

#include <maya/MString.h>

#include <AdskUsdRenderSetup/RenderLayerInfo.h>

#include <string>

// Helpers shared by the Maya batch render handlers.
namespace MayaUsdRenderSetup {

//! Offers to save the scene when a layer used by `stage` is dirty, since the batch process
//! renders the saved files. Returns false when the user cancels.
bool saveForBatchRender(const PXR_NS::UsdStageRefPtr& stage);

//! The batchRender command for the current renderer, with `extraOptions` appended to the Render
//! flags. It renders `sceneFile` when given, otherwise a copy of the scene Maya writes for it.
MString batchRenderCommand(const std::string& extraOptions, const std::string& sceneFile = {});

//! Render flag making the batch process render the pass of `renderLayer` of `stage`, escaped for
//! a MEL string literal; empty when the layer has no pass or the non-null `stage` has no UFE
//! path in the scene.
std::string activeRenderDescriptionOptions(
    const PXR_NS::UsdStageRefPtr&              stage,
    const AdskUsdRenderSetup::RenderLayerInfo& renderLayer);

//! Generic file path of the layer of `renderLayer`, opening it when it is not loaded; empty
//! when the layer is anonymous or cannot be opened.
std::string sessionLayerPath(const AdskUsdRenderSetup::RenderLayerInfo& renderLayer);

//! Whether `text` has no double quote, which would end the MEL string literal it is passed in.
bool canPassInMelString(const std::string& text);

//! Render flags passing the layer at `layerPath` to MayaHydra as the session layer of `stage`,
//! escaped for a MEL string literal. `layerPath` must be non-empty and pass canPassInMelString.
std::string sessionLayerOptions(const PXR_NS::UsdStageRefPtr& stage, const std::string& layerPath);

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDER_H
