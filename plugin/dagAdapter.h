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

#include "utils.h"
#include "adapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDagAdapter : public HdMayaAdapter {
protected:
    HdMayaDagAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath);
public:
    virtual ~HdMayaDagAdapter() = default;

    virtual GfRange3d GetExtent();
    virtual HdMeshTopology GetMeshTopology();
    virtual GfMatrix4d GetTransform();
    virtual void CreateCallbacks() override;
    virtual void MarkDirty(HdDirtyBits dirtyBits);
    virtual HdPrimvarDescriptorVector
    GetPrimvarDescriptors(HdInterpolation interpolation);
    // TODO: think about this!
    virtual VtValue GetLightParamValue(const TfToken& paramName);

    const MDagPath& GetDagPath() { return _dagPath; }
protected:
    MDagPath _dagPath;
};

using HdMayaDagAdapterPtr = std::shared_ptr<HdMayaDagAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DG_ADAPTER_H__
