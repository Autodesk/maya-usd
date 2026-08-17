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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYARENDERSETUPHOST_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYARENDERSETUPHOST_H

#include <AdskUsdRenderSetup/Host.h>

namespace MayaUsdRenderSetup {

//! MayaUSD implementation of AdskUsdRenderSetup::Host for the Render Setup UI.
//! Reports the current frame and playback range from Maya's animation control,
//! so the adsk:frames widget reflects the active scene timeline.
class MayaRenderSetupHost : public AdskUsdRenderSetup::Host
{
public:
    //! \return Maya's current time, in UI units (frames).
    double currentFrame() const override;

    //! \return Maya's playback range (time slider min/max), in UI units (frames).
    AdskUsdRenderSetup::FrameRange timelineRange() const override;

    //! \return \p logicalPixels scaled by Maya's UI DPI factor.
    int dpiScaled(int logicalPixels) const override;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYARENDERSETUPHOST_H
