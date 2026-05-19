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

#include <mayaUsd/nodes/sceneRenderSettings.h>
#include <mayaUsd/nodes/usdSceneSettingsManager.h>
#include <mayaUsd/nodes/usdSettingsNode.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/relationship.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdRender/settings.h>

#include <ufe/path.h>
#include <ufe/pathString.h>

#include <string>

namespace {

const std::string     kRenderSettingsNodeName("UsdDefaultRenderSettings");
const PXR_NS::TfToken kRenderSettingsPrimPathToken("renderSettingsPrimPath");

// UFE path to a camera that lives outside renderSettings's stage (Maya native
// camera, or a camera in a different USD stage). Overrides the schema `camera`
// relationship for active-camera selection when authored.
const PXR_NS::TfToken kExternalCameraAttrName("adskUsd:externalCamera");

// Transform path (not shape) for end-user readability; Maya Hydra resolves to
// the underlying camera shape when required.
const std::string kDefaultExternalCameraUfePath("|persp");

// Avoids the AE default name-splitting of "adskUsd:externalCamera".
const std::string kExternalCameraDisplayName("External Camera");

PXR_NS::UsdAttribute createExternalCameraAttr(const PXR_NS::UsdPrim& renderSettings)
{
    PXR_NS::UsdAttribute attr = renderSettings.CreateAttribute(
        kExternalCameraAttrName, PXR_NS::SdfValueTypeNames->String, /*custom=*/true);
    if (attr) {
        attr.SetDisplayName(kExternalCameraDisplayName);
    }
    return attr;
}

// NOLINTNEXTLINE(cert-err58-cpp) -- intentional static initializer pattern
const bool kRenderSettingsRegistered = []() {
    MayaUsd::UsdSceneSettingsManager::registerSettingNode(
        kRenderSettingsNodeName, [](PXR_NS::UsdStageRefPtr stage, MayaUsd::UsdSettingsNode& node) {
            if (!stage) {
                return;
            }

            // /Render scope per UsdRender conventions:
            // https://openusd.org/dev/api/usd_render_page_front.html
            const PXR_NS::SdfPath renderScopePath("/Render");
            const PXR_NS::SdfPath renderSettingsPath("/Render/SceneRenderSettings");

            PXR_NS::UsdGeomScope::Define(stage, renderScopePath);

            PXR_NS::UsdRenderSettings renderSettings
                = PXR_NS::UsdRenderSettings::Define(stage, renderSettingsPath);
            if (!renderSettings) {
                return;
            }

            createExternalCameraAttr(renderSettings.GetPrim()).Set(kDefaultExternalCameraUfePath);

            stage->SetMetadata(kRenderSettingsPrimPathToken, renderSettingsPath.GetString());

            // Skip when non-empty so a pre-populator user value is preserved.
            if (node.activeSettingsPath().empty()) {
                node.setActiveSettingsPath(
                    kRenderSettingsNodeName + "," + renderSettingsPath.GetString());
            }
        });
    return true;
}();

} // namespace

namespace MAYAUSD_NS_DEF {
namespace SceneRenderSettings {

std::string find()
{
    MayaUsd::UsdSettingsNode* node
        = MayaUsd::UsdSceneSettingsManager::getNodeForNodeName(kRenderSettingsNodeName);
    return node ? node->nodeName() : std::string();
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
    MayaUsd::UsdSettingsNode* node
        = MayaUsd::UsdSceneSettingsManager::getNodeForNodeName(kRenderSettingsNodeName);
    if (!node) {
        return {};
    }
    // Force the populator to seed the default before reading.
    node->getUsdStage();
    return node->activeSettingsPath();
}

bool setActiveSettingPath(const std::string& ufePath)
{
    MayaUsd::UsdSettingsNode* node
        = MayaUsd::UsdSceneSettingsManager::getNodeForNodeName(kRenderSettingsNodeName);
    if (!node) {
        return false;
    }
    // Run the populator first so a later replay cannot overwrite this write.
    node->getUsdStage();
    return node->setActiveSettingsPath(ufePath);
}

const PXR_NS::TfToken& externalCameraAttrName() { return kExternalCameraAttrName; }

bool setRenderSettingsCamera(
    const PXR_NS::UsdPrim& renderSettings,
    const std::string&     cameraUfePath)
{
    if (!renderSettings || !renderSettings.IsA<PXR_NS::UsdRenderSettings>()) {
        return false;
    }

    // Try SdfPath against renderSettings's stage first (handles bare USD
    // paths and prims on stages not reachable through any UFE gateway), then
    // fall back to UFE for paths with a Maya gateway segment.
    PXR_NS::UsdPrim resolvedPrim;
    if (!cameraUfePath.empty()) {
        if (PXR_NS::SdfPath::IsValidPathString(cameraUfePath)) {
            const PXR_NS::SdfPath sdfPath(cameraUfePath);
            if (sdfPath.IsAbsolutePath() && sdfPath.IsPrimPath()) {
                resolvedPrim = renderSettings.GetStage()->GetPrimAtPath(sdfPath);
            }
        }
        if (!resolvedPrim) {
            try {
                const Ufe::Path ufePath = Ufe::PathString::path(cameraUfePath);
                resolvedPrim = MayaUsd::ufe::ufePathToPrim(ufePath);
            } catch (const std::exception&) {
            }
        }
    }

    PXR_NS::UsdRenderSettings settingsSchema(renderSettings);
    PXR_NS::UsdRelationship   cameraRel = settingsSchema.CreateCameraRel();

    const bool resolvesToSameStageCamera = resolvedPrim && resolvedPrim.IsA<PXR_NS::UsdGeomCamera>()
        && resolvedPrim.GetStage() == renderSettings.GetStage();

    if (resolvesToSameStageCamera) {
        if (!cameraRel.SetTargets({ resolvedPrim.GetPath() })) {
            return false;
        }
        if (renderSettings.HasAttribute(kExternalCameraAttrName)) {
            // RemoveProperty is non-const; UsdPrim is a value-type handle.
            PXR_NS::UsdPrim mutablePrim = renderSettings;
            mutablePrim.RemoveProperty(kExternalCameraAttrName);
        }
        return true;
    }

    cameraRel.SetTargets({});

    PXR_NS::UsdAttribute externalCameraAttr = createExternalCameraAttr(renderSettings);
    return externalCameraAttr && externalCameraAttr.Set(cameraUfePath);
}

} // namespace SceneRenderSettings
} // namespace MAYAUSD_NS_DEF
