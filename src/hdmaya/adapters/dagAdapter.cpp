#include <hdmaya/adapters/dagAdapter.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaDagAdapter, TfType::Bases<HdMayaAdapter> >();
}

namespace {

void _dirtyTransform(MObject& node, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->MarkDirty(HdChangeTracker::DirtyTransform);
}

}

HdMayaDagAdapter::HdMayaDagAdapter(
    const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath) :
    HdMayaAdapter(dagPath.node(), id, delegate), _dagPath(dagPath) {
    CalculateTransform();
    CalculateExtent();
}

void
HdMayaDagAdapter::CalculateExtent() {
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

void
HdMayaDagAdapter::CalculateTransform() {
    _transform = getGfMatrixFromMaya(_dagPath.inclusiveMatrix());
};

HdMeshTopology
HdMayaDagAdapter::GetMeshTopology() {
    return {};
};

void
HdMayaDagAdapter::CreateCallbacks() {
    MStatus status;
    for (auto dag = GetDagPath(); dag.length() > 0; dag.pop()) {
        // The adapter itself will free the callbacks, so we don't have to worry about
        // passing raw pointers to the callbacks. Hopefully.
        MObject obj = dag.node();
        if (obj != MObject::kNullObj) {
            auto id = MNodeMessage::addNodeDirtyCallback(obj, _dirtyTransform, this, &status);
            if (status) { AddCallback(id); }
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void
HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
    if (dirtyBits & HdChangeTracker::DirtyTransform) {
        CalculateTransform();
    }
    if (dirtyBits & HdChangeTracker::DirtyPoints) {
        CalculateExtent();
    }
}

void
HdMayaDagAdapter::RemovePrim() {
    GetDelegate()->RemoveRprim(GetID());
}

HdPrimvarDescriptorVector
HdMayaDagAdapter::GetPrimvarDescriptors(HdInterpolation interpolation) {
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
