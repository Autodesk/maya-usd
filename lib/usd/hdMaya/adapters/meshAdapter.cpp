//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/adapters/mayaAttrs.h>
#include <hdMaya/adapters/shapeAdapter.h>
#include <hdMaya/adapters/tokens.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/pxOsd/tokens.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MAnimControl.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MDGContext.h>
#include <maya/MDGContextGuard.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MNodeMessage.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MPolyMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const std::pair<MObject&, HdDirtyBits> _dirtyBits[] {
    { MayaAttrs::mesh::pnts,
      // This is useful when the user edits the mesh.
      HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent
          | HdChangeTracker::DirtySubdivTags },
    { MayaAttrs::mesh::inMesh,
      // We are tracking topology changes and uv changes separately
      HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent
          | HdChangeTracker::DirtySubdivTags },
    { MayaAttrs::mesh::worldMatrix, HdChangeTracker::DirtyTransform },
    { MayaAttrs::mesh::doubleSided, HdChangeTracker::DirtyDoubleSided },
    { MayaAttrs::mesh::intermediateObject, HdChangeTracker::DirtyVisibility },
    { MayaAttrs::mesh::uvPivot,
      // Tracking manual edits to uvs.
      HdChangeTracker::DirtyPrimvar },
    { MayaAttrs::mesh::displaySmoothMesh, HdChangeTracker::DirtyDisplayStyle },
    { MayaAttrs::mesh::smoothLevel, HdChangeTracker::DirtyDisplayStyle }
};

} // namespace

class HdMayaMeshAdapter : public HdMayaShapeAdapter
{
public:
    HdMayaMeshAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaShapeAdapter(delegate->GetPrimPath(dag, false), delegate, dag)
    {
    }

    ~HdMayaMeshAdapter() = default;

    void Populate() override
    {
        if (_isPopulated) {
            return;
        }
        GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), GetInstancerID());
        _isPopulated = true;
    }

    void AddBuggyCallback(MCallbackId id) { _buggyCallbacks.append(id); }

    void CreateCallbacks() override
    {
        MStatus status;
        auto    obj = GetNode();
        if (obj != MObject::kNullObj) {
            TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
                .Msg("Creating mesh adapter callbacks for prim (%s).\n", GetID().GetText());

            auto id
                = MNodeMessage::addNodeDirtyPlugCallback(obj, NodeDirtiedCallback, this, &status);
            if (status) {
                AddCallback(id);
            }
            id = MNodeMessage::addAttributeChangedCallback(
                obj, AttributeChangedCallback, this, &status);
            if (status) {
                AddCallback(id);
            }
            id = MPolyMessage::addPolyTopologyChangedCallback(
                obj, TopologyChangedCallback, this, &status);
            if (status) {
                AddCallback(id);
            }
            bool wantModifications[3] = { true, true, true };
            id = MPolyMessage::addPolyComponentIdChangedCallback(
                obj, wantModifications, 3, ComponentIdChanged, this, &status);
            if (status) {
                AddBuggyCallback(id);
            }
            id = MPolyMessage::addUVSetChangedCallback(obj, UVSetChangedCallback, this, &status);
            if (status) {
                AddBuggyCallback(id);
            }
        }
        HdMayaDagAdapter::CreateCallbacks();
    }

    HDMAYA_API
    void RemoveCallbacks() override
    {
        if (_buggyCallbacks.length() > 0) {
            TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
                .Msg("Removing buggy PolyComponentIdChangedCallbacks\n");
            if (_node != MObject::kNullObj && MObjectHandle(_node).isValid()) {
                MMessage::removeCallbacks(_buggyCallbacks);
            }
            _buggyCallbacks.clear();
        }
        HdMayaAdapter::RemoveCallbacks();
    }

    bool IsSupported() const override
    {
        return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->mesh);
    }

    VtValue GetUVs()
    {
        MStatus status;
        MFnMesh mesh(GetDagPath(), &status);
        if (ARCH_UNLIKELY(!status)) {
            return {};
        }
        VtArray<GfVec2f> uvs;
        uvs.reserve(static_cast<size_t>(mesh.numFaceVertices()));
        for (MItMeshPolygon pit(GetDagPath()); !pit.isDone(); pit.next()) {
            const auto vertexCount = pit.polygonVertexCount();
            for (auto i = decltype(vertexCount) { 0 }; i < vertexCount; ++i) {
                float2 uv = { 0.0f, 0.0f };
                pit.getUV(i, uv);
                uvs.push_back(GfVec2f(uv[0], uv[1]));
            }
        }

        return VtValue(uvs);
    }

    VtValue GetPoints(const MFnMesh& mesh)
    {
        MStatus     status;
        const auto* rawPoints = reinterpret_cast<const GfVec3f*>(mesh.getRawPoints(&status));
        if (ARCH_UNLIKELY(!status)) {
            return {};
        }
        VtVec3fArray ret;
        ret.assign(rawPoints, rawPoints + mesh.numVertices());
        return VtValue(ret);
    }

    VtValue Get(const TfToken& key) override
    {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "Called HdMayaMeshAdapter::Get(%s) - %s\n",
                key.GetText(),
                GetDagPath().partialPathName().asChar());

        if (key == HdTokens->points) {
            MStatus status;
            MFnMesh mesh(GetDagPath(), &status);
            if (ARCH_UNLIKELY(!status)) {
                return {};
            }
            return GetPoints(mesh);
        } else if (key == HdMayaAdapterTokens->st) {
            return GetUVs();
        }
        return {};
    }

    size_t SamplePrimvar(const TfToken& key, size_t maxSampleCount, float* times, VtValue* samples)
        override
    {
        if (maxSampleCount < 1) {
            return 0;
        }

        if (key == HdTokens->points) {
            MStatus status;
            MFnMesh mesh(GetDagPath(), &status);
            if (ARCH_UNLIKELY(!status)) {
                return 0;
            }
            times[0] = 0.0f;
            samples[0] = GetPoints(mesh);
            if (maxSampleCount == 1 || !GetDelegate()->GetParams().enableMotionSamples) {
                return 1;
            }
            times[1] = 1.0f;
            MDGContextGuard guard(MAnimControl::currentTime() + 1.0);
            samples[1] = GetPoints(mesh);
            // FIXME: should we do this or in the render delegate?
            return samples[1] == samples[0] ? 1 : 2;
        } else if (key == HdMayaAdapterTokens->st) {
            times[0] = 0.0f;
            samples[0] = GetUVs();
            return 1;
        }
        return 0;
    }

    HdMeshTopology GetMeshTopology() override
    {
        MFnMesh    mesh(GetDagPath());
        const auto numPolygons = mesh.numPolygons();
        VtIntArray faceVertexCounts;
        faceVertexCounts.reserve(static_cast<size_t>(numPolygons));
        VtIntArray faceVertexIndices;
        faceVertexIndices.reserve(static_cast<size_t>(mesh.numFaceVertices()));
        for (MItMeshPolygon pit(GetDagPath()); !pit.isDone(); pit.next()) {
            const auto vc = pit.polygonVertexCount();
            faceVertexCounts.push_back(vc);
            for (auto i = decltype(vc) { 0 }; i < vc; ++i) {
                faceVertexIndices.push_back(pit.vertexIndex(i));
            }
        }

        // TODO: Maybe we could use the flat shading of the display style?
        return HdMeshTopology(
#if MAYA_APP_VERSION >= 2019
            (GetDelegate()->GetParams().displaySmoothMeshes || GetDisplayStyle().refineLevel > 0)
                ? PxOsdOpenSubdivTokens->catmullClark
                : PxOsdOpenSubdivTokens->none,
#else
            GetDelegate()->GetParams().displaySmoothMeshes ? PxOsdOpenSubdivTokens->catmullClark
                                                           : PxOsdOpenSubdivTokens->none,
#endif

            UsdGeomTokens->rightHanded,
            faceVertexCounts,
            faceVertexIndices);
    }

    HdDisplayStyle GetDisplayStyle() override
    {
#if MAYA_APP_VERSION >= 2019
        MStatus           status;
        MFnDependencyNode node(GetNode(), &status);
        if (ARCH_UNLIKELY(!status)) {
            return { 0, false, false };
        }
        const auto displaySmoothMesh
            = node.findPlug(MayaAttrs::mesh::displaySmoothMesh, true).asShort();
        if (displaySmoothMesh == 0) {
            return { 0, false, false };
        }
        const auto smoothLevel
            = std::max(0, node.findPlug(MayaAttrs::mesh::smoothLevel, true).asInt());
        return { smoothLevel, false, false };
#else
        return { 0, false, false };
#endif
    }

    PxOsdSubdivTags GetSubdivTags() override
    {
#if MAYA_APP_VERSION >= 2019
        PxOsdSubdivTags tags;
        if (GetDisplayStyle().refineLevel < 1) {
            return tags;
        }

        MStatus status;
        MFnMesh mesh(GetNode(), &status);
        if (ARCH_UNLIKELY(!status)) {
            return tags;
        }
        MUintArray   creaseVertIds;
        MDoubleArray creaseVertValues;
        mesh.getCreaseVertices(creaseVertIds, creaseVertValues);
        const auto creaseVertIdCount = creaseVertIds.length();
        if (!TF_VERIFY(creaseVertIdCount == creaseVertValues.length())) {
            return tags;
        }

        MUintArray   creaseEdgeIds;
        MDoubleArray creaseEdgeValues;
        mesh.getCreaseEdges(creaseEdgeIds, creaseEdgeValues);
        const auto creaseEdgeIdCount = creaseEdgeIds.length();
        if (!TF_VERIFY(creaseEdgeIdCount == creaseEdgeIds.length())) {
            return tags;
        }

        if (creaseVertIdCount > 0) {
            VtIntArray   cornerIndices(creaseVertIdCount);
            VtFloatArray cornerWeights(creaseVertIdCount);
            for (auto i = decltype(creaseVertIdCount) { 0 }; i < creaseVertIdCount; ++i) {
                cornerIndices[i] = static_cast<int>(creaseVertIds[i]);
                cornerWeights[i] = static_cast<float>(creaseVertValues[i]);
            }

            tags.SetCornerIndices(cornerIndices);
            tags.SetCornerWeights(cornerWeights);
        }

        // TODO: Do a similar compression to usdMaya:
        //  meshWrite_Subdiv.cpp:_CompressCreases.
        if (creaseEdgeIdCount > 0) {
            VtIntArray   edgeIndices(creaseEdgeIdCount * 2);
            VtFloatArray edgeWeights(creaseEdgeIdCount);
            int          edgeVertices[2] = { 0, 0 };
            for (auto i = decltype(creaseEdgeIdCount) { 0 }; i < creaseEdgeIdCount; ++i) {
                mesh.getEdgeVertices(creaseEdgeIds[i], edgeVertices);
                edgeIndices[i * 2] = edgeVertices[0];
                edgeIndices[i * 2 + 1] = edgeVertices[1];
                edgeWeights[i] = static_cast<float>(creaseEdgeValues[i]);
            }

            tags.SetCreaseIndices(edgeIndices);
            tags.SetCreaseLengths(VtIntArray(creaseEdgeIdCount, 2));
            tags.SetCreaseWeights(edgeWeights);
        }

        tags.SetVertexInterpolationRule(UsdGeomTokens->edgeAndCorner);
        tags.SetFaceVaryingInterpolationRule(UsdGeomTokens->cornersPlus1);
        tags.SetTriangleSubdivision(UsdGeomTokens->catmullClark);

        return tags;
#else
        return {};
#endif
    }

    HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation) override
    {
        if (interpolation == HdInterpolationVertex) {
            HdPrimvarDescriptor desc;
            desc.name = UsdGeomTokens->points;
            desc.interpolation = interpolation;
            desc.role = HdPrimvarRoleTokens->point;
            return { desc };
        } else if (interpolation == HdInterpolationFaceVarying) {
            // UVs are face varying in maya.
            MFnMesh mesh(GetDagPath());
            if (mesh.numUVs() > 0) {
                HdPrimvarDescriptor desc;
                desc.name = HdMayaAdapterTokens->st;
                desc.interpolation = interpolation;
                desc.role = HdPrimvarRoleTokens->textureCoordinate;
                return { desc };
            }
        }
        return {};
    }

    bool GetDoubleSided() override
    {
        MFnMesh mesh(GetDagPath());
        auto    p = mesh.findPlug(MayaAttrs::mesh::doubleSided, true);
        if (ARCH_UNLIKELY(p.isNull())) {
            return true;
        }
        bool doubleSided = true;
        p.getValue(doubleSided);
        return doubleSided;
    }

    bool HasType(const TfToken& typeId) const override { return typeId == HdPrimTypeTokens->mesh; }

private:
    static void NodeDirtiedCallback(MObject& node, MPlug& plug, void* clientData)
    {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        for (const auto& it : _dirtyBits) {
            if (it.first == plug) {
                adapter->MarkDirty(it.second);
                TF_DEBUG(HDMAYA_ADAPTER_MESH_PLUG_DIRTY)
                    .Msg(
                        "Marking prim dirty with bits %u because %s plug was "
                        "dirtied.\n",
                        it.second,
                        plug.partialName().asChar());
                return;
            }
        }

        TF_DEBUG(HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY)
            .Msg(
                "%s (%s) plug dirtying was not handled by "
                "HdMayaMeshAdapter::NodeDirtiedCallback.\n",
                plug.name().asChar(),
                plug.partialName().asChar());
    }

    // For material assignments for now.
    static void AttributeChangedCallback(
        MNodeMessage::AttributeMessage msg,
        MPlug&                         plug,
        MPlug&                         otherPlug,
        void*                          clientData)
    {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        if (plug == MayaAttrs::mesh::instObjGroups) {
            adapter->MarkDirty(HdChangeTracker::DirtyMaterialId);
        } else {
            TF_DEBUG(HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY)
                .Msg(
                    "%s (%s) plug dirtying was not handled by "
                    "HdMayaMeshAdapter::attributeChangedCallback.\n",
                    plug.name().asChar(),
                    plug.name().asChar());
        }
    }

    static void TopologyChangedCallback(MObject& node, void* clientData)
    {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        adapter->MarkDirty(
            HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPrimvar
            | HdChangeTracker::DirtyPoints);
    }

    static void ComponentIdChanged(MUintArray componentIds[], unsigned int count, void* clientData)
    {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        adapter->MarkDirty(
            HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPrimvar
            | HdChangeTracker::DirtyPoints);
    }

    static void UVSetChangedCallback(
        MObject&                  node,
        const MString&            name,
        MPolyMessage::MessageType type,
        void*                     clientData)
    {
        // TODO: Only track the uvset we care about.
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        adapter->MarkDirty(HdChangeTracker::DirtyPrimvar);
    }

    // Maya has a bug with removing some MPolyMessage callbacks. Known
    // problem callbacks include:
    //     MPolyMessage::addPolyComponentIdChangedCallback
    //     MPolyMessage::addUVSetChangedCallback
    // Reproduction code can be found here:
    //    https://gist.github.com/elrond79/668d9809873125f608e0f7360fff7fac
    // To work around this, we register these callbacks specially, and only
    // remove them if the underlying node is currently valid.
    MCallbackIdArray _buggyCallbacks;
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaMeshAdapter, TfType::Bases<HdMayaShapeAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, mesh)
{
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken("mesh"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(new HdMayaMeshAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
