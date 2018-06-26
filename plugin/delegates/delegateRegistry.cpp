#include "delegateRegistry.h"

#include <pxr/base/tf/instantiateSingleton.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaDelegateRegistry);

void
HdMayaDelegateRegistry::RegisterDelegate(const TfToken& name, DelegateCreator creator) {
    auto& instance = GetInstance();
    for (auto it : instance._delegates) {
        if (name == std::get<0>(it)) { return; }
    }
    instance._delegates.emplace_back(name, creator);
}

std::vector<TfToken>
HdMayaDelegateRegistry::GetDelegateNames() {
    TfRegistryManager::GetInstance().SubscribeTo<HdMayaDelegateRegistry>();
    const auto& instance = GetInstance();
    std::vector<TfToken> ret;
    ret.reserve(instance._delegates.size());
    for (auto it : instance._delegates) {
        ret.push_back(std::get<0>(it));
    }
    return ret;
}

std::vector<HdMayaDelegateRegistry::DelegateCreator>
HdMayaDelegateRegistry::GetDelegateCreators() {
    TfRegistryManager::GetInstance().SubscribeTo<HdMayaDelegateRegistry>();
    const auto& instance = GetInstance();
    std::vector<HdMayaDelegateRegistry::DelegateCreator> ret;
    ret.reserve(instance._delegates.size());
    for (auto it : instance._delegates) {
        ret.push_back(std::get<1>(it));
    }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
