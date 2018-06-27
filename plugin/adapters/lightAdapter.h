#ifndef __HDMAYA_LIGHT_ADAPTER_H__
#define __HDMAYA_LIGHT_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/frustum.h>

#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

#include <maya/MFnLight.h>

#include "dagAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaLightAdapter : public HdMayaDagAdapter {
public:
    HdMayaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);
    void MarkDirty(HdDirtyBits dirtyBits) override;
    VtValue GetLightParamValue(const TfToken& paramName) override;
    VtValue Get(const TfToken& key) override;
    virtual void CreateCallbacks() override;
    virtual void Populate() override;
protected:
    virtual void CalculateLightParams(GlfSimpleLight& light);
    void CalculateShadowParams(MFnLight& light, GfFrustum& frustum, HdxShadowParams& params);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_LIGHT_ADAPTER_H__
