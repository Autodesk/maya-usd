#include <hdmaya/adapters/shapeAdapter.h>

#include <pxr/base/tf/type.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaShapeAdapter, TfType::Bases<HdMayaDagAdapter> >();
}

HdMayaShapeAdapter::HdMayaShapeAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath)
    : HdMayaDagAdapter(id, delegate, dagPath) {
    CalculateExtent();
}

void
HdMayaShapeAdapter::CalculateExtent() {
    MStatus status;
    MFnDagNode dagNode(GetDagPath(), &status);
    if (ARCH_LIKELY(status)) {
        const auto bb = dagNode.boundingBox();
        const auto mn = bb.min();
        const auto mx = bb.max();
        _extent.SetMin({mn.x, mn.y, mn.z});
        _extent.SetMax({mx.x, mx.y, mx.z});
    }
};

HdMeshTopology
HdMayaShapeAdapter::GetMeshTopology() {
    return {};
};

HdPrimvarDescriptorVector
HdMayaShapeAdapter::GetPrimvarDescriptors(HdInterpolation interpolation) {
    return {};
}

void
HdMayaShapeAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    HdMayaDagAdapter::MarkDirty(dirtyBits);
    if (dirtyBits & HdChangeTracker::DirtyPoints) {
        CalculateExtent();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
