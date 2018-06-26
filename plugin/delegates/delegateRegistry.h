#ifndef __HDMAYA_DELEGATE_REGISTRY_H__
#define __HDMAYA_DELEGATE_REGISTRY_H__

#include <pxr/pxr.h>
#include <pxr/base/tf/singleton.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegateRegistry : public TfSingleton<HdMayaDelegateRegistry> {
    friend class TfSingleton<HdMayaDelegateRegistry>;
    HdMayaDelegateRegistry() = default;
public:
    static HdMayaDelegateRegistry& getInstance() {
        return TfSingleton<HdMayaDelegateRegistry>::GetInstance();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_REGISTRY_H__
