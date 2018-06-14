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

#include "utils.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDagAdapter {
protected:
    HdMayaDagAdapter(const MDagPath& dagPath);
public:
    virtual ~HdMayaDagAdapter() = default;

    virtual void Populate(
        HdRenderIndex& renderIndex,
        HdSceneDelegate* delegate,
        const SdfPath& id) = 0;

    virtual VtValue Get(const TfToken& key);
    virtual GfRange3d GetExtent();
    virtual HdMeshTopology GetMeshTopology();
    virtual GfMatrix4d GetTransform() ;

    const MDagPath& GetDagPath() { return _dagPath; }
protected:
    MDagPath _dagPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DG_ADAPTER_H__
