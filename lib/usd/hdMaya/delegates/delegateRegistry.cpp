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
#include "delegateRegistry.h"

#include <hdMaya/delegates/delegateDebugCodes.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/type.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaDelegateRegistry);

void HdMayaDelegateRegistry::RegisterDelegate(const TfToken& name, DelegateCreator creator)
{
    auto& instance = GetInstance();
    for (auto it : instance._delegates) {
        if (name == std::get<0>(it)) {
            TF_DEBUG(HDMAYA_DELEGATE_REGISTRY)
                .Msg(
                    "HdMayaDelegateRegistry::RegisterDelegate(%s) - existing "
                    "delegate\n",
                    name.GetText());
            return;
        }
    }

    TF_DEBUG(HDMAYA_DELEGATE_REGISTRY)
        .Msg("HdMayaDelegateRegistry::RegisterDelegate(%s) - new delegate\n", name.GetText());
    instance._delegates.emplace_back(name, creator);
}

std::vector<TfToken> HdMayaDelegateRegistry::GetDelegateNames()
{
    LoadAllDelegates();
    const auto&          instance = GetInstance();
    std::vector<TfToken> ret;
    ret.reserve(instance._delegates.size());
    for (auto it : instance._delegates) {
        ret.push_back(std::get<0>(it));
    }
    return ret;
}

std::vector<HdMayaDelegateRegistry::DelegateCreator> HdMayaDelegateRegistry::GetDelegateCreators()
{
    LoadAllDelegates();
    const auto&                                          instance = GetInstance();
    std::vector<HdMayaDelegateRegistry::DelegateCreator> ret;
    ret.reserve(instance._delegates.size());
    for (auto it : instance._delegates) {
        ret.push_back(std::get<1>(it));
    }
    return ret;
}

void HdMayaDelegateRegistry::SignalDelegatesChanged()
{
    for (const auto& s : GetInstance()._signals) {
        s();
    }
}

void HdMayaDelegateRegistry::LoadAllDelegates()
{
    static std::once_flag loadAllOnce;
    std::call_once(loadAllOnce, _LoadAllDelegates);
}

void HdMayaDelegateRegistry::InstallDelegatesChangedSignal(DelegatesChangedSignal signal)
{
    GetInstance()._signals.emplace_back(signal);
}

void HdMayaDelegateRegistry::_LoadAllDelegates()
{
    TF_DEBUG(HDMAYA_DELEGATE_REGISTRY).Msg("HdMayaDelegateRegistry::_LoadAllDelegates()\n");

    TfRegistryManager::GetInstance().SubscribeTo<HdMayaDelegateRegistry>();

    const TfType& delegateType = TfType::Find<HdMayaDelegate>();
    if (delegateType.IsUnknown()) {
        TF_CODING_ERROR("Could not find HdMayaDelegate type");
        return;
    }

    std::set<TfType> delegateTypes;
    delegateType.GetAllDerivedTypes(&delegateTypes);

    PlugRegistry& plugReg = PlugRegistry::GetInstance();

    for (auto& subType : delegateTypes) {
        const PlugPluginPtr pluginForType = plugReg.GetPluginForType(subType);
        if (!pluginForType) {
            TF_CODING_ERROR("Could not find plugin for '%s'", subType.GetTypeName().c_str());
            return;
        }
        pluginForType->Load();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
