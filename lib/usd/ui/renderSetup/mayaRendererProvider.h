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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYARENDERERPROVIDER_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYARENDERERPROVIDER_H

#include <AdskUsdRenderSetup/IRendererProvider.h>

namespace MayaUsdRenderSetup {

//! MayaUSD implementation of AdskUsdRenderSetup::IRendererProvider for the Render Setup UI.
//! Lists every renderer Maya knows about and reads/writes Maya's global current renderer
//! (defaultRenderGlobals.currentRenderer).
class MayaRendererProvider : public AdskUsdRenderSetup::IRendererProvider
{
public:
    //! \return every renderer Maya currently knows about, Hydra-capable or not.
    std::vector<AdskUsdRenderSetup::RendererInfo> availableRenderers() const override;

    //! \return Maya's current renderer name (defaultRenderGlobals.currentRenderer).
    std::string currentRenderer() const override;

protected:
    //! Switches Maya's current renderer via the setCurrentRenderer MEL proc.
    void switchRenderer(const std::string& next) override;

private:
    //! \return true if `rendererName` reports the "isHydra" capability.
    bool isHydraCapable(const std::string& rendererName) const;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYARENDERERPROVIDER_H
