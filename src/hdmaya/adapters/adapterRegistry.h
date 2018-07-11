#ifndef __HDMAYA_ADAPTER_REGISTRY_H__
#define __HDMAYA_ADAPTER_REGISTRY_H__

#include <pxr/pxr.h>
#include <pxr/base/tf/singleton.h>

#include <maya/MFn.h>

#include <hdmaya/delegates/delegateCtx.h>
#include <hdmaya/adapters/dagAdapter.h>
#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/adapters/materialAdapter.h>

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
    static void RegisterDagAdapter(const TfToken& type, DagAdapterCreator creator);

    HDMAYA_API
    static DagAdapterCreator GetDagAdapterCreator(const MDagPath& dag);

    using LightAdapterCreator = std::function<
        HdMayaLightAdapterPtr(HdMayaDelegateCtx*, const MDagPath&)>;
    HDMAYA_API
    static void RegisterLightAdapter(const TfToken& type, LightAdapterCreator creator);

    HDMAYA_API
    static LightAdapterCreator GetLightAdapterCreator(const MDagPath& dag);

    using MaterialAdapterCreator = std::function<
        HdMayaMaterialAdapterPtr(HdMayaDelegateCtx*, const MObject&)>;
    HDMAYA_API
    static void RegisterMaterialAdapter(const TfToken& type, MaterialAdapterCreator creator);

    HDMAYA_API
    static MaterialAdapterCreator GetMaterialAdapterCreator(const MObject& node);

    // Find all HdMayaAdapter plugins, and load them all
    HDMAYA_API
    static void LoadAllPlugin();
private:
    std::unordered_map<TfToken, DagAdapterCreator, TfToken::HashFunctor> _dagAdapters;
    std::unordered_map<TfToken, LightAdapterCreator, TfToken::HashFunctor> _lightAdapters;
    std::unordered_map<TfToken, MaterialAdapterCreator, TfToken::HashFunctor> _materialAdapters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_REGISTRY_H__
