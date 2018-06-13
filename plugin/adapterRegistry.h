#ifndef __HDMAYA_ADAPTER_REGISTRY_H__
#define __HDMAYA_ADAPTER_REGISTRY_H__

#include <pxr/pxr.h>
#include <pxr/base/tf/singleton.h>

#include <maya/MFn.h>

#include "dagAdapter.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapterRegistry : public TfSingleton<HdMayaAdapterRegistry> {
    friend class TfSingleton<HdMayaAdapterRegistry>;
    HdMayaAdapterRegistry() = default;
public:
    static HdMayaAdapterRegistry& getInstance() {
        return TfSingleton<HdMayaAdapterRegistry>::GetInstance();
    }

    using DagAdapterCreator = std::function<HdMayaDagAdapter*(const MDagPath& dag)>;
    void registerDagAdapter(const MFn::Type& type, DagAdapterCreator creator);

    HdMayaDagAdapter* createDagAdapter(const MDagPath& dag);
private:
    std::vector<std::tuple<MFn::Type, DagAdapterCreator>> dagAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_REGISTRY_H__
