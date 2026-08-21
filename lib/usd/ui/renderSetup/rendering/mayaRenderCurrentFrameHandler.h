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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYARENDERCURRENTFRAMEHANDLER_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYARENDERCURRENTFRAMEHANDLER_H

#include <AdskUsdRenderSetup/IRenderHandler.h>

#include <string_view>

namespace AdskUsdRenderSetup {
class IRenderResult;
}

namespace MayaUsdRenderSetup {

//! MayaUSD concrete class implementation of AdskUsdRenderSetup::IRenderHandler
//! for Maya render current frame.
class MayaRenderCurrentFrameHandler : public AdskUsdRenderSetup::IRenderHandler
{
public:
    inline static constexpr std::string_view kName = "Render";
    inline static constexpr std::string_view kLabel = "Render";

    ~MayaRenderCurrentFrameHandler() = default;

    std::string name() const override { return std::string(kName); }
    std::string label() const override { return std::string(kLabel); }

    bool isAsync() const override { return false; }

    std::shared_ptr<AdskUsdRenderSetup::IRenderResult> render() const override;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYARENDERCURRENTFRAMEHANDLER_H
