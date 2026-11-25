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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERALLHANDLER_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERALLHANDLER_H

#include <mayaUsdUI/ui/api.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MString.h>

#include <AdskUsdRenderSetup/IRenderHandler.h>

#include <string>
#include <string_view>
#include <vector>

namespace AdskUsdRenderSetup {
class IRenderResult;
}

namespace MayaUsdRenderSetup {

//! MayaUSD concrete class implementation of AdskUsdRenderSetup::IRenderHandler that batch
//! renders, one after another, every renderable render layer of the active render target's stage.
class MAYAUSD_UI_PUBLIC MayaBatchRenderAllHandler : public AdskUsdRenderSetup::IRenderHandler
{
public:
    inline static constexpr std::string_view kName = "BatchRenderAll";

    ~MayaBatchRenderAllHandler() = default;

    std::string                           name() const override { return std::string(kName); }
    std::string                           displayName() const override;
    std::string                           toolTip() const override;
    AdskUsdRenderSetup::RenderHandlerIcon icon() const override
    {
        return AdskUsdRenderSetup::RenderHandlerIcon::BatchRenderAll;
    }

    bool isAsync() const override { return true; }

    std::shared_ptr<AdskUsdRenderSetup::IRenderResult> render() const override;

    //! One batchRender command per renderable render layer of `stage`, sorted by layer name.
    //! Layers without a file the batch process can open are skipped with a warning.
    static std::vector<MString> batchRenderCommands(const PXR_NS::UsdStageRefPtr& stage);
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERALLHANDLER_H
