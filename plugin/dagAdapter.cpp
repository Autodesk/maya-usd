#include "dagAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaDagAdapter::HdMayaDagAdapter(const MDagPath& dagPath) :
    dagPath(dagPath) {

}

VtValue HdMayaDagAdapter::get(const MDagPath& /*dag*/, const TfToken& /*key*/) {
    return {};
};

GfRange3d HdMayaDagAdapter::getExtent (const MDagPath& dag) {
    MStatus status;
    MFnDagNode dagNode(dag, &status);
    if (ARCH_UNLIKELY(!status)) { return {}; }
    const auto bb = dagNode.boundingBox();
    const auto mn = bb.min();
    const auto mx = bb.max();
    return {
        {mn.x, mn.y, mn.z},
        {mx.x, mx.y, mx.z}
    };
};

HdMeshTopology HdMayaDagAdapter::getMeshTopology (const MDagPath& /*dag*/) {
    return {};
};

GfMatrix4d HdMayaDagAdapter::getTransform (const MDagPath& dag) {
    return getGfMatrixFromMaya(dag.inclusiveMatrix());
};

PXR_NAMESPACE_CLOSE_SCOPE
