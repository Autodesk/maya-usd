#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/pxOsd/tokens.h>

#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/shapeAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (st)
);

std::vector<std::pair<MString, HdDirtyBits>> _dirtyBits = {
    {MString("pt"),
     HdChangeTracker::DirtyPoints |
     HdChangeTracker::DirtyExtent},
    {MString("i"),
     HdChangeTracker::DirtyPoints |
     HdChangeTracker::DirtyExtent |
     HdChangeTracker::DirtyPrimvar |
     HdChangeTracker::DirtyTopology |
     HdChangeTracker::DirtyNormals},
};

}

class HdMayaMeshAdapter : public HdMayaShapeAdapter {
public:
    HdMayaMeshAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaShapeAdapter(delegate->GetPrimPath(dag), delegate, dag) { }

    void
    Populate() override {
        GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), HdChangeTracker::AllDirty);

        MStatus status;
        // Duh!
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyCallback(
            obj,
            NodeDirtiedCallback,
            this,
            &status);
        if (status) { AddCallback(id); }
        id = MNodeMessage::addAttributeChangedCallback(
            obj,
            AttributeChangedCallback,
            this,
            &status);
        if (status) { AddCallback(id); }
    }

    bool
    IsSupported() override {
        return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->mesh);
    }

    VtValue
    Get(const TfToken& key) override {
        if (key == HdTokens->points) {
            MFnMesh mesh(GetDagPath());
            // Same memory layout for MFloatVector and GfVec3f!
            MStatus status;
            const auto* rawPoints = reinterpret_cast<const GfVec3f*>(mesh.getRawPoints(&status));
            if (!status) { return {}; }
            VtVec3fArray ret; ret.assign(rawPoints, rawPoints + mesh.numVertices());
            return VtValue(ret);
        } if (key == _tokens->st) {
            MFnMesh mesh(GetDagPath());
            // We need to flatten out the uvs.
            MStatus status;
            VtArray<GfVec2f> uvs;
            const auto numPolygons = mesh.numPolygons();
            uvs.reserve(static_cast<size_t>(mesh.numFaceVertices()));
            for (auto p = decltype(numPolygons){0}; p < numPolygons; ++p) {
                const auto numVertices = mesh.polygonVertexCount(p);
                for (auto v = decltype(numVertices){0}; v < numVertices; ++v) {
                    GfVec2f uv(0.0f, 0.0f);
                    mesh.getPolygonUV(p, v, uv[0], uv[1]);
                    uvs.push_back(uv);
                }
            }

            return VtValue(uvs);
        }
        return {};
    }

    HdMeshTopology
    GetMeshTopology() override {
        MFnMesh mesh(GetDagPath());
        const auto numPolygons = mesh.numPolygons();
        VtIntArray faceVertexCounts; faceVertexCounts.reserve(static_cast<size_t>(numPolygons));
        VtIntArray faceVertexIndices; faceVertexIndices.reserve(static_cast<size_t>(mesh.numFaceVertices()));
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
            GetDelegate()->GetParams().displaySmoothMeshes
            ? PxOsdOpenSubdivTokens->catmullClark
            : PxOsdOpenSubdivTokens->none,
            UsdGeomTokens->rightHanded,
            faceVertexCounts,
            faceVertexIndices);
    }

    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(HdInterpolation interpolation) override {
        if (interpolation == HdInterpolationVertex) {
            HdPrimvarDescriptor desc;
            desc.name = UsdGeomTokens->points;
            desc.interpolation = interpolation;
            desc.role = HdPrimvarRoleTokens->point;
            return {desc};
        } else if (interpolation == HdInterpolationFaceVarying) {
            // UVs are face varying in maya.
            MFnMesh mesh(GetDagPath());
            if (mesh.numUVs() > 0) {
                HdPrimvarDescriptor desc;
                desc.name = _tokens->st;
                desc.interpolation = interpolation;
                desc.role = HdPrimvarRoleTokens->textureCoordinate;
                return {desc};
            }
        }
        return {};
    }

    bool
    HasType(const TfToken& typeId) override {
        return typeId == HdPrimTypeTokens->mesh;
    }

private:
    static void
    NodeDirtiedCallback(MObject& node, MPlug& plug, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        const auto plugName = plug.partialName();
        for (const auto& it: _dirtyBits) {
            if (it.first == plugName) {
                adapter->MarkDirty(it.second);
                TF_DEBUG(HDMAYA_ADAPTER_MESH_PLUG_DIRTY).Msg(
                    "Marking prim dirty with bits %u because %s plug was dirtied.\n",
                    it.second, it.first.asChar());
                return;
            }
        }

        TF_DEBUG(HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY).Msg(
                "%s (%s) plug dirtying was not handled by HdMayaMeshAdapter::NodeDirtiedCallback.\n",
                plug.name().asChar(), plugName.asChar());
    }

    // For material assignments for now.
    static void
    AttributeChangedCallback(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        const auto plugName = plug.partialName();
        if (plugName == MString("iog")) {
            adapter->MarkDirty(HdChangeTracker::DirtyMaterialId);
        }
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaMeshAdapter, TfType::Bases<HdMayaShapeAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, mesh) {
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken("mesh"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(new HdMayaMeshAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
