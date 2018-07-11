#ifndef __HDMAYA_DG_ADAPTER_H__
#define __HDMAYA_DG_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>

#include <pxr/base/tf/token.h>

#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <functional>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MMatrix.h>
#include <maya/MFn.h>

#include <maya/MMessage.h>

#include <hdmaya/utils.h>
#include <hdmaya/adapters/adapter.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDagAdapter : public HdMayaAdapter {
protected:
    HDMAYA_API
    HdMayaDagAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath);

public:
    HDMAYA_API
    virtual ~HdMayaDagAdapter() = default;

    HDMAYA_API
    virtual void CreateCallbacks() override;
    HDMAYA_API
    virtual void MarkDirty(HdDirtyBits dirtyBits);
    HDMAYA_API
    virtual void RemovePrim() override;

    const MDagPath& GetDagPath() { return _dagPath; }
    GfMatrix4d GetTransform() {
        return _transform;
    }

protected:
    HDMAYA_API
    void CalculateTransform();

private:
    MDagPath _dagPath;
    GfMatrix4d _transform;
};

using HdMayaShapeAdapterPtr = std::shared_ptr<HdMayaDagAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DG_ADAPTER_H__
