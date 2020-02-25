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
constexpr auto MTOH_DEFAULT_RENDERER_PLUGIN_NAME =
    "MTOH_DEFAULT_RENDERER_PLUGIN";
}

std::string MtohGetRendererPluginDisplayName(const TfToken& id) {
    HfPluginDesc pluginDesc;
    if (!TF_VERIFY(HdRendererPluginRegistry::GetInstance().GetPluginDesc(
            id, &pluginDesc))) {
        return {};
    }

    return pluginDesc.displayName;
}

MtohRendererInitialization MtohInitializeRenderPlugins() {
    using Storage = std::pair<MtohRendererDescriptionVector, MtohRendererSettingsVector>;
    static const Storage ret = []() -> Storage {
        HdRendererPluginRegistry& pluginRegistry = HdRendererPluginRegistry::GetInstance();
        HfPluginDescVector pluginDescs;
        pluginRegistry.GetPluginDescs(&pluginDescs);

        Storage store;
        store.first.reserve(pluginDescs.size());
        store.second.reserve(pluginDescs.size());

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
            if (delegate) {
                store.first.emplace_back(
                    renderer,
                    TfToken(
                        TfStringPrintf("mtohRenderOverride_%s", renderer.GetText())
                    ),
                    TfToken(
                        TfStringPrintf("%s (Hydra)", pluginDesc.displayName.c_str())
                    )
                );

                store.second.emplace_back(
                    std::move(delegate->GetRenderSettingDescriptors()));

                plugin->DeleteRenderDelegate(delegate);
            }
            // XXX: No 'delete plugin', should plugin be cached as well?
        }
        // Make sure the static's size doesn't have any extra overhead
        store.first.shrink_to_fit();
        store.second.shrink_to_fit();
        return store;
    }();
    return ret;
}

const MtohRendererDescriptionVector& MtohGetRendererDescriptions() {
    return MtohInitializeRenderPlugins().first;
}

TfTokenVector MtohGetRendererPlugins() {
    // This is returning a copy, so just build it up
    TfTokenVector ret;
    for (auto& plugin : MtohGetRendererDescriptions())
        ret.emplace_back(plugin.rendererName);
    return ret;
}

TfToken MtohGetDefaultRenderer() {
    const MtohRendererDescriptionVector& plugins = MtohGetRendererDescriptions();
    if (plugins.empty())
        return {};

    const auto* defaultRenderer = getenv(MTOH_DEFAULT_RENDERER_PLUGIN_NAME);
    if (defaultRenderer == nullptr) {
        // FIXME: Why have two of these ?
        // In the case of MTOH_DEFAULT_RENDERER_PLUGIN_NAME, HD_DEFAULT_RENDERER
        // will still be created once during UsdImagingGL initialization
        // Give the UsdImagingGL env-variable a try ...
        defaultRenderer = getenv("HD_DEFAULT_RENDERER");
        if (defaultRenderer == nullptr)
            return plugins.front().rendererName;
    }

    const TfToken defaultRendererToken(defaultRenderer);
    auto it = std::find_if(plugins.begin(), plugins.end(),
        [&](const MtohRendererDescription& desc) -> bool {
            return desc.rendererName == defaultRendererToken;
        });
    return it == plugins.end() ? plugins.front().rendererName : defaultRendererToken;
}


PXR_NAMESPACE_CLOSE_SCOPE
