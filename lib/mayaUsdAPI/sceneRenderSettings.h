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
#ifndef MAYAUSDAPI_SCENE_RENDER_SETTINGS_H
#define MAYAUSDAPI_SCENE_RENDER_SETTINGS_H

#include <mayaUsdAPI/api.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

namespace MAYAUSDAPI_NS_DEF {

//! Public accessors for the process-wide render-settings singleton
//! (UsdDefaultRenderSettings).
namespace SceneRenderSettings {

//! \return the USD stage backing the render-settings singleton.
MAYAUSD_API_PUBLIC PXR_NS::UsdStageRefPtr getUsdStage();

//! \return the name of the custom attribute holding the UFE path string of a
//!         camera that lives outside the render-settings stage.
MAYAUSD_API_PUBLIC const PXR_NS::TfToken& externalCameraAttrName();

} // namespace SceneRenderSettings

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_SCENE_RENDER_SETTINGS_H
