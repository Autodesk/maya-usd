//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/pxOsd/tokens.h>

#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPolyMessage.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/mayaAttrs.h>
#include <hdmaya/adapters/shapeAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(_tokens, (st));

const std::array<std::pair<MObject&, HdDirtyBits>, 6> _dirtyBits{{
    {MayaAttrs::mesh::pnts,
     // This is useful when the user edits the mesh.
     HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent},
    {MayaAttrs::mesh::inMesh,
     // We are tracking topology changes and uv changes separately
     HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent},
    {MayaAttrs::mesh::worldMatrix, HdChangeTracker::DirtyTransform},
    {MayaAttrs::mesh::doubleSided, HdChangeTracker::DirtyDoubleSided},
    {MayaAttrs::mesh::intermediateObject, HdChangeTracker::DirtyVisibility},
    {MayaAttrs::mesh::uvPivot,
     // Tracking manual edits to uvs.
     HdChangeTracker::DirtyPrimvar},
}};

} // namespace

class HdMayaMeshAdapter : public HdMayaShapeAdapter {
public:
    HdMayaMeshAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaShapeAdapter(delegate->GetPrimPath(dag), delegate, dag) {}

    ~HdMayaMeshAdapter() = default;

    void Populate() override {
        GetDelegate()->InsertRprim(
            HdPrimTypeTokens->mesh, GetID(), HdChangeTracker::AllDirty);
    }

    void CreateCallbacks() override {
        MStatus status;
        auto obj = GetNode();
        if (obj != MObject::kNullObj) {
            auto id = MNodeMessage::addNodeDirtyPlugCallback(
                obj, NodeDirtiedCallback, this, &status);
            if (status) { AddCallback(id); }
            id = MNodeMessage::addAttributeChangedCallback(
                obj, AttributeChangedCallback, this, &status);
            if (status) { AddCallback(id); }
            id = MPolyMessage::addPolyTopologyChangedCallback(
                obj, TopologyChangedCallback, this, &status);
            if (status) { AddCallback(id); }
            bool wantModifications[3] = {true, true, true};
            id = MPolyMessage::addPolyComponentIdChangedCallback(
                obj, wantModifications, 3, ComponentIdChanged, this, &status);
            if (status) { AddCallback(id); }
            id = MPolyMessage::addUVSetChangedCallback(
                obj, UVSetChangedCallback, this, &status);
            if (status) { AddCallback(id); }
        }
        HdMayaDagAdapter::CreateCallbacks();
    }

    bool IsSupported() override {
        return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(
            HdPrimTypeTokens->mesh);
    }

    VtValue Get(const TfToken& key) override {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "Called HdMayaMeshAdapter::Get(%s) - %s\n", key.GetText(),
                GetDagPath().partialPathName().asChar());

        if (key == HdTokens->points) {
            MFnMesh mesh(GetDagPath());
            // Same memory layout for MFloatVector and GfVec3f!
            MStatus status;
            const auto* rawPoints =
                reinterpret_cast<const GfVec3f*>(mesh.getRawPoints(&status));
            if (!status) { return {}; }
            VtVec3fArray ret;
            ret.assign(rawPoints, rawPoints + mesh.numVertices());
            return VtValue(ret);
        } else if (key == _tokens->st) {
            MFnMesh mesh(GetDagPath());
            MStatus status;
            VtArray<GfVec2f> uvs;
            uvs.reserve(mesh.numFaceVertices());
            for (MItMeshPolygon pit(GetDagPath()); !pit.isDone(); pit.next()) {
                const auto vertexCount = pit.polygonVertexCount();
                for (auto i = decltype(vertexCount){0}; i < vertexCount; ++i) {
                    float2 uv = {0.0f, 0.0f};
                    pit.getUV(i, uv);
                    uvs.push_back(GfVec2f(uv[0], uv[1]));
                }
            }

            return VtValue(uvs);
        }
        return {};
    }

    HdMeshTopology GetMeshTopology() override {
        MFnMesh mesh(GetDagPath());
        const auto numPolygons = mesh.numPolygons();
        VtIntArray faceVertexCounts;
        faceVertexCounts.reserve(static_cast<size_t>(numPolygons));
        VtIntArray faceVertexIndices;
        faceVertexIndices.reserve(static_cast<size_t>(mesh.numFaceVertices()));
        for (MItMeshPolygon pit(GetDagPath()); !pit.isDone(); pit.next()) {
            const auto vc = pit.polygonVertexCount();
            faceVertexCounts.push_back(vc);
            for (auto i = decltype(vc){0}; i < vc; ++i) {
                faceVertexIndices.push_back(pit.vertexIndex(i));
            }
        }

        return HdMeshTopology(
            GetDelegate()->GetParams().displaySmoothMeshes
                ? PxOsdOpenSubdivTokens->catmullClark
                : PxOsdOpenSubdivTokens->none,
            UsdGeomTokens->rightHanded, faceVertexCounts, faceVertexIndices);
    }

    HdPrimvarDescriptorVector GetPrimvarDescriptors(
        HdInterpolation interpolation) override {
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

    bool GetDoubleSided() override {
        MFnMesh mesh(GetDagPath());
        auto p = mesh.findPlug(MayaAttrs::mesh::doubleSided, true);
        if (ARCH_UNLIKELY(p.isNull())) { return true; }
        bool doubleSided = true;
        p.getValue(doubleSided);
        return doubleSided;
    }

    bool HasType(const TfToken& typeId) override {
        return typeId == HdPrimTypeTokens->mesh;
    }

private:
    static void NodeDirtiedCallback(
        MObject& node, MPlug& plug, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        for (const auto& it : _dirtyBits) {
            if (it.first == plug) {
                adapter->MarkDirty(it.second);
                TF_DEBUG(HDMAYA_ADAPTER_MESH_PLUG_DIRTY)
                    .Msg(
                        "Marking prim dirty with bits %u because %s plug was "
                        "dirtied.\n",
                        it.second, plug.partialName().asChar());
                return;
            }
        }

        TF_DEBUG(HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY)
            .Msg(
                "%s (%s) plug dirtying was not handled by "
                "HdMayaMeshAdapter::NodeDirtiedCallback.\n",
                plug.name().asChar(), plug.partialName().asChar());
    }

    // For material assignments for now.
    static void AttributeChangedCallback(
        MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug,
        void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        if (plug == MayaAttrs::mesh::instObjGroups) {
            adapter->MarkDirty(HdChangeTracker::DirtyMaterialId);
        } else {
            TF_DEBUG(HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY)
                .Msg(
                    "%s (%s) plug dirtying was not handled by "
                    "HdMayaMeshAdapter::attributeChangedCallback.\n",
                    plug.name().asChar(), plug.name().asChar());
        }
    }

    static void TopologyChangedCallback(MObject& node, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        adapter->MarkDirty(
            HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyPoints);
    }

    static void ComponentIdChanged(
        MUintArray componentIds[], unsigned int count, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        adapter->MarkDirty(
            HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyPoints);
    }

    static void UVSetChangedCallback(
        MObject& node, const MString& name, MPolyMessage::MessageType type,
        void* clientData) {
        // TODO: Only track the uvset we care about.
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        adapter->MarkDirty(HdChangeTracker::DirtyPrimvar);
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaMeshAdapter, TfType::Bases<HdMayaShapeAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, mesh) {
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken("mesh"),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(new HdMayaMeshAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
