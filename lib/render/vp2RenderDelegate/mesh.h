//
// Copyright 2019 Autodesk
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

#ifndef HD_VP2_MESH
#define HD_VP2_MESH

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"

#include "render_delegate.h"

#include <maya/MHWGeometry.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2DrawItem;
struct HdMeshReprDesc;

/*! \brief  VP2 representation of poly-mesh object.
    \class  HdVP2Mesh

    The prim object's main function is to bridge the scene description and the
    renderable representation. The Hydra image generation algorithm will call
    HdRenderIndex::SyncAll() before any drawing; this, in turn, will call
    Sync() for each mesh with new data.

    Sync() is passed a set of dirtyBits, indicating which scene buffers are
    dirty. It uses these to pull all of the new scene data and constructs
    updated embree geometry objects.  Commit of changed buffers to GPU happens
    in HdVP2RenderDelegate::CommitResources(), which runs on main-thread after
    all prims have been updated.
*/
class HdVP2Mesh final : public HdMesh {
public:
    HdVP2Mesh(HdVP2RenderDelegate*, const SdfPath&, const SdfPath& instancerId = SdfPath());

    ~HdVP2Mesh() override;

    void Sync(
        HdSceneDelegate*, HdRenderParam*,
        HdDirtyBits*, const TfToken& reprToken) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits) const override;

    void _InitRepr(const TfToken&, HdDirtyBits*) override;

    void _UpdateRepr(HdSceneDelegate*, const TfToken&, HdDirtyBits*);

    void _UpdateDrawItem(
        HdSceneDelegate*, HdVP2DrawItem*, HdDirtyBits*,
        const HdMeshReprDesc&, bool requireSmoothNormals, bool requireFlatNormals);

    bool _EnableWireDrawItems(const HdReprSharedPtr& repr, HdDirtyBits* dirtyBits, bool enable);

    void _UpdatePrimvarSources(HdSceneDelegate* sceneDelegate, HdDirtyBits dirtyBits);

    MHWRender::MRenderItem* _CreateRenderItem(const MString& name, const HdMeshReprDesc& desc) const;
    MHWRender::MRenderItem* _CreateSmoothHullRenderItem(const MString& name) const;
    MHWRender::MRenderItem* _CreateWireframeRenderItem(const MString& name) const;
    MHWRender::MRenderItem* _CreatePointsRenderItem(const MString& name) const;

    //! Custom dirty bits used by this mesh
    enum DirtyBits : HdDirtyBits {
        DirtySmoothNormals = HdChangeTracker::CustomBitsBegin,
        DirtyFlatNormals = (DirtySmoothNormals << 1),
        DirtyIndices = (DirtyFlatNormals << 1),
        DirtyHullIndices = (DirtyIndices << 1),
        DirtyPointsIndices = (DirtyHullIndices << 1),
        DirtyBoundingBox = (DirtyPointsIndices << 1)
    };
    
    HdVP2RenderDelegate* _delegate{ nullptr };          //!< VP2 render delegate for which this mesh was created
    HdDirtyBits          _customDirtyBitsInUse{ 0 };    //!< Storage for custom dirty bits. See _PropagateDirtyBits for details.

    // TODO: Define HdVP2MeshSharedData to hold extra shared data specific to VP2?
    std::unique_ptr<MHWRender::MVertexBuffer> _positionsBuffer; //!< Per-Rprim position buffer to be shared among render items
    bool _wireItemsEnabled{ false };                    //!< Whether draw items for the wire repr are enabled

    //! Cached scene data. VtArrays are reference counted, so as long as we
    //! only call const accessors keeping them around doesn't incur a buffer
    //! copy.
    HdMeshTopology _topology;
    VtVec3fArray _points;

    //! A local cache of primvar scene data. "data" is a copy-on-write handle to
    //! the actual primvar buffer, and "interpolation" is the interpolation mode
    //! to be used.
    struct PrimvarSource {
        VtValue data;
        HdInterpolation interpolation;
    };
    TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> _primvarSourceMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
