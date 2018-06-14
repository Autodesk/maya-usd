#include "meshAdapter.h"

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/tokens.h>

#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, mesh) {
    HdMayaAdapterRegistry::RegisterDagAdapter(MFn::kMesh,
        [](const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dag) -> std::shared_ptr<HdMayaDagAdapter> {
        return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaMeshAdapter>(id, delegate, dag));
    });
}

HdMayaMeshAdapter::HdMayaMeshAdapter(
    const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dagPath) :
    HdMayaDagAdapter(id, delegate, dagPath) {

}

void HdMayaMeshAdapter::Populate(HdRenderIndex& renderIndex, HdSceneDelegate* delegate, const SdfPath& id) {
    renderIndex.InsertRprim(HdPrimTypeTokens->mesh, delegate, id);
}

HdMeshTopology
HdMayaMeshAdapter::GetMeshTopology() {
    MFnMesh mesh(_dagPath);
    const auto numPolygons = mesh.numPolygons();
    VtIntArray faceVertexCounts; faceVertexCounts.reserve(numPolygons);
    VtIntArray faceVertexIndices; faceVertexIndices.reserve(mesh.numFaceVertices());
    MIntArray mayaFaceVertexIndices;
    for (auto i = decltype(numPolygons){0}; i < numPolygons; ++i) {
        mesh.getPolygonVertices(i, mayaFaceVertexIndices);
        const auto numIndices = mayaFaceVertexIndices.length();
        faceVertexCounts.push_back(numIndices);
        for (auto j = decltype(numIndices){0}; j < numIndices; ++j) {
            faceVertexIndices.push_back(mayaFaceVertexIndices[j]);
        }
    }

    return HdMeshTopology(
        UsdGeomTokens->triangleSubdivisionRule,
        UsdGeomTokens->rightHanded,
        faceVertexCounts,
        faceVertexIndices);
}

VtValue
HdMayaMeshAdapter::Get(const TfToken& key) {
    MFnMesh mesh(_dagPath);

    if (key == HdTokens->points) {
        // Same memory layout for MFloatVector and GfVec3f!
        MStatus status;
        const auto* rawPoints = reinterpret_cast<const GfVec3f*>(mesh.getRawPoints(&status));
        if (!status) { return VtValue(); }
        VtVec3fArray ret; ret.assign(rawPoints, rawPoints + mesh.numVertices());
        return VtValue(ret);
    }
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
