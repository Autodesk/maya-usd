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
//! so the adsk:frames widget reflects the active scene timeline, persists
//! the active render description on the UsdDefaultRenderDescription node, and
//! routes prim deletion and renaming through UFE commands.
class MayaRenderSetupHost : public AdskUsdRenderSetup::Host
{
public:
    //! \return Maya's current time, in UI units (frames).
    double currentFrame() const override;

    //! \return Maya's playback range (time slider min/max), in UI units (frames).
    AdskUsdRenderSetup::FrameRange timelineRange() const override;

    //! \return \p logicalPixels scaled by Maya's UI DPI factor.
    int dpiScaled(int logicalPixels) const override;

    //! \return MayaUsd's prettify name.
    std::string prettifyName(const std::string& name) const override;

#ifdef MAYA_HAS_USD_SETTINGS_NODES
    //! \return The stage and prim named by the UsdDefaultRenderDescription node's
    //! activeRenderDescriptionPath, or an empty description if it does not resolve.
    AdskUsdRenderSetup::RenderDescription activeRenderDescription() const override;

    //! Author \p description as a UFE path string on the UsdDefaultRenderDescription
    //! node. Ignored if the stage has no UFE gateway node.
    void
    setActiveRenderDescription(const AdskUsdRenderSetup::RenderDescription& description) override;
#endif

    //! Deletes \p prim through UFE so the removal joins Maya's undo queue and
    //! notifies UFE observers. Falls back to the base implementation inside a
    //! Maya undo block for stages with no proxy shape.
    //! \return true when the delete was carried out.
    bool deletePrim(const PXR_NS::UsdPrim& prim) override;

    //! Renames \p prim through UFE, which applies Maya's own name
    //! sanitization and sibling uniquification, so the resulting name is
    //! often not \p newName verbatim. Falls back to the base implementation
    //! inside a Maya undo block for stages with no proxy shape.
    //! \return The prim's new path, or an empty path when the rename failed.
    PXR_NS::SdfPath renamePrim(const PXR_NS::UsdPrim& prim, const std::string& newName) override;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYARENDERSETUPHOST_H
