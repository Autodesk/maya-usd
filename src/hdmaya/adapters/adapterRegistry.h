#ifndef __HDMAYA_ADAPTER_REGISTRY_H__
#define __HDMAYA_ADAPTER_REGISTRY_H__

#include <pxr/pxr.h>
#include <pxr/base/tf/singleton.h>

#include <maya/MFn.h>

#include <hdmaya/delegates/delegateCtx.h>
#include <hdmaya/adapters/dagAdapter.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapterRegistry : public TfSingleton<HdMayaAdapterRegistry> {
    friend class TfSingleton<HdMayaAdapterRegistry>;
    HDMAYA_API
    HdMayaAdapterRegistry() = default;
public:
    static HdMayaAdapterRegistry& getInstance() {
        return TfSingleton<HdMayaAdapterRegistry>::GetInstance();
    }

    using DagAdapterCreator = std::function<
        HdMayaDagAdapterPtr(HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterDagAdapter(const std::string& type, DagAdapterCreator creator);

    HDMAYA_API
    static DagAdapterCreator GetAdapterCreator(const MDagPath& dag);
private:
    std::unordered_map<std::string, DagAdapterCreator> _dagAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_REGISTRY_H__
