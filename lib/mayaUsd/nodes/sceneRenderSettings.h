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
#ifndef MAYAUSD_NODES_SCENE_RENDER_SETTINGS_H
#define MAYAUSD_NODES_SCENE_RENDER_SETTINGS_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <string>

namespace MAYAUSD_NS_DEF {

//! Public C++ surface for the UsdDefaultRenderSettings singleton, also bound
//! by the Python wrapper.
namespace SceneRenderSettings {

//! DG name of the singleton, or an empty string if not yet created.
MAYAUSD_CORE_PUBLIC std::string find();

//! Singleton's USD stage, materialized on first call.
MAYAUSD_CORE_PUBLIC PXR_NS::UsdStageRefPtr getUsdStage();

//! Prim at the path published in the stage's `renderSettingsPrimPath`
//! metadata, or an invalid prim if unavailable.
MAYAUSD_CORE_PUBLIC PXR_NS::UsdPrim getDefaultRenderSettingsPrim();

//! UFE path of the currently active settings prim, or an empty string if
//! unavailable.
MAYAUSD_CORE_PUBLIC std::string getActiveSettingPath();

//! Author the UFE path of the currently active settings prim. Returns false
//! if the singleton node is unavailable or the plug write fails.
MAYAUSD_CORE_PUBLIC bool setActiveSettingPath(const std::string& ufePath);

//! Name of the custom string attribute that records the UFE path of a camera
//! that does not live on the same stage as the render-settings prim
//! (e.g. a Maya native camera shape, or a camera in another USD stage).
//! When this attribute is authored, it overrides the schema `camera`
//! relationship for the purpose of selecting the active render camera.
MAYAUSD_CORE_PUBLIC const PXR_NS::TfToken& externalCameraAttrName();

//! Set the active camera for a UsdRenderSettings prim from a UFE path string.
//!
//! Resolution rule:
//! - If `cameraUfePath` resolves to a UsdGeomCamera prim that lives on the
//!   same stage as `renderSettings`, the schema `camera` relationship is set
//!   to that prim and any previously authored `adskUsd:externalCamera`
//!   attribute is removed.
//! - Otherwise (Maya native camera, or a camera prim in a different stage),
//!   the schema `camera` relationship targets are cleared and the UFE path
//!   string is stored on the `adskUsd:externalCamera` attribute.
//!
//! \param renderSettings A valid prim of type UsdRenderSettings.
//! \param cameraUfePath  UFE path string parseable by Ufe::PathString::path.
//! \return true on success; false if the prim is invalid, not a
//!         UsdRenderSettings, or the authoring operation fails.
MAYAUSD_CORE_PUBLIC bool
setRenderSettingsCamera(const PXR_NS::UsdPrim& renderSettings, const std::string& cameraUfePath);

} // namespace SceneRenderSettings

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_NODES_SCENE_RENDER_SETTINGS_H
