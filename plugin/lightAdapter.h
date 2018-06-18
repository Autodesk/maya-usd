#ifndef __HDMAYA_LIGHT_ADAPTER_H__
#define __HDMAYA_LIGHT_ADAPTER_H__

#include <pxr/pxr.h>

#include "dagAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaLightAdapter : public HdMayaDagAdapter {
protected:
    HdMayaLightAdapter(const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dagPath);
public:
    virtual ~HdMayaLightAdapter() = default;

    void MarkDirty(HdDirtyBits dirtyBits) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_LIGHT_ADAPTER_H__
