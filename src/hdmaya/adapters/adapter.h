#ifndef __HDMAYA_ADAPTER_H__
#define __HDMAYA_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MMessage.h>

#include <vector>

#include <hdmaya/api.h>
#include <hdmaya/delegates/delegateCtx.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapter {
public:
    HDMAYA_API
    HdMayaAdapter(MObject node, const SdfPath& id, HdMayaDelegateCtx* delegate);
    HDMAYA_API
    virtual ~HdMayaAdapter();

    const SdfPath& GetID() { return _id; }
    HdMayaDelegateCtx* GetDelegate() { return _delegate; }
    HDMAYA_API
    void AddCallback(MCallbackId callbackId);
    HDMAYA_API
    virtual VtValue Get(const TfToken& key);
    const MObject& GetNode() { return _node; }
    HDMAYA_API
    virtual bool IsSupported() = 0;
    HDMAYA_API
    virtual bool HasType(const TfToken& typeId);

    HDMAYA_API
    virtual void CreateCallbacks();
    virtual void MarkDirty(HdDirtyBits dirtyBits) = 0;
    virtual void RemovePrim() = 0;
    virtual void Populate() = 0;
protected:
    MObject _node;
    SdfPath _id;
    HdMayaDelegateCtx* _delegate;

    std::vector<MCallbackId> _callbacks;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_H__
