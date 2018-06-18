#ifndef __HDMAYA_POINT_LIGHT_ADAPTER_H__
#define __HDMAYA_POINT_LIGHT_ADAPTER_H__

#include <pxr/pxr.h>

#include "lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaPointLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaPointLightAdapter(const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dagPath);

    ~HdMayaPointLightAdapter() override = default;

    void Populate(
        HdRenderIndex& renderIndex,
        HdSceneDelegate* delegate,
        const SdfPath& id) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_POINT_LIGHT_ADAPTER_H__
