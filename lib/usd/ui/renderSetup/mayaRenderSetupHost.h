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

#ifdef MAYA_HAS_USD_SETTINGS_NODES
#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/notice.h>

#include <unordered_map>
#endif

namespace MayaUsdRenderSetup {

//! MayaUSD implementation of AdskUsdRenderSetup::Host for the Render Setup UI.
//! Reports Maya's current frame, playback range and cameras, persists the
//! active render description on the UsdDefaultRenderDescription node, and
//! routes prim deletion and renaming through UFE.
class MayaRenderSetupHost
    : public AdskUsdRenderSetup::Host
#ifdef MAYA_HAS_USD_SETTINGS_NODES
    , public PXR_NS::TfWeakBase
#endif
{
public:
#ifdef MAYA_HAS_USD_SETTINGS_NODES
    MayaRenderSetupHost();
    ~MayaRenderSetupHost() override;
#endif

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

    //! \return Maya DAG cameras, plus every proxy shape's cameras grouped by
    //!         proxy shape name when \p editedStage is the render description
    //!         stage.
    std::vector<AdskUsdRenderSetup::ExternalCamera>
    externalCameras(const PXR_NS::UsdStageRefPtr& editedStage) const override;

#ifdef MAYA_HAS_USD_SETTINGS_NODES
private:
    //! Drops \p notice's stage from the camera cache, but only on a resync:
    //! an info-only change cannot alter which prims are cameras.
    void onObjectsChanged(const PXR_NS::UsdNotice::ObjectsChanged& notice);

    PXR_NS::TfNotice::Key _objectsChangedKey;

    //! Camera prim paths per stage, filled on demand by externalCameras() and
    //! pruned there to the stages that still exist.
    mutable std::unordered_map<PXR_NS::UsdStageWeakPtr, PXR_NS::SdfPathVector, PXR_NS::TfHash>
        _cameraPathsByStage;
#endif
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYARENDERSETUPHOST_H
