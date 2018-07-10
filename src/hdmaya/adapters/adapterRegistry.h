#ifndef __HDMAYA_ADAPTER_REGISTRY_H__
#define __HDMAYA_ADAPTER_REGISTRY_H__

#include <pxr/pxr.h>
#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/singleton.h>

#include <maya/MFn.h>

#include <hdmaya/delegates/delegateCtx.h>
#include <hdmaya/adapters/dagAdapter.h>
#include <hdmaya/adapters/lightAdapter.h>

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
    static void RegisterDagAdapter(const TfToken& type, DagAdapterCreator creator);

    HDMAYA_API
    static DagAdapterCreator GetDagAdapterCreator(const MDagPath& dag);

    using LightAdapterCreator = std::function<
        HdMayaLightAdapterPtr(HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterLightAdapter(const TfToken& type, LightAdapterCreator creator);

    HDMAYA_API
    static LightAdapterCreator GetLightAdapterCreator(const MDagPath& dag);

    // Find all HdMayaAdapter plugins, and load them all
    HDMAYA_API
    static void LoadAllPlugin();
private:
    TfHashMap<TfToken, DagAdapterCreator, TfToken::HashFunctor> _dagAdapters;
    TfHashMap<TfToken, LightAdapterCreator, TfToken::HashFunctor> _lightAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_REGISTRY_H__
