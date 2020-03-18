//
// Copyright 2019 Luma Pictures
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
#include "utils.h"
#include "tokens.h"

#include <maya/MGlobal.h>

#if USD_VERSION_NUM >= 1911
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>
#else
#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>
PXR_NAMESPACE_OPEN_SCOPE
using HdRendererPluginRegistry = HdxRendererPluginRegistry;
PXR_NAMESPACE_CLOSE_SCOPE
#endif

#include <pxr/imaging/glf/contextCaps.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
constexpr auto _renderOverrideOptionBoxTemplate = R"mel(
global proc {{override}}OptionBox() {
    string $windowName = "{{override}}OptionsWindow";
    if (`window -exists $windowName`) {
        showWindow $windowName;
        return;
    }
    string $cc = "mtoh -updateRenderGlobals; refresh -f";

    mtoh -createRenderGlobals;

    window -title "Maya to Hydra Settings" "{{override}}OptionsWindow";
    scrollLayout;
    frameLayout -label "Hydra Settings";
    columnLayout;
    attrControlGrp -label "Enable Motion Samples" -attribute "defaultRenderGlobals.mtohEnableMotionSamples" -changeCommand $cc;
    attrControlGrp -label "Texture Memory Per Texture (KB)" -attribute "defaultRenderGlobals.mtohTextureMemoryPerTexture" -changeCommand $cc;
    attrControlGrp -label "OpenGL Selection Overlay" -attribute "defaultRenderGlobals.mtohSelectionOverlay" -changeCommand $cc;
    attrControlGrp -label "Show Wireframe on Selected Objects" -attribute "defaultRenderGlobals.mtohWireframeSelectionHighlight" -changeCommand $cc;
    attrControlGrp -label "Highlight Selected Objects" -attribute "defaultRenderGlobals.mtohColorSelectionHighlight" -changeCommand $cc;
    attrControlGrp -label "Highlight Color for Selected Objects" -attribute "defaultRenderGlobals.mtohColorSelectionHighlightColor" -changeCommand $cc;
    setParent ..;
    setParent ..;
    {{override}}Options();
    setParent ..;

    showWindow $windowName;
}
)mel";

bool _IsSupportedAttribute(const VtValue& v) {
    return v.IsHolding<bool>() || v.IsHolding<int>() || v.IsHolding<float>() ||
           v.IsHolding<GfVec4f>() || v.IsHolding<std::string>() ||
           v.IsHolding<TfEnum>();
}

static void _BuildOptionsMenu(const MtohRendererDescription& rendererDesc,
    const HdRenderSettingDescriptorList& rendererSettingDescriptors)
{
    const auto optionBoxCommand = TfStringReplace(
        _renderOverrideOptionBoxTemplate, "{{override}}",
        rendererDesc.overrideName.GetText());

    auto status = MGlobal::executeCommand(optionBoxCommand.c_str());
    if (!status) {
        TF_WARN(
            "Error in render override option box command function: \n%s",
            status.errorString().asChar());
    }

    std::stringstream ss;
    ss << "global proc " << rendererDesc.overrideName << "Options() {\n";
    ss << "\tstring $cc = \"mtoh -updateRenderGlobals; refresh -f\";\n";
    ss << "\tframeLayout -label \"" << rendererDesc.displayName
       << "Options\" -collapsable true;\n";
    ss << "\tcolumnLayout;\n";
    for (const auto& desc : rendererSettingDescriptors) {
        if (!_IsSupportedAttribute(desc.defaultValue))
            continue;

        const auto attrName = TfStringPrintf(
            "%s%s", rendererDesc.rendererName.GetText(),
            desc.key.GetText());
        ss << "\tattrControlGrp -label \"" << desc.name
           << "\" -attribute \"defaultRenderGlobals." << attrName
           << "\" -changeCommand $cc;\n";
    }
    if (rendererDesc.rendererName == MtohTokens->HdStormRendererPlugin) {
        ss << "\tattrControlGrp -label \"Maximum shadow map size"
           << "\" -attribute \"defaultRenderGlobals."
           << MtohTokens->mtohMaximumShadowMapResolution.GetString()
           << "\" -changeCommand $cc;\n";
    }
    ss << "\tsetParent ..;\n";
    ss << "\tsetParent ..;\n";
    ss << "}\n";

    const auto optionsCommand = ss.str();
    status = MGlobal::executeCommand(optionsCommand.c_str());
    if (!status) {
        TF_WARN(
            "Error in render delegate options function: \n%s",
            status.errorString().asChar());
    }
}

std::pair<const MtohRendererDescriptionVector&, const MtohRendererSettings&>
MtohInitializeRenderPlugins() {
    using Storage = std::pair<MtohRendererDescriptionVector, MtohRendererSettings>;

    static const Storage ret = []() -> Storage {
        HdRendererPluginRegistry& pluginRegistry = HdRendererPluginRegistry::GetInstance();
        HfPluginDescVector pluginDescs;
        pluginRegistry.GetPluginDescs(&pluginDescs);

        Storage store;
        store.first.reserve(pluginDescs.size());

        for (const auto& pluginDesc : pluginDescs) {
            const TfToken renderer = pluginDesc.id;
            HdRendererPlugin* plugin = pluginRegistry.GetRendererPlugin(renderer);
            if (!plugin)
                continue;

            // XXX: As of 22.02, this needs to be called for Storm
            if (pluginDesc.id == MtohTokens->HdStormRendererPlugin)
                GlfContextCaps::InitInstance();

            HdRenderDelegate* delegate = plugin->IsSupported() ?
                plugin->CreateRenderDelegate() : nullptr;

            // No 'delete plugin', should plugin be cached as well?
            if (!delegate)
                continue;

            auto& rendererSettingDescriptors = store.second.emplace(renderer,
                delegate->GetRenderSettingDescriptors()).first->second;

            // We only needed the delegate for the settings, so release
            plugin->DeleteRenderDelegate(delegate);
            // Null it out to make any possible usage later obv, wrong!
            delegate = nullptr;

            store.first.emplace_back(
                renderer,
                TfToken(
                    TfStringPrintf("mtohRenderOverride_%s", renderer.GetText())
                ),
                TfToken(
                    TfStringPrintf("%s (Hydra)", pluginDesc.displayName.c_str())
                )
            );

            _BuildOptionsMenu(store.first.back(), rendererSettingDescriptors);
        }

        // Make sure the static's size doesn't have any extra overhead
        store.first.shrink_to_fit();
        assert(store.first.size() == store.second.size());
        return store;
    }();
    return ret;
}

}

std::string MtohGetRendererPluginDisplayName(const TfToken& id) {
    HfPluginDesc pluginDesc;
    if (!TF_VERIFY(HdRendererPluginRegistry::GetInstance().GetPluginDesc(
            id, &pluginDesc))) {
        return {};
    }

    return pluginDesc.displayName;
}

const MtohRendererDescriptionVector& MtohGetRendererDescriptions() {
    return MtohInitializeRenderPlugins().first;
}

const MtohRendererSettings& MtohGetRendererSettings() {
    return MtohInitializeRenderPlugins().second;
}


PXR_NAMESPACE_CLOSE_SCOPE
