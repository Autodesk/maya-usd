#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate {
public:
    HdMayaDelegate() = default;

    virtual void Populate() = 0;
    virtual void PreFrame() { }
    virtual void PostFrame() { }
};

using HdMayaDelegatePtr = std::shared_ptr<HdMayaDelegate>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
