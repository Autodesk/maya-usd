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
#ifdef LUMA_USD_BUILD
#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>

#include <pxr/usd/usdGeom/imagePlane.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/pxOsd/tokens.h>

#include <maya/MFloatArray.h>
#include <maya/MFnCamera.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MNodeClass.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MRenderUtil.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/mayaAttrs.h>
#include <hdmaya/adapters/shapeAdapter.h>

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

constexpr float inch_to_mm = 25.4f;

TF_DEFINE_PRIVATE_TOKENS(_tokens, (st));

// Simplest right handed vertex counts and vertex indices.
const VtIntArray _faceVertexCounts = {3, 3};

const VtIntArray _faceVertexIndices = {0, 1, 2, 0, 2, 3};

const VtIntArray _holeIndices;

void _ImagePlaneNodeDirtiedCallback(
    MObject& node, MPlug& plug, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_DAG_PLUG_DIRTY)
        .Msg(
            "Image plane adapter marking prim (%s) dirty because %s plug was "
            "dirtied.\n",
            adapter->GetID().GetText(), plug.partialName().asChar());
    if (plug == MayaAttrs::dagNode::worldMatrix) {
        adapter->MarkDirty(HdChangeTracker::DirtyTransform);
    } else {
        adapter->MarkDirty(
            HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent |
            HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyTopology |
            HdChangeTracker::DirtyNormals);
    }
}

void _CameraNodeDirtiedCallback(MObject& node, MPlug& plug, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_DAG_PLUG_DIRTY)
        .Msg(
            "Camera adapter marking prim (%s) dirty because %s plug was "
            "dirtied.\n",
            adapter->GetID().GetText(), plug.partialName().asChar());
    adapter->MarkDirty(
        HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyPoints);
}

} // namespace

class HdMayaImagePlaneAdapter : public HdMayaShapeAdapter {
private:
    MObject _camera;
    VtVec3fArray _vertices;
    VtVec2fArray _uvs;
    bool _planeIsDirty = true;

public:
    HdMayaImagePlaneAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaShapeAdapter(delegate->GetPrimPath(dag), delegate, dag) {
        // find the camera attached to the image plane
        _camera = MObject::kNullObj;
        MFnDagNode fn(dag.node());
        MStatus status;
        MPlug messagePlug = fn.findPlug(MayaAttrs::node::message, true, &status);
        MPlugArray referencePlugs;
        messagePlug.connectedTo(referencePlugs, false, true);
        for (uint32_t i = 0, n = referencePlugs.length(); i < n; ++i) {
            MObject temp = referencePlugs[i].node();
            if (temp.hasFn(MFn::kCamera)) { _camera = temp; }
        }
        if (_camera.isNull()) {
            TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
                .Msg(
                    "imagePlane %s is not linked to a camera\n",
                    fn.fullPathName().asChar());
        }
    }

    ~HdMayaImagePlaneAdapter() {}

    void Populate() {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg(
                "Called HdMayaImagePlaneAdapter::Populate() - %s\n",
                GetDagPath().partialPathName().asChar());

        GetDelegate()->InsertRprim(
            HdPrimTypeTokens->mesh, GetID(), HdChangeTracker::AllDirty);

        MStatus status;
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyPlugCallback(
            obj, _ImagePlaneNodeDirtiedCallback, this, &status);
        if (status) { AddCallback(id); }

        if (!_camera.isNull()) {
            id = MNodeMessage::addNodeDirtyPlugCallback(
                _camera, _CameraNodeDirtiedCallback, this, &status);
            if (status) { AddCallback(id); }
        }
    }

    // note: could remove if made a header for meshAdapter
    bool IsSupported() override {
        return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(
            HdPrimTypeTokens->mesh);
    }

    bool HasType(const TfToken& typeId) override {
        return typeId == HdPrimTypeTokens->mesh;
    }

    HdPrimvarDescriptorVector GetPrimvarDescriptors(
        HdInterpolation interpolation) override {
        if (interpolation == HdInterpolationVertex) {
            HdPrimvarDescriptor desc;
            desc.name = UsdGeomTokens->points;
            desc.interpolation = interpolation;
            desc.role = HdPrimvarRoleTokens->point;

            // Our created uvs are simple and just per point, rather than face
            // varying like maya meshes
            HdPrimvarDescriptor desc2;
            desc2.name = _tokens->st;
            desc2.interpolation = interpolation;
            desc2.role = HdPrimvarRoleTokens->textureCoordinate;
            return {desc, desc2};
        }
        return {};
    }

    void UpdateGeometry() {
        if (!_planeIsDirty) { return; }
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg(
                "HdMayaImagePlaneAdapter::UpdateGeometry - %s\n",
                GetDagPath().partialPathName().asChar());

        UsdGeomImagePlane::ImagePlaneParams paramsFromMaya;
        // get imagePlane attributes
        MFnDagNode dnode(GetDagPath().node());

        auto imageNameExtracted =
            MRenderUtil::exactImagePlaneFileName(dnode.object());
        const SdfAssetPath imageNameExtractedPath(
            std::string(imageNameExtracted.asChar()));
        paramsFromMaya.fileName = imageNameExtractedPath;

        const auto coveragePlug = dnode.findPlug(
                MayaAttrs::imagePlane::coverage, true);
        paramsFromMaya.coverage = GfVec2i(
            coveragePlug.child(0).asInt(), coveragePlug.child(1).asInt());
        const auto coverageOriginPlug = dnode.findPlug(
                MayaAttrs::imagePlane::coverageOrigin, true);
        paramsFromMaya.coverageOrigin = GfVec2i(
            coverageOriginPlug.child(0).asInt(),
            coverageOriginPlug.child(1).asInt());

        MStatus status;
        if (!_camera.isNull()) {
            MFnCamera camera(_camera);
            paramsFromMaya.aperture[0] = camera.horizontalFilmAperture(&status);
            paramsFromMaya.aperture[1] = camera.verticalFilmAperture(&status);
            paramsFromMaya.focalLength = camera.focalLength(&status);

            // enabled attributes when connected to camera
            paramsFromMaya.depth = dnode.findPlug(
                    MayaAttrs::imagePlane::depth, true).asFloat();
            paramsFromMaya.rotate = dnode.findPlug(
                    MayaAttrs::imagePlane::rotate, true).asFloat();

            const auto fit = dnode.findPlug(
                    MayaAttrs::imagePlane::fit, true).asShort();
            if (fit == UsdGeomImagePlane::FIT_BEST) {
                paramsFromMaya.fit = UsdGeomTokens->best;
            } else if (fit == UsdGeomImagePlane::FIT_FILL) {
                paramsFromMaya.fit = UsdGeomTokens->fill;
            } else if (fit == UsdGeomImagePlane::FIT_HORIZONTAL) {
                paramsFromMaya.fit = UsdGeomTokens->horizontal;
            } else if (fit == UsdGeomImagePlane::FIT_VERTICAL) {
                paramsFromMaya.fit = UsdGeomTokens->vertical;
            } else if (fit == UsdGeomImagePlane::FIT_TO_SIZE) {
                paramsFromMaya.fit = UsdGeomTokens->toSize;
            }

            // always enabled but only affect when connected to camera
            const auto sizePlug = dnode.findPlug(
                    MayaAttrs::imagePlane::size, true);
            // Size attr is in inches while aperture is in millimeters.
            paramsFromMaya.size =
                GfVec2f(
                    sizePlug.child(0).asFloat(), sizePlug.child(1).asFloat()) *
                inch_to_mm;
            const auto offsetPlug = dnode.findPlug(
                    MayaAttrs::imagePlane::offset, true);
            paramsFromMaya.offset = GfVec2f(
                                        offsetPlug.child(0).asFloat(),
                                        offsetPlug.child(1).asFloat()) *
                                    inch_to_mm;

        } else {
            //  if no camera maya gets size from height and width
            paramsFromMaya.size = GfVec2f(
                dnode.findPlug(MayaAttrs::imagePlane::width, true).asFloat(),
                dnode.findPlug(MayaAttrs::imagePlane::height, true).asFloat());
            // if no camera, fit wont affect size, and this fit value uses
            // size without any modifications
            paramsFromMaya.fit = UsdGeomTokens->toSize;
            // maya uses a center attribute as a 3d offset, but we can also
            // express this with depth + offset since those other attributes
            // do not affect non camera image planes.
            const auto centerPlug = dnode.findPlug(
                    MayaAttrs::imagePlane::imageCenter, true);
            paramsFromMaya.offset = GfVec2f(
                centerPlug.child(0).asFloat(), centerPlug.child(1).asFloat());
            paramsFromMaya.depth = -centerPlug.child(2).asFloat();
            // need to zero out focal length to prevent projection of depth
            paramsFromMaya.focalLength = 0.0f;
        }

        UsdGeomImagePlane::CalculateGeometry(&_vertices, &_uvs, paramsFromMaya);
        _planeIsDirty = false;
    }

    VtValue Get(const TfToken& key) override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg(
                "Called HdMayaImagePlaneAdapter::Get(%s) - %s\n", key.GetText(),
                GetDagPath().partialPathName().asChar());

        if (key == HdTokens->points) {
            UpdateGeometry();
            return VtValue(_vertices);
        } else if (key == _tokens->st) {
            UpdateGeometry();
            return VtValue(_uvs);
        }
        return {};
    }

    HdMeshTopology GetMeshTopology() override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg(
                "Called HdMayaImagePlaneAdapter::GetMeshTopology - %s\n",
                GetDagPath().partialPathName().asChar());
        return HdMeshTopology(
            UsdGeomTokens->triangleSubdivisionRule,
            // TODO: Would it be cleaner to flip the xform instead?
            // Without this, the normal is facing away from camera.
            // UsdGeomTokens->rightHanded,
            UsdGeomTokens->leftHanded, _faceVertexCounts, _faceVertexIndices,
            _holeIndices, 0);
    }

    MObject GetMaterial() override {
        // return image plane itself
        MStatus status;
        MFnDagNode dagNode(GetDagPath(), &status);
        if (!status) { return MObject::kNullObj; }
        return dagNode.object();
    }

    void MarkDirty(HdDirtyBits dirtyBits) {
        if (dirtyBits & HdChangeTracker::DirtyPoints) { _planeIsDirty = true; }
        HdMayaShapeAdapter::MarkDirty(dirtyBits);
    }
}; // HdMayaImagePlaneAdapter

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaImagePlaneAdapter, TfType::Bases<HdMayaShapeAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, imagePlane) {
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken("imagePlane"),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(
                new HdMayaImagePlaneAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif // LUMA_USD_BUILD
