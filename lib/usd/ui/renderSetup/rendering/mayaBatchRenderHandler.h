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

#include <AdskUsdRenderSetup/IRenderHandler.h>

#include <string>
#include <string_view>

namespace AdskUsdRenderSetup {
class IRenderResult;
}

namespace MayaUsdRenderSetup {

//! MayaUSD concrete class implementation of
//! AdskUsdRenderSetup::IRenderHandler for Maya batch render.
class MAYAUSD_UI_PUBLIC MayaBatchRenderHandler : public AdskUsdRenderSetup::IRenderHandler
{
public:
    inline static constexpr std::string_view kName = "BatchRender";
    inline static constexpr std::string_view kLabel = "Batch Render";

    ~MayaBatchRenderHandler() = default;

    std::string name() const override { return std::string(kName); }
    std::string displayName() const override { return std::string(kLabel); }

    bool isAsync() const override { return true; }

    std::shared_ptr<AdskUsdRenderSetup::IRenderResult> render() const override;

    //! Render flags passing the active render layer of `stage` to MayaHydra as its session
    //! layer, escaped for a MEL string literal; empty when there is none to pass.
    static std::string sessionLayerRenderOptions(const PXR_NS::UsdStageRefPtr& stage);
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERHANDLER_H
