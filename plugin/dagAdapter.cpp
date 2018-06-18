#include "dagAdapter.h"

#include <pxr/imaging/hd/tokens.h>

#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    void _dirtyTransform(MObject& node, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
        adapter->MarkDirty(HdChangeTracker::DirtyTransform);
    }
}

HdMayaDagAdapter::HdMayaDagAdapter(
    const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dagPath) :
    HdMayaAdapter(id, delegate), _dagPath(dagPath) {

}

GfRange3d
HdMayaDagAdapter::GetExtent() {
    MStatus status;
    MFnDagNode dagNode(_dagPath, &status);
    if (ARCH_UNLIKELY(!status)) { return {}; }
    const auto bb = dagNode.boundingBox();
    const auto mn = bb.min();
    const auto mx = bb.max();
    return {
        {mn.x, mn.y, mn.z},
        {mx.x, mx.y, mx.z}
    };
};

HdMeshTopology
HdMayaDagAdapter::GetMeshTopology() {
    return {};
};

GfMatrix4d
HdMayaDagAdapter::GetTransform() {
    return getGfMatrixFromMaya(_dagPath.inclusiveMatrix());
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
}

void
HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetRenderIndex().GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
}

HdPrimvarDescriptorVector
HdMayaDagAdapter::GetPrimvarDescriptors(HdInterpolation interpolation) {
    return {};
}

VtValue
HdMayaDagAdapter::GetLightParamValue(const TfToken& paramName) {
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
