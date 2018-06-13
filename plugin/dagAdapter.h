#ifndef __HDMAYA_DG_ADAPTER_H__
#define __HDMAYA_DG_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>

#include <pxr/base/tf/token.h>

#include <pxr/imaging/hd/meshTopology.h>

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

    virtual VtValue get (const MDagPath& dag, const TfToken& key);
    virtual GfRange3d getExtent (const MDagPath& dag);
    virtual HdMeshTopology getMeshTopology (const MDagPath& dag);
    virtual GfMatrix4d getTransform (const MDagPath& dag) ;

private:
    MDagPath dagPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DG_ADAPTER_H__
