#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>

#include <memory>

#include <hdmaya/api.h>
#include <hdmaya/delegates/params.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate {
public:
    HDMAYA_API
    HdMayaDelegate() = default;
    HDMAYA_API
    virtual ~HdMayaDelegate() = default;

    virtual void Populate() = 0;
    virtual void PreFrame() { }
    virtual void PostFrame() { }

    HDMAYA_API
    virtual void SetParams(const HdMayaParams& params);
    const HdMayaParams& GetParams() { return _params; }
private:
    HdMayaParams _params;
};

using HdMayaDelegatePtr = std::shared_ptr<HdMayaDelegate>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
