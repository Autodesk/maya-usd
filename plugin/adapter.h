#ifndef __HDMAYA_ADAPTER_H__
#define __HDMAYA_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/usd/sdf/path.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapter {
public:
    HdMayaAdapter(const SdfPath& id, HdSceneDelegate* delegate);

    const SdfPath& GetID() { return _id; }
    HdSceneDelegate* GetDelegate() { return _delegate; }
protected:
    SdfPath _id;
    HdSceneDelegate* _delegate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_H__
