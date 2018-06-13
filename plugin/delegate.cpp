#include "delegate.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hdx/renderSetupTask.h>
#include <pxr/imaging/hdx/renderTask.h>

#include <usdMaya/util.h>

#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MString.h>

#include "util.h"

PXR_NAMESPACE_USING_DIRECTIVE

HdMayaDelegate::HdMayaDelegate(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) :
    HdSceneDelegate(renderIndex, delegateID) {
    // Unique ID
}

HdMayaDelegate::~HdMayaDelegate() {

}

HdMeshTopology
HdMayaDelegate::GetMeshTopology(const SdfPath& id) {
    MDagPath dg;
    if (!TfMapLookup(pathToDgMap, id, &dg)) { return {}; }
    MFnMesh mesh(dg);
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

GfRange3d
HdMayaDelegate::GetExtent(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting extent " << id << std::endl;
    GfRange3d ret(GfVec3d(-1.0), GfVec3d(1.0));
    return ret;
}

GfMatrix4d
HdMayaDelegate::GetTransform(const SdfPath& id) {
    std::cerr << "[HdMayaSceneDelegate] Getting transform " << id << std::endl;
    MDagPath dg;
    if (!TfMapLookup(pathToDgMap, id, &dg)) { return GfMatrix4d(1.0); }
    return getGfMatrixFromMaya(dg.inclusiveMatrix());
}

bool
HdMayaDelegate::IsEnabled(const TfToken& option) const {
    // Maya scene can't be accessed on multiple threads,
    // so I don't think this is safe to enable.
    if (option == HdOptionTokens->parallelRprimSync) {
        return false;
    }

    std::cerr << "Checking for option: " << option << std::endl;

    return false;
}

VtValue
HdMayaDelegate::Get(SdfPath const& id, const TfToken& key) {
    std::cerr << "[HdMayaDelegate] Accessing " << key << " from " << id << std::endl;
    MDagPath dg;
    if (!TfMapLookup(pathToDgMap, id, &dg)) { return VtValue(); }
    MFnMesh mesh(dg);

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

void
HdMayaDelegate::Populate() {

    for (MItDag dagIt(MItDag::kDepthFirst, MFn::kInvalid); !dagIt.isDone(); dagIt.next()) {
        MDagPath path;
        dagIt.getPath(path);

        // We don't care about transforms for now.
        if (path.hasFn(MFn::kTransform)) { continue; }

        if (path.hasFn(MFn::kMesh)) {
            const auto id = getPrimPath(path);
            pathToDgMap.insert({id, path});
            GetRenderIndex().InsertRprim(HdPrimTypeTokens->mesh, this, id);
        }
    }
}

SdfPath
HdMayaDelegate::getPrimPath(const MDagPath& dg) {
    const auto mayaPath = PxrUsdMayaUtil::MDagPathToUsdPath(dg, false, false);
    const auto* chr = mayaPath.GetText();
    TF_VERIFY(chr != nullptr);
    return GetDelegateID().AppendPath(SdfPath(std::string(chr + 1)));
}

bool HdMayaDelegate::GetVisible(const SdfPath& id) {
    std::cerr << "[HdSceneDelegate] Getting visibility of " << id << std::endl;
    return true;
}
