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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERHANDLER_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERHANDLER_H

#include <mayaUsdUI/ui/api.h>

#include <pxr/usd/usd/stage.h>

#include <AdskUsdRenderSetup/Host.h>
#include <AdskUsdRenderSetup/IRenderHandler.h>

#include <string>
#include <string_view>

namespace AdskUsdRenderSetup {
class IRenderResult;
}

namespace MayaUsdRenderSetup {

//! MayaUSD concrete class implementation of
//! AdskUsdRenderSetup::IRenderHandler for Maya batch render of the active render target.
class MAYAUSD_UI_PUBLIC MayaBatchRenderHandler : public AdskUsdRenderSetup::IRenderHandler
{
public:
    inline static constexpr std::string_view kName = "BatchRender";

    ~MayaBatchRenderHandler() = default;

    std::string                           name() const override { return std::string(kName); }
    std::string                           displayName() const override;
    std::string                           toolTip() const override;
    AdskUsdRenderSetup::RenderHandlerIcon icon() const override
    {
        return AdskUsdRenderSetup::RenderHandlerIcon::BatchRender;
    }

    bool isAsync() const override { return true; }

    std::shared_ptr<AdskUsdRenderSetup::IRenderResult> render() const override;

    //! "Batch render <name>" for the active render layer of `desc`'s stage, else for the render
    //! setting `desc` points at; plain "Batch render" when there is neither.
    static std::string displayNameFor(const AdskUsdRenderSetup::RenderDescription& desc);

    //! Render flags passing the active render layer of `stage` to MayaHydra as its session
    //! layer, escaped for a MEL string literal; empty when there is none, it has no file, or its
    //! path cannot be passed in a MEL string.
    static std::string sessionLayerRenderOptions(const PXR_NS::UsdStageRefPtr& stage);
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERHANDLER_H
