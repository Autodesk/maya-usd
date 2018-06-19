#ifndef __HDMAYA_LIGHT_ADAPTER_H__
#define __HDMAYA_LIGHT_ADAPTER_H__

#include <pxr/pxr.h>

#include "dagAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaLightAdapter : public HdMayaDagAdapter {
protected:
    HdMayaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);
public:
    void MarkDirty(HdDirtyBits dirtyBits) override;
    VtValue GetLightParamValue(const TfToken& paramName) override;
    VtValue Get(const TfToken& key) override;
    virtual void CreateCallbacks() override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_LIGHT_ADAPTER_H__
