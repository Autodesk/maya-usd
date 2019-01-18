//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "utils.h"

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
constexpr auto MTOH_DEFAULT_RENDERER_PLUGIN_NAME =
    "MTOH_DEFAULT_RENDERER_PLUGIN";
}

TfTokenVector MtohGetRendererPlugins() {
    static const auto ret = []() -> TfTokenVector {
        HfPluginDescVector pluginDescs;
        HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

        TfTokenVector r;
        r.reserve(pluginDescs.size());
        for (const auto& desc : pluginDescs) { r.emplace_back(desc.id); }
        return r;
    }();

    return ret;
}

std::string MtohGetRendererPluginDisplayName(const TfToken& id) {
    HfPluginDesc pluginDesc;
    if (!TF_VERIFY(HdxRendererPluginRegistry::GetInstance().GetPluginDesc(
            id, &pluginDesc))) {
        return {};
    }

    return pluginDesc.displayName;
}

TfToken MtohGetDefaultRenderer() {
    const auto l = MtohGetRendererPlugins();
    if (l.empty()) { return {}; }
    const auto* defaultRenderer = getenv(MTOH_DEFAULT_RENDERER_PLUGIN_NAME);
    if (defaultRenderer == nullptr) { return l[0]; }
    const TfToken defaultRendererToken(defaultRenderer);
    if (std::find(l.begin(), l.end(), defaultRendererToken) != l.end()) {
        return defaultRendererToken;
    }
    return l[0];
}

const MtohRendererDescriptionVector& MtohGetRendererDescriptions() {
    static const auto ret = []() -> MtohRendererDescriptionVector {
        MtohRendererDescriptionVector r;
        const auto& plugins = MtohGetRendererPlugins();
        if (plugins.empty()) { return r; }
        r.reserve(plugins.size());
        for (const auto& plugin : plugins) {
            r.emplace_back(
                plugin,
                TfToken(
                    TfStringPrintf("mtohRenderOverride_%s", plugin.GetText())),
                TfToken(TfStringPrintf(
                    "%s (Hydra)",
                    MtohGetRendererPluginDisplayName(plugin).c_str())));
        }
        return r;
    }();
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
