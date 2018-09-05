//
// Copyright 2018 Luma Pictures
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
#include <hdmaya/delegates/delegateRegistry.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/type.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaDelegateRegistry);

void HdMayaDelegateRegistry::RegisterDelegate(
    const TfToken& name, DelegateCreator creator) {
    auto& instance = GetInstance();
    for (auto it : instance._delegates) {
        if (name == std::get<0>(it)) { return; }
    }
    instance._delegates.emplace_back(name, creator);
}

std::vector<TfToken> HdMayaDelegateRegistry::GetDelegateNames() {
    LoadAllDelegates();
    const auto& instance = GetInstance();
    std::vector<TfToken> ret;
    ret.reserve(instance._delegates.size());
    for (auto it : instance._delegates) { ret.push_back(std::get<0>(it)); }
    return ret;
}

std::vector<HdMayaDelegateRegistry::DelegateCreator>
HdMayaDelegateRegistry::GetDelegateCreators() {
    LoadAllDelegates();
    const auto& instance = GetInstance();
    std::vector<HdMayaDelegateRegistry::DelegateCreator> ret;
    ret.reserve(instance._delegates.size());
    for (auto it : instance._delegates) { ret.push_back(std::get<1>(it)); }
    return ret;
}

void HdMayaDelegateRegistry::SignalDelegatesChanged() {
    for (const auto& s : GetInstance()._signals) { s(); }
}

void HdMayaDelegateRegistry::LoadAllDelegates() {
    static std::once_flag loadAllOnce;
    std::call_once(loadAllOnce, _LoadAllDelegates);
}

void HdMayaDelegateRegistry::InstallDelegatesChangedSignal(
    DelegatesChangedSignal signal) {
    GetInstance()._signals.emplace_back(signal);
}

void HdMayaDelegateRegistry::_LoadAllDelegates() {
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
            TF_CODING_ERROR(
                "Could not find plugin for '%s'",
                subType.GetTypeName().c_str());
            return;
        }
        pluginForType->Load();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
