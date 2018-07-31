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
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>

#include <hdmaya/mayaAttrs.h>
#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/shapeAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(_tokens, (st));

// When this code is loaded, maya may not be initialized - delay setting the
// MObjects...
bool _dirtyBitsInitialized = false;

std::vector<std::pair<MObject, HdDirtyBits>> _dirtyBits = {
    {MObject(), // Will hold "pnts" attribute when initialized
     HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent},
    {MObject(), // Will hold "inMesh" attribute when initialized
     HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent | HdChangeTracker::DirtyPrimvar |
         HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyNormals},
    {MObject(), // Will hold "worldMatrix" attribute when initialized
     HdChangeTracker::DirtyTransform},
    {MObject(), // Will hold "doubleSided" attribute when initialized
     HdChangeTracker::DirtyDoubleSided},
    {MObject(), // Will hold "intermediateObject" attribute when initialized
     HdChangeTracker::DirtyVisibility},
};

} // namespace

class HdMayaMeshAdapter : public HdMayaShapeAdapter {
   public:
    HdMayaMeshAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaShapeAdapter(delegate->GetPrimPath(dag), delegate, dag) {
        // Do this here just because we want to wait for a point in time where
        // we KNOW maya is initialized
        if (!_dirtyBitsInitialized) {
            // Don't have a mutex here, should be fine - worst case we have two
            // threads setting these to the same thing at the same time
            _dirtyBits[0].first = MayaAttrs::pnts;
            _dirtyBits[1].first = MayaAttrs::inMesh;
            _dirtyBits[2].first = MayaAttrs::worldMatrix;
            _dirtyBits[3].first = MayaAttrs::doubleSided;
            _dirtyBits[4].first = MayaAttrs::intermediateObject;
            _dirtyBitsInitialized = true;
        }
    }

    void Populate() override {
        GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), HdChangeTracker::AllDirty);

        MStatus status;
        // Duh!
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyCallback(obj, NodeDirtiedCallback, this, &status);
        if (status) { AddCallback(id); }
        id =
            MNodeMessage::addAttributeChangedCallback(obj, AttributeChangedCallback, this, &status);
        if (status) { AddCallback(id); }
    }

    bool IsSupported() override {
        return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->mesh);
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
            const auto* rawPoints = reinterpret_cast<const GfVec3f*>(mesh.getRawPoints(&status));
            if (!status) { return {}; }
            VtVec3fArray ret;
            ret.assign(rawPoints, rawPoints + mesh.numVertices());
            return VtValue(ret);
        }
        if (key == _tokens->st) {
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

    HdMeshTopology GetMeshTopology() override {
        MFnMesh mesh(GetDagPath());
        const auto numPolygons = mesh.numPolygons();
        VtIntArray faceVertexCounts;
        faceVertexCounts.reserve(static_cast<size_t>(numPolygons));
        VtIntArray faceVertexIndices;
        faceVertexIndices.reserve(static_cast<size_t>(mesh.numFaceVertices()));
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
            GetDelegate()->GetParams().displaySmoothMeshes ? PxOsdOpenSubdivTokens->catmullClark
                                                           : PxOsdOpenSubdivTokens->none,
            UsdGeomTokens->rightHanded, faceVertexCounts, faceVertexIndices);
    }

    HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation) override {
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

    bool GetDoubleSided() {
        MFnMesh mesh(GetDagPath());
        auto p = mesh.findPlug("doubleSided", true);
        if (ARCH_UNLIKELY(p.isNull())) { return true; }
        bool doubleSided;
        p.getValue(doubleSided);
        return doubleSided;
    }

    bool HasType(const TfToken& typeId) override { return typeId == HdPrimTypeTokens->mesh; }

   private:
    static void NodeDirtiedCallback(MObject& node, MPlug& plug, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        for (const auto& it : _dirtyBits) {
            if (it.first == plug) {
                adapter->MarkDirty(it.second);
                TF_DEBUG(HDMAYA_ADAPTER_MESH_PLUG_DIRTY)
                    .Msg(
                        "Marking prim dirty with bits %u because %s plug was dirtied.\n", it.second,
                        plug.partialName().asChar());
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
        MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaMeshAdapter*>(clientData);
        if (plug == MayaAttrs::instObjGroups) {
            adapter->MarkDirty(HdChangeTracker::DirtyMaterialId);
        } else {
            TF_DEBUG(HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY)
                .Msg(
                    "%s (%s) plug dirtying was not handled by "
                    "HdMayaMeshAdapter::attributeChangedCallback.\n",
                    plug.name().asChar(), plug.name().asChar());
        }
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaMeshAdapter, TfType::Bases<HdMayaShapeAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, mesh) {
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken("mesh"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(new HdMayaMeshAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
