#include "dagAdapter.h"

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

VtValue
HdMayaDagAdapter::Get(const TfToken& /*key*/) {
    return {};
};

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
HdMayaDagAdapter::CreateCallbacks(std::vector<MCallbackId>& ids) {
    MStatus status;
    for (auto dag = GetDagPath(); dag.length() > 0; dag.pop()) {
        // FIXME: I hate this. I have to figure out if the memory allocated for the
        // callback is going to be freed properly. If yes, we could use a weak pointer
        // to the adapter itself.
        MObject obj = dag.node();
        if (obj != MObject::kNullObj) {
            auto id = MNodeMessage::addNodeDirtyCallback(obj, _dirtyTransform, this, &status);
            if (status) { ids.push_back(id); }
        }
    }
}

void
HdMayaDagAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetRenderIndex().GetChangeTracker().MarkRprimDirty(GetID(), dirtyBits);
}

PXR_NAMESPACE_CLOSE_SCOPE
