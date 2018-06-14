#include "dagAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaDagAdapter::HdMayaDagAdapter(const MDagPath& dagPath) :
    _dagPath(dagPath) {

}

VtValue HdMayaDagAdapter::Get(const TfToken& /*key*/) {
    return {};
};

GfRange3d HdMayaDagAdapter::GetExtent() {
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

HdMeshTopology HdMayaDagAdapter::GetMeshTopology() {
    return {};
};

GfMatrix4d HdMayaDagAdapter::GetTransform() {
    return getGfMatrixFromMaya(_dagPath.inclusiveMatrix());
};

PXR_NAMESPACE_CLOSE_SCOPE
