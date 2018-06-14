#include "adapterRegistry.h"

#include <pxr/base/tf/instantiateSingleton.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaAdapterRegistry);

void
HdMayaAdapterRegistry::RegisterDagAdapter(const MFn::Type& type,
                                          DagAdapterCreator creator) {
    auto& instance = GetInstance();
    for (const auto& it: instance._dagAdapters) {
        if (std::get<0>(it) == type) { return; }
    }
    instance._dagAdapters.emplace_back(type, creator);
}

HdMayaAdapterRegistry::DagAdapterCreator
HdMayaAdapterRegistry::GetAdapterCreator(const MDagPath& dag) {
    TfRegistryManager::GetInstance().SubscribeTo<HdMayaAdapterRegistry>();
    auto& instance = GetInstance();
    for (const auto& it: instance._dagAdapters) {
        if (dag.hasFn(std::get<0>(it))) {
            return std::get<1>(it);
        }
    }
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
