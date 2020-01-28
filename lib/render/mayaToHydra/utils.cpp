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

PXR_NAMESPACE_OPEN_SCOPE

namespace {
constexpr auto MTOH_DEFAULT_RENDERER_PLUGIN_NAME =
    "MTOH_DEFAULT_RENDERER_PLUGIN";
}

TfTokenVector MtohGetRendererPlugins() {
    static const auto ret = []() -> TfTokenVector {
        HfPluginDescVector pluginDescs;
        HdRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

        TfTokenVector r;
        r.reserve(pluginDescs.size());
        for (const auto& desc : pluginDescs) { r.emplace_back(desc.id); }
        return r;
    }();

    return ret;
}

std::string MtohGetRendererPluginDisplayName(const TfToken& id) {
    HfPluginDesc pluginDesc;
    if (!TF_VERIFY(HdRendererPluginRegistry::GetInstance().GetPluginDesc(
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
