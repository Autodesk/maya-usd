#include "adapterRegistry.h"

#include <pxr/base/tf/instantiateSingleton.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdMayaAdapterRegistry);

void
HdMayaAdapterRegistry::registerDagAdapter(const MFn::Type& type,
                                          DagAdapterCreator creator) {
    for (const auto& it: dagAdapters) {
        if (std::get<0>(it) == type) { return; }
    }
    dagAdapters.emplace_back(type, creator);
}

HdMayaDagAdapter*
HdMayaAdapterRegistry::createDagAdapter(const MDagPath& dag) {
    for (const auto& it: dagAdapters) {
        if (dag.hasFn(std::get<0>(it))) {
            return std::get<1>(it)(dag);
        }
    }
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
