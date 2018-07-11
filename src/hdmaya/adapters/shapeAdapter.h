#ifndef __HDMAYA_SHAPE_ADAPTER_H__
#define __HDMAYA_SHAPE_ADAPTER_H__

#include <pxr/pxr.h>

#include <hdmaya/adapters/dagAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaShapeAdapter : public HdMayaDagAdapter {
protected:
    HDMAYA_API
    HdMayaShapeAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath);

public:
    HDMAYA_API
    virtual ~HdMayaShapeAdapter() = default;

    HDMAYA_API
    virtual HdMeshTopology GetMeshTopology();
    HDMAYA_API
    virtual HdPrimvarDescriptorVector
    GetPrimvarDescriptors(HdInterpolation interpolation);
    HDMAYA_API
    virtual void MarkDirty(HdDirtyBits dirtyBits) override;

    GfRange3d GetExtent() {
        return _extent;
    }

protected:
    HDMAYA_API
    void CalculateExtent();

private:
    GfRange3d _extent;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_SHAPE_ADAPTER_H__
