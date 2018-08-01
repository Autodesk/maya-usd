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
#include <pxr/usd/usdGeom/imagePlane.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/pxOsd/tokens.h>

#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnCamera.h>
#include <maya/MIntArray.h>
#include <maya/MNodeClass.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MRenderUtil.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/shapeAdapter.h>

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"


PXR_NAMESPACE_OPEN_SCOPE

namespace {

constexpr float inch_to_mm = 25.4f;

TF_DEFINE_PRIVATE_TOKENS(_tokens, (st));

// Simplest right handed vertex counts and vertex indices.
const VtIntArray _faceVertexCounts = {
        3, 3
};

const VtIntArray _faceVertexIndices = {
        0, 1, 2, 0, 2, 3
};

const VtIntArray _holeIndices;

// TODO: Refine callback granularity
void _ImagePlaneNodeDirtiedCallback(MObject& node, MPlug& plug, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_DAG_PLUG_DIRTY).Msg(
                "Image plane adapter marking prim (%s) dirty because %s plug was dirtied.\n",
                adapter->GetID().GetText(), plug.partialName().asChar());
    // if wm changes
    adapter->MarkDirty(HdChangeTracker::DirtyTransform |
                       HdChangeTracker::DirtyPoints |
                       HdChangeTracker::DirtyExtent |
                       HdChangeTracker::DirtyPrimvar |
                       HdChangeTracker::DirtyTopology |
                       HdChangeTracker::DirtyNormals);
}

void _CameraNodeDirtiedCallback(MObject& node, MPlug& plug, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    TF_DEBUG(HDMAYA_ADAPTER_DAG_PLUG_DIRTY).Msg(
                "Camera adapter marking prim (%s) dirty because %s plug was dirtied.\n",
                adapter->GetID().GetText(), plug.partialName().asChar());
    adapter->MarkDirty(HdChangeTracker::DirtyTransform |
                       HdChangeTracker::DirtyPoints);
}

} // namespace


// TODO: Move inside usd imagePlane schema
struct _ImagePlaneParams {
    float depth = 100.0f;
    GfVec2f aperture {1.0f, 1.0f};
    GfVec2f offset {0.0f, 0.0f};
    float focalLength = 1.0f;
    GfVec2f size {-1.0f, -1.0f};
    GfVec2f coverage {-1.0f, -1.0f};
    GfVec2f coverageOrigin {0, 0};
    float rotate = 0.0f;
    GfVec2f imageSize {100.0f, 100.0f};
    SdfAssetPath fileName {""};
    TfToken fit = UsdGeomImagePlaneFitTokens->best;
};

void
CalculateGeometryForCamera(
        VtVec3fArray* vertices,
        VtVec2fArray* uvs,
        _ImagePlaneParams params) {
    if (ARCH_UNLIKELY(vertices == nullptr)) { return; }
    // The trick here is to take the image plane size (if not valid the camera aperture),
    // and try to fit the aperture to the image ratio, based on the fit parameter on the image plane.
    // We don't need the viewport aspect ratio / size, because it's already affecting the image by
    // affecting the projection matrix.

    if (params.size[0] <= 0.0f) {
        params.size[0] = params.aperture[0];
    }
    if (params.size[1] <= 0.0f) {
        params.size[1] = params.aperture[1];
    }

    // Doesn't matter where we divide, we'll just multiply values anyway.
    params.size[0] *= 0.5f;
    params.size[1] *= 0.5f;

    GfVec2f imageSize {100.0f, 100.0f};
    {
//        auto* in = OIIO::ImageInput::open(fileName.GetResolvedPath());
//        if (in) {
//            in->close();
//            const auto& spec = in->spec();
//            imageSize[0] = static_cast<float>(spec.width);
//            imageSize[1] = static_cast<float>(spec.height);
//            OIIO::ImageInput::destroy(in);
//        }
    }
    const auto imageRatio = imageSize[0] / imageSize[1];
    const auto sizeRatio = params.size[0] / params.size[1];

    if (params.fit == UsdGeomImagePlaneFitTokens->fill) {
        if (imageRatio > sizeRatio) {
            params.size[0] = params.size[1] * imageRatio;
        } else {
            params.size[1] = params.size[0] / imageRatio;
        }
    } else if (params.fit == UsdGeomImagePlaneFitTokens->best) {
        if (imageRatio > sizeRatio) {
            params.size[1] = params.size[0] / imageRatio;
        } else {
            params.size[0] = params.size[1] * imageRatio;
        }
    } else if (params.fit == UsdGeomImagePlaneFitTokens->horizontal) {
        params.size[1] = params.size[0] / imageRatio;
    } else if (params.fit == UsdGeomImagePlaneFitTokens->vertical) {
        params.size[0] = params.size[1] * imageRatio;
    } else if (params.fit == UsdGeomImagePlaneFitTokens->toSize) {
    } else { assert("Invalid value passed to UsdGeomImagePlane.fit!"); }

    GfVec2f upperLeft  { -params.size[0],  params.size[1]};
    GfVec2f upperRight {  params.size[0],  params.size[1]};
    GfVec2f lowerLeft  { -params.size[0], -params.size[1]};
    GfVec2f lowerRight {  params.size[0], -params.size[1]};

    if (!GfIsClose(params.rotate, 0.0f, 0.001f)) {
        const float rsin = sinf(-params.rotate);
        const float rcos = cosf(-params.rotate);

        auto rotateCorner = [rsin, rcos] (GfVec2f& corner) {
            const float t = corner[0] * rcos - corner[1] * rsin;
            corner[1] = corner[0] * rsin + corner[1] * rcos;
            corner[0] = t;
        };

        rotateCorner(upperLeft);
        rotateCorner(upperRight);
        rotateCorner(lowerLeft);
        rotateCorner(lowerRight);
    }

    // FIXME: Offset doesn't work properly!
    upperLeft  += params.offset;
    upperRight += params.offset;
    lowerLeft  += params.offset;
    lowerRight += params.offset;
    // Both aperture and focal length should be in millimeters,
    auto projectVertex = [params] (GfVec2f& vertex) {
        vertex[0] = params.depth * vertex[0] / params.focalLength;
        vertex[1] = params.depth * vertex[1] / params.focalLength;
    };

    if (params.depth != 0.0f and params.focalLength != 0.0f){
        projectVertex(upperLeft);
        projectVertex(upperRight);
        projectVertex(lowerLeft);
        projectVertex(lowerRight);
    }

    vertices->resize(4);
    vertices->operator[](0) = GfVec3f(upperLeft[0] , upperLeft[1] , -params.depth);
    vertices->operator[](1) = GfVec3f(upperRight[0], upperRight[1], -params.depth);
    vertices->operator[](2) = GfVec3f(lowerRight[0], lowerRight[1], -params.depth);
    vertices->operator[](3) = GfVec3f(lowerLeft[0] , lowerLeft[1] , -params.depth);

    auto lerp = [] (float v, float lo, float hi) -> float {
        return lo * (1.0f - v) + hi * v;
    };

    auto clamp = [] (float v, float lo, float hi) -> float {
        if (v < lo) { return lo; }
        else if (v > hi) { return hi; }
        return v;
    };

    if (ARCH_UNLIKELY(uvs == nullptr)) { return; }
    if (params.coverage[0] <= 0.0f) {
        params.coverage[0] = imageSize[0];
    }
    if (params.coverage[1] <= 0.0f) {
        params.coverage[1] = imageSize[1];
    }
    params.coverage[0] = clamp(params.coverage[0], 0.0f, imageSize[0]);
    params.coverage[1] = clamp(params.coverage[1], 0.0f, imageSize[1]);
    params.coverageOrigin[0] = clamp(params.coverageOrigin[0], -imageSize[0], imageSize[0]);
    params.coverageOrigin[1] = clamp(params.coverageOrigin[1], -imageSize[1], imageSize[1]);

    GfVec2f minUV = {0.0f, 0.0f};
    GfVec2f maxUV = {1.0f, 1.0f};

    if (params.coverageOrigin[0] > 0) {
        minUV[0] = params.coverageOrigin[0] / imageSize[0];
        maxUV[0] = lerp(std::min(params.coverage[0], imageSize[0] - params.coverageOrigin[0]) /
                        (imageSize[0] - params.coverageOrigin[0]), minUV[0], 1.0f);
    } else if (params.coverageOrigin[0] < 0) {
        maxUV[0] = params.coverage[0] * (imageSize[0] + params.coverageOrigin[0]) / (imageSize[0] * imageSize[0]);
    } else {
        maxUV[0] = params.coverage[0] / imageSize[0];
    }

    if (params.coverageOrigin[1] > 0) {
        maxUV[1] = (imageSize[1] - params.coverageOrigin[1]) / imageSize[1];
        minUV[1] = lerp(std::min(params.coverage[1], imageSize[1] - params.coverageOrigin[1]) /
                        (imageSize[1] - params.coverageOrigin[1]), maxUV[1], 0.0f);
    } else if (params.coverageOrigin[1] < 0) {
        minUV[1] = std::min(1.0f, -params.coverageOrigin[1] / imageSize[1] +
                                  (1.0f - params.coverage[1] / imageSize[1]));
    } else {
        minUV[1] = 1.0f - params.coverage[1] / imageSize[1];
    }

    uvs->resize(4);
    uvs->operator[](0) = GfVec2f(minUV[0], minUV[1]);
    uvs->operator[](1) = GfVec2f(maxUV[0], minUV[1]);
    uvs->operator[](2) = GfVec2f(maxUV[0], maxUV[1]);
    uvs->operator[](3) = GfVec2f(minUV[0], maxUV[1]);
}

class HdMayaImagePlaneAdapter : public HdMayaShapeAdapter {

private:
    MObject _camera;
    VtVec3fArray _vertices;
    VtVec2fArray _uvs;
    bool _planeIsDirty = true;

public:
    HdMayaImagePlaneAdapter(HdMayaDelegateCtx *delegate, const MDagPath &dag)
            : HdMayaShapeAdapter(delegate->GetPrimPath(dag), delegate, dag) {

        // find the camera attached to the image plane
        _camera = MObject::kNullObj;
        MFnDagNode fn(dag.node());
        MStatus status;
        MPlug messagePlug = fn.findPlug("message", &status);
        MPlugArray referencePlugs;
        messagePlug.connectedTo(referencePlugs, false, true);
        for(uint32_t i = 0, n = referencePlugs.length(); i < n; ++i)
        {
            MObject temp = referencePlugs[i].node();
            if(temp.hasFn(MFn::kCamera)) {
                _camera = temp;
            }
        }
        if (_camera.isNull()){
            TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES).Msg(
                    "imagePlane %s is not linked to a camera\n",
                    fn.fullPathName().asChar());
        }
    }

    ~HdMayaImagePlaneAdapter() {
    }

    void Populate() {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES).Msg(
                    "Called HdMayaImagePlaneAdapter::Populate() - %s\n",
                    GetDagPath().partialPathName().asChar());

        GetDelegate()->InsertRprim(HdPrimTypeTokens->mesh, GetID(), HdChangeTracker::AllDirty);

        MStatus status;
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyCallback(obj, _ImagePlaneNodeDirtiedCallback, this, &status);
        if (status) { AddCallback(id); }

        if (!_camera.isNull()){
            id = MNodeMessage::addNodeDirtyCallback(_camera, _CameraNodeDirtiedCallback, this, &status);
            if (status) { AddCallback(id); }
        }
    }

    // note: could remove if made a header for meshAdapter
    bool IsSupported() override {
        return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(HdPrimTypeTokens->mesh);
    }

    bool HasType(const TfToken& typeId) override { return typeId == HdPrimTypeTokens->mesh; }

    HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation) override {
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

    void UpdateGeometry(){
        if (!_planeIsDirty){ return; }
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("HdMayaImagePlaneAdapter::UpdateGeometry - %s\n",
                 GetDagPath().partialPathName().asChar());

        _ImagePlaneParams paramsFromMaya;
        // get imagePlane attributes
        MFnDagNode dnode(GetDagPath().node());

        auto imageNameExtracted = MRenderUtil::exactImagePlaneFileName(dnode.object());
        paramsFromMaya.fileName = SdfAssetPath(std::string(imageNameExtracted.asChar()));

        const auto fit = dnode.findPlug("fit").asShort();
        if (fit == UsdGeomImagePlane::FIT_BEST) {
            paramsFromMaya.fit = UsdGeomImagePlaneFitTokens->best;
        } else if (fit == UsdGeomImagePlane::FIT_FILL) {
            paramsFromMaya.fit = UsdGeomImagePlaneFitTokens->fill;
        } else if (fit == UsdGeomImagePlane::FIT_HORIZONTAL) {
            paramsFromMaya.fit = UsdGeomImagePlaneFitTokens->horizontal;
        } else if (fit == UsdGeomImagePlane::FIT_VERTICAL) {
            paramsFromMaya.fit = UsdGeomImagePlaneFitTokens->vertical;
        } else if (fit == UsdGeomImagePlane::FIT_TO_SIZE) {
            paramsFromMaya.fit = UsdGeomImagePlaneFitTokens->toSize;
        }

        const auto coveragePlug = dnode.findPlug("coverage");
        paramsFromMaya.coverage = GfVec2i(coveragePlug.child(0).asInt(), coveragePlug.child(1).asInt());
        const auto coverageOriginPlug = dnode.findPlug("coverageOrigin");
        paramsFromMaya.coverageOrigin = GfVec2i(coverageOriginPlug.child(0).asInt(), coverageOriginPlug.child(1).asInt());

        MStatus status;
        if (!_camera.isNull()) {
            MFnCamera camera(_camera);
            paramsFromMaya.aperture[0] = camera.horizontalFilmAperture(&status);
            paramsFromMaya.aperture[1] = camera.verticalFilmAperture(&status);
            paramsFromMaya.focalLength = camera.focalLength(&status);

            // enabled attributes when connected to camera
            paramsFromMaya.depth = dnode.findPlug("depth").asFloat();
            paramsFromMaya.rotate = dnode.findPlug("rotate").asFloat();

            // always enabled but only affect when connected to camera
            const auto sizePlug = dnode.findPlug("size");
            // Size attr is in inches while aperture is in millimeters.
            paramsFromMaya.size = GfVec2f(sizePlug.child(0).asFloat(), sizePlug.child(1).asFloat()) * inch_to_mm;
            const auto offsetPlug = dnode.findPlug("offset");
            paramsFromMaya.offset = GfVec2f(offsetPlug.child(0).asFloat(), offsetPlug.child(1).asFloat()) * inch_to_mm;
        } else {
            paramsFromMaya.size = GfVec2f(dnode.findPlug("width").asFloat(), dnode.findPlug("height").asFloat());
            // maya uses a center attribute as a 3d offset, but we can also
            // express this with depth + offset since those other attributes
            // do not affect non camera image planes.
            const auto centerPlug = dnode.findPlug("imageCenter");
            paramsFromMaya.offset = GfVec2f(centerPlug.child(0).asFloat(), centerPlug.child(1).asFloat());
            paramsFromMaya.depth = -centerPlug.child(2).asFloat();
            // need to zero out focal length to prevent projection of depth
            paramsFromMaya.focalLength = 0.0f;
        }

        CalculateGeometryForCamera(
                &_vertices,
                &_uvs,
                paramsFromMaya);
        _planeIsDirty = false;
    }

    VtValue Get(const TfToken &key) override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("Called HdMayaImagePlaneAdapter::Get(%s) - %s\n", key.GetText(),
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
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES).Msg(
                "Called HdMayaImagePlaneAdapter::GetMeshTopology - %s\n",
                GetDagPath().partialPathName().asChar());
        return HdMeshTopology(UsdGeomTokens->triangleSubdivisionRule,
                              // TODO: Would it be cleaner to flip the xform instead?
                              // Without this, the normal is facing away from camera.
                              // UsdGeomTokens->rightHanded,
                              UsdGeomTokens->leftHanded,
                              _faceVertexCounts,
                              _faceVertexIndices,
                              _holeIndices,
                              0);
    }

    MObject GetMaterial() override {
        // return image plane itself
        MStatus status;
        MFnDagNode dagNode(GetDagPath(), &status);
        if (!status) { return MObject::kNullObj; }
        return dagNode.object();
    }

    void MarkDirty(HdDirtyBits dirtyBits) {
        if (dirtyBits & HdChangeTracker::DirtyPoints){
            _planeIsDirty = true;
        }
        HdMayaShapeAdapter::MarkDirty(dirtyBits);
    }
}; // HdMayaImagePlaneAdapter


TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaImagePlaneAdapter, TfType::Bases<HdMayaShapeAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, imagePlane) {
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken("imagePlane"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(new HdMayaImagePlaneAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
