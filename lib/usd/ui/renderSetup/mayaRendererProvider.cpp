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

#include "mayaRendererProvider.h"

#ifdef MAYA_HAS_USD_SETTINGS_NODES
#include <mayaUsd/nodes/sceneRenderDescription.h>
#endif

#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>

#include <maya/MCommandResult.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MStringArray.h>

namespace MayaUsdRenderSetup {

namespace {

//! \return the label Maya's own Render Settings dialog shows for `rendererName`.
MString rendererUIName(const MString& rendererName)
{
    MString cmd;
    cmd.format("renderer -query -rendererUIName \"^1s\"", rendererName);

    MString uiName;
    MStatus status = MGlobal::executeCommand(cmd, uiName);
    if (status != MS::kSuccess)
        return rendererName;

    if (uiName.length() == 0)
        return rendererName;

    return uiName;
}

bool isHydraCapable(const std::string& rendererName)
{
    MString cmd;
    cmd.format("renderer -query -capability \"isHydra\" \"^1s\"", MString(rendererName.c_str()));

    // Renderers that don't report the "isHydra" capability at all return no
    // value (MCommandResult::kInvalid) rather than a boolean false.
    MCommandResult result;
    MStatus        status = MGlobal::executeCommand(cmd, result);
    if (!status) {
        MGlobal::displayWarning(
            MString("\"isHydra\" capability query failed for renderer \"") + rendererName.c_str()
            + "\": " + status.errorString());
        return false;
    }

    if (result.resultType() != MCommandResult::kString)
        return false;

    MString value;
    result.getResult(value);
    return value == "true" || value == "1";
}

bool isHydraRendererAvailable(
    PXR_NS::HdRendererPluginRegistry& registry,
    const PXR_NS::TfToken&            rendererId)
{
    PXR_NS::HdRendererPlugin* plugin = registry.GetRendererPlugin(rendererId);
    if (!plugin)
        return false;

    // As of 22.02, this needs to be called for Storm
    if (rendererId == PXR_NS::TfToken("HdStormRendererPlugin")) {
        PXR_NS::GlfContextCaps::InitInstance();
    }

    if (!plugin->IsSupported())
        return false;

    PXR_NS::HdRenderDelegate* delegate = plugin->CreateRenderDelegate();
    if (!delegate)
        return false;

    // We only needed the delegate to check if it would work.
    plugin->DeleteRenderDelegate(delegate);

    return true;
}

std::map<std::string, AdskUsdRenderSetup::RendererInfo> getRenderersMap()
{
    std::map<std::string, AdskUsdRenderSetup::RendererInfo> renderers;

    {
        MStringArray mayaRenderers;
        MStatus      status
            = MGlobal::executeCommand("renderer -query -namesOfAvailableRenderers", mayaRenderers);
        if (!status) {
            MGlobal::displayWarning("Unable to retrieve available renderers.");
        } else {
            for (const auto& rendererName : mayaRenderers) {
                AdskUsdRenderSetup::RendererInfo info;
                info.name = rendererName.asChar();
                info.displayName = rendererUIName(rendererName).asChar();
                info.isHydra = isHydraCapable(info.name);
                renderers[info.name] = std::move(info);
            }
        }
    }

    {
        PXR_NS::HdRendererPluginRegistry& registry
            = PXR_NS::HdRendererPluginRegistry::GetInstance();
        std::vector<PXR_NS::HfPluginDesc> hydraDescs;
        registry.GetPluginDescs(&hydraDescs);

        for (const auto& desc : hydraDescs) {
            const PXR_NS::TfToken rendererId = desc.id;
            if (!isHydraRendererAvailable(registry, rendererId)) {
                continue;
            }

            // Note: some Hydra renderers may also be registered in Maya's legacy renderer registry,
            //       so we need to merge the two lists.
            //
            //       The display-name registered with Maya is usually better than the one registered
            //       with Hydra, so we prefer that one if it exists. In that case we only update the
            //       isHydra flag, since the `isHydraCapable` query might not be reliable for some
            //       renderers.
            const std::string name = rendererId.GetText();
            auto              it = renderers.find(name);
            if (it != renderers.end()) {
                it->second.isHydra = true;
            } else {
                renderers[name] = AdskUsdRenderSetup::RendererInfo { name, desc.displayName, true };
            }
        }
    }

    return renderers;
}

} // namespace

std::vector<AdskUsdRenderSetup::RendererInfo> MayaRendererProvider::availableRenderers() const
{
    std::vector<AdskUsdRenderSetup::RendererInfo> renderers;

    auto renderersMap = getRenderersMap();
    renderers.reserve(renderersMap.size());
    for (const auto& pair : renderersMap) {
        renderers.push_back(pair.second);
    }

    return renderers;
}

std::string MayaRendererProvider::currentRenderer() const
{
    MSelectionList slist;
    slist.add("defaultRenderGlobals");
    MObject defaultRenderGlobalsObj;
    if (slist.length() > 0 && slist.getDependNode(0, defaultRenderGlobalsObj)) {
        MFnDependencyNode depNode(defaultRenderGlobalsObj);
        MPlug             currentRendererPlug = depNode.findPlug("currentRenderer", false);
        if (!currentRendererPlug.isNull()) {
            MString value;
            currentRendererPlug.getValue(value);
            if (value.length() > 0) {
                return value.asChar();
            }
        }
    }

#ifdef MAYA_HAS_USD_SETTINGS_NODES
    std::string hydraRenderer = MAYAUSD_NS_DEF::SceneRenderDescription::getCurrentRenderer();
    if (!hydraRenderer.empty()) {
        return hydraRenderer;
    }
#endif

    return std::string();
}

void MayaRendererProvider::switchRenderer(const std::string& next)
{
    // TODO: should we allow setting an empty renderer?
    if (next.empty()) {
        return;
    }

    // This validates that the renderer is available and avoid
    // using some arbitrary string in the MEL command.
    {
        const auto avail = getRenderersMap();
        const auto it = avail.find(next);
        if (it == avail.end())
            return;

#ifdef MAYA_HAS_USD_SETTINGS_NODES
        if (it->second.isHydra) {
            MAYAUSD_NS_DEF::SceneRenderDescription::setCurrentRenderer(next);
        } else {
            // TODO: do we need to clear the SceneRenderDescription current renderer when the
            // renderer is switched to a legacy renderer?
            MAYAUSD_NS_DEF::SceneRenderDescription::setCurrentRenderer("");
        }
#endif
    }

    // Note: other code watch the "currentRenderer" attribute of the defaultRenderGlobals node,
    //       so we change it last after we already updated the `SceneRenderDescription` current
    //       renderer.
    MString cmd;
    cmd.format("setCurrentRenderer(\"^1s\")", MString(next.c_str()));
    MGlobal::executeCommand(cmd);
}

} // namespace MayaUsdRenderSetup
