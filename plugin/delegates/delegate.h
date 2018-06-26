#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate : protected HdSceneDelegate {
public:
    HdMayaDelegate(
        HdRenderIndex* parentIndex,
        const SdfPath& delegateID);

    using HdSceneDelegate::GetDelegateID;
    virtual void Populate() = 0;
};

using HdMayaDelegatePtr = std::shared_ptr<HdMayaDelegate>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
