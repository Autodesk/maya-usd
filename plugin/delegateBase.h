#ifndef __HDMAYA_DELEGATE_BASE_H__
#define __HDMAYA_DELEGATE_BASE_H__

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <pxr/usd/sdf/path.h>


PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegateBase : protected HdSceneDelegate {
protected:
    HdMayaDelegateBase(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);
public:
    using HdSceneDelegate::GetRenderIndex;
    HdChangeTracker& GetChangeTracker() { return GetRenderIndex().GetChangeTracker(); }
private:
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_BASE_H__
