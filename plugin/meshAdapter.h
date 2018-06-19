#ifndef __HDMAYA_MESH_ADAPTER_H__
#define __HDMAYA_MESH_ADAPTER_H__

#include <pxr/pxr.h>

#include "dagAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaMeshAdapter : public HdMayaDagAdapter {
public:
    HdMayaMeshAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);

    void Populate() override;

    VtValue Get(const TfToken& key) override;
    HdMeshTopology GetMeshTopology() override;
    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(HdInterpolation interpolation) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_MESH_ADAPTER_H__
