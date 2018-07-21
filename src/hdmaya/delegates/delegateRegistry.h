#ifndef __HDMAYA_DELEGATE_REGISTRY_H__
#define __HDMAYA_DELEGATE_REGISTRY_H__

#include <pxr/pxr.h>
#include <pxr/base/tf/singleton.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/sdf/path.h>

#include <vector>
#include <tuple>

#include <hdmaya/delegates/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegateRegistry : public TfSingleton<HdMayaDelegateRegistry> {
    friend class TfSingleton<HdMayaDelegateRegistry>;
    HDMAYA_API
    HdMayaDelegateRegistry() = default;
public:
    // function creates and returns a pointer to a HdMayaDelegate - may return
    // a nullptr indicate failure, or that the delegate is currently disabled
    using DelegateCreator = std::function<
        HdMayaDelegatePtr(HdRenderIndex*, const SdfPath&)>;

    HDMAYA_API
    static void RegisterDelegate(const TfToken& name, DelegateCreator creator);
    HDMAYA_API
    static std::vector<TfToken> GetDelegateNames();
    HDMAYA_API
    static std::vector<DelegateCreator> GetDelegateCreators();

    // Signal that some delegate types are now either valid or invalid.
    // ie, say some delegate type is only useful / works when a certain maya
    // plugin is loaded - you would call this every time that plugin was loaded
    // or unloaded.
    HDMAYA_API
    static void SignalDelegatesChanged();

    // Find all HdMayaDelegate plugins, and load them all
    HDMAYA_API
    static void LoadAllDelegates();
private:
    static void _LoadAllDelegates();

    std::vector<std::tuple<TfToken, DelegateCreator>> _delegates;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_REGISTRY_H__
