#include <hdmaya/adapters/adapterRegistry.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/type.h>

#include <maya/MFnDependencyNode.h>

#include <mutex>

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
    LoadAllAdapters();
    const auto& instance = GetInstance();
    MFnDependencyNode node(dag.node());
    const auto it = instance._dagAdapters.find(node.typeName().asChar());
    return it == instance._dagAdapters.end() ? nullptr : it->second;
}

void
HdMayaAdapterRegistry::LoadAllAdapters() {
    static std::once_flag loadAllOnce;
    std::call_once(loadAllOnce, _LoadAllAdapters);
}

void
HdMayaAdapterRegistry::_LoadAllAdapters() {
    TfRegistryManager::GetInstance().SubscribeTo<HdMayaAdapterRegistry>();

    const TfType& adapterType = TfType::Find<HdMayaAdapter>();
    if (adapterType.IsUnknown()) {
        TF_CODING_ERROR("Could not find HdMayaAdapter type");
        return;
    }

    std::set<TfType> adapterTypes;
    adapterType.GetAllDerivedTypes(&adapterTypes);

    PlugRegistry& plugReg = PlugRegistry::GetInstance();

    for(auto& subType : adapterTypes)
    {
        const PlugPluginPtr pluginForType = plugReg.GetPluginForType(subType);
        if (!pluginForType) {
            TF_CODING_ERROR(
                "Could not find plugin for '%s'", subType.GetTypeName().c_str());
            return;
        }
        pluginForType->Load();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
