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

// GLEW must be included early (for core USD < 21.02), but we need pxr.h first
// so that USD_VERSION_NUM has the correct value.
// We also disable clang-format for this block, since otherwise v10.0.0 fails
// to recognize that "utils.h" is the related header.
// clang-format off
#include <pxr/pxr.h>
#if USD_VERSION_NUM < 2102
#include <pxr/imaging/glf/glew.h>
#endif
// clang-format on

#include "utils.h"

#include "renderGlobals.h"
#include "tokens.h"

#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hd/rendererPluginRegistry.h>

#include <maya/MGlobal.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::pair<const MtohRendererDescriptionVector&, const MtohRendererSettings&>
MtohInitializeRenderPlugins()
{
    using Storage = std::pair<MtohRendererDescriptionVector, MtohRendererSettings>;

    static const Storage ret = []() -> Storage {
        HdRendererPluginRegistry& pluginRegistry = HdRendererPluginRegistry::GetInstance();
        HfPluginDescVector        pluginDescs;
        pluginRegistry.GetPluginDescs(&pluginDescs);

        Storage store;
        store.first.reserve(pluginDescs.size());

        MtohRenderGlobals::OptionsPreamble();

        for (const auto& pluginDesc : pluginDescs) {
            const TfToken     renderer = pluginDesc.id;
            HdRendererPlugin* plugin = pluginRegistry.GetRendererPlugin(renderer);
            if (!plugin) {
                continue;
            }

            // XXX: As of 22.02, this needs to be called for Storm
            if (pluginDesc.id == MtohTokens->HdStormRendererPlugin) {
#if USD_VERSION_NUM < 2102
                GlfGlewInit();
#endif
                GlfContextCaps::InitInstance();
            }

            HdRenderDelegate* delegate
                = plugin->IsSupported() ? plugin->CreateRenderDelegate() : nullptr;

            // No 'delete plugin', should plugin be cached as well?
            if (!delegate) {
                continue;
            }

            auto& rendererSettingDescriptors
                = store.second.emplace(renderer, delegate->GetRenderSettingDescriptors())
                      .first->second;

            // We only needed the delegate for the settings, so release
            plugin->DeleteRenderDelegate(delegate);
            // Null it out to make any possible usage later obv, wrong!
            delegate = nullptr;

            store.first.emplace_back(
                renderer,
                TfToken(TfStringPrintf("mtohRenderOverride_%s", renderer.GetText())),
                TfToken(TfStringPrintf("%s (Hydra)", pluginDesc.displayName.c_str())));
            MtohRenderGlobals::BuildOptionsMenu(store.first.back(), rendererSettingDescriptors);
        }

        // Make sure the static's size doesn't have any extra overhead
        store.first.shrink_to_fit();
        assert(store.first.size() == store.second.size());
        return store;
    }();
    return ret;
}

} // namespace

std::string MtohGetRendererPluginDisplayName(const TfToken& id)
{
    HfPluginDesc pluginDesc;
    if (!TF_VERIFY(HdRendererPluginRegistry::GetInstance().GetPluginDesc(id, &pluginDesc))) {
        return {};
    }

    return pluginDesc.displayName;
}

const MtohRendererDescriptionVector& MtohGetRendererDescriptions()
{
    return MtohInitializeRenderPlugins().first;
}

const MtohRendererSettings& MtohGetRendererSettings()
{
    return MtohInitializeRenderPlugins().second;
}

PXR_NAMESPACE_CLOSE_SCOPE
