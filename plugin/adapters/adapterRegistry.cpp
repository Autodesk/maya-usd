#include "adapterRegistry.h"

#include <pxr/base/tf/instantiateSingleton.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaAdapterRegistry);

void
HdMayaAdapterRegistry::RegisterDagAdapter(const std::string& type,
                                          DagAdapterCreator creator) {
    auto& instance = GetInstance();
    if (instance._dagAdapters.find(type) != instance._dagAdapters.end()) { return; }
    instance._dagAdapters.insert({type, creator});
}

HdMayaAdapterRegistry::DagAdapterCreator
HdMayaAdapterRegistry::GetAdapterCreator(const MDagPath& dag) {
    TfRegistryManager::GetInstance().SubscribeTo<HdMayaAdapterRegistry>();
    auto& instance = GetInstance();
    MFnDependencyNode node(dag.node());
    const auto it = instance._dagAdapters.find(node.typeName().asChar());
    return it == instance._dagAdapters.end() ? nullptr : it->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
