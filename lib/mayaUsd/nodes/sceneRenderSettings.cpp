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

// Registration site and public C++ surface for the UsdDefaultRenderSettings
// settings node. Node type, callbacks and serialization are handled
// generically by UsdSettingsNode and UsdSceneSettingsManager.

#include <mayaUsd/nodes/sceneRenderSettings.h>
#include <mayaUsd/nodes/usdSceneSettingsManager.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdRender/settings.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

#include <string>

namespace {

// Locked DG node name and UsdSceneSettingsManager lookup key.
const std::string kRenderSettingsNodeName("UsdDefaultRenderSettings");

// Stage metadata key for the SdfPath of the default render-settings prim.
const PXR_NS::TfToken kRenderSettingsPrimPathToken("renderSettingsPrimPath");

// Stage metadata key for the UFE path of the currently active settings prim.
const PXR_NS::TfToken kActiveSettingsPathToken("activeSettingsPath");

// Static initializer ensures registration happens before
// UsdSceneSettingsManager::onPluginInitialize(). When the plugin is already
// running (sub-plugin load), registerSettingNode() creates the node directly.
// NOLINTNEXTLINE(cert-err58-cpp) -- intentional static initializer pattern
const bool kRenderSettingsRegistered = []() {
    MayaUsd::UsdSceneSettingsManager::registerSettingNode(
        kRenderSettingsNodeName, [](PXR_NS::UsdStageRefPtr stage) {
            if (!stage) {
                return;
            }

            // /Render scope per UsdRender conventions:
            // https://openusd.org/dev/api/usd_render_page_front.html
            const PXR_NS::SdfPath renderScopePath("/Render");
            const PXR_NS::SdfPath renderSettingsPath("/Render/SceneRenderSettings");

            PXR_NS::UsdGeomScope::Define(stage, renderScopePath);

            // Leave attributes un-authored so they fall back to schema defaults.
            PXR_NS::UsdRenderSettings renderSettings
                = PXR_NS::UsdRenderSettings::Define(stage, renderSettingsPath);
            if (!renderSettings) {
                return;
            }

            stage->SetMetadata(kRenderSettingsPrimPathToken, renderSettingsPath.GetString());

            // Comma is the segment separator used by Ufe::PathString.
            const std::string defaultActivePath
                = kRenderSettingsNodeName + "," + renderSettingsPath.GetString();
            stage->SetMetadata(kActiveSettingsPathToken, defaultActivePath);
        });
    return true;
}();

} // namespace

namespace MAYAUSD_NS_DEF {
namespace SceneRenderSettings {

std::string find()
{
    MObject obj = MayaUsd::UsdSceneSettingsManager::find(kRenderSettingsNodeName);
    if (obj.isNull()) {
        return {};
    }
    MFnDependencyNode depFn(obj);
    return depFn.name().asChar();
}

PXR_NS::UsdStageRefPtr getUsdStage()
{
    return MayaUsd::UsdSceneSettingsManager::getStage(kRenderSettingsNodeName);
}

PXR_NS::UsdPrim getDefaultRenderSettingsPrim()
{
    auto stage = MayaUsd::UsdSceneSettingsManager::getStage(kRenderSettingsNodeName);
    if (!stage) {
        return {};
    }
    std::string path;
    stage->GetMetadata(kRenderSettingsPrimPathToken, &path);
    if (path.empty()) {
        return {};
    }
    return stage->GetPrimAtPath(PXR_NS::SdfPath(path));
}

std::string getActiveSettingPath()
{
    auto stage = MayaUsd::UsdSceneSettingsManager::getStage(kRenderSettingsNodeName);
    if (!stage) {
        return {};
    }
    std::string path;
    stage->GetMetadata(kActiveSettingsPathToken, &path);
    return path;
}

bool setActiveSettingPath(const std::string& ufePath)
{
    auto stage = MayaUsd::UsdSceneSettingsManager::getStage(kRenderSettingsNodeName);
    if (!stage) {
        return false;
    }
    return stage->SetMetadata(kActiveSettingsPathToken, ufePath);
}

} // namespace SceneRenderSettings
} // namespace MAYAUSD_NS_DEF
