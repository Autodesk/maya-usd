//
// Copyright 2020 Autodesk
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

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <pxr/imaging/hd/mesh.h>
#include <pxr/pxr.h>

#include <maya/MHWGeometry.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2DrawItem;
class HdVP2RenderDelegate;

//! Primvar data and interpolation.
struct PrimvarSource
{
    VtValue         data;
    HdInterpolation interpolation;
};

//! A hash map of primvar scene data using primvar name as the key.
typedef TfHashMap<TfToken, PrimvarSource, TfToken::HashFunctor> PrimvarSourceMap;

/*! \brief  HdVP2Mesh-specific data shared among all its draw items.
    \class  HdVP2MeshSharedData

    A Rprim can have multiple draw items. The shared data are extracted from
    USD scene delegate during synchronization. Then each draw item can prepare
    draw data from these shared data as needed.
*/
struct HdVP2MeshSharedData
{
    //! Cached scene topology. VtArrays are reference counted, so as long as we
    //! only call const accessors keeping them around doesn't incur a buffer
    //! copy.
    HdMeshTopology _topology;

    //! The rendering topology is to create unshared or sorted vertice layout
    //! for efficient GPU rendering.
    HdMeshTopology _renderingTopology;

    //! An array to store original scene face vertex index of each rendering
    //! face vertex index.
    VtIntArray _renderingToSceneFaceVtxIds;

    //! The number of vertices in each vertex buffer.
    size_t _numVertices;

    //! A local cache of primvar scene data. "data" is a copy-on-write handle to
    //! the actual primvar buffer, and "interpolation" is the interpolation mode
    //! to be used.
    PrimvarSourceMap _primvarSourceMap;

    //! A local cache of points. It is not cached in the above primvar map
    //! but a separate VtArray for easier access.
    VtVec3fArray _points;

    //! Position buffer of the Rprim to be shared among all its draw items.
    std::unique_ptr<MHWRender::MVertexBuffer> _positionsBuffer;

    //! Render tag of the Rprim.
    TfToken _renderTag;
};

/*! \brief  VP2 representation of poly-mesh object.
    \class  HdVP2Mesh

    The prim object's main function is to bridge the scene description and the
    renderable representation. The Hydra image generation algorithm will call
    HdRenderIndex::SyncAll() before any drawing; this, in turn, will call
    Sync() for each mesh with new data.

    Sync() is passed a set of dirtyBits, indicating which scene buffers are
    dirty. It uses these to pull all of the new scene data and constructs
    updated geometry objects.  Commit of changed buffers to GPU happens
    in HdVP2RenderDelegate::CommitResources(), which runs on main-thread after
    all prims have been updated.
*/
class HdVP2Mesh final : public HdMesh
{
public:
    HdVP2Mesh(HdVP2RenderDelegate*, const SdfPath&, const SdfPath& instancerId = SdfPath());

    //! Destructor.
    ~HdVP2Mesh() override = default;

    void Sync(HdSceneDelegate*, HdRenderParam*, HdDirtyBits*, const TfToken& reprToken) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits) const override;

    void _InitRepr(const TfToken&, HdDirtyBits*) override;

    void _UpdateRepr(HdSceneDelegate*, const TfToken&);

    void _UpdateDrawItem(
        HdSceneDelegate*,
        HdVP2DrawItem*,
        const HdMeshReprDesc& desc,
        bool                  requireSmoothNormals,
        bool                  requireFlatNormals);

    void _HideAllDrawItems(const TfToken& reprToken);

    void _UpdatePrimvarSources(
        HdSceneDelegate*     sceneDelegate,
        HdDirtyBits          dirtyBits,
        const TfTokenVector& requiredPrimvars);

    MHWRender::MRenderItem* _CreateSelectionHighlightRenderItem(const MString& name) const;
    MHWRender::MRenderItem* _CreateSmoothHullRenderItem(const MString& name) const;
    MHWRender::MRenderItem* _CreateWireframeRenderItem(const MString& name) const;
    MHWRender::MRenderItem* _CreatePointsRenderItem(const MString& name) const;
    MHWRender::MRenderItem* _CreateBoundingBoxRenderItem(const MString& name) const;

    //! Custom dirty bits used by this mesh
    enum DirtyBits : HdDirtyBits
    {
        DirtySmoothNormals = HdChangeTracker::CustomBitsBegin,
        DirtyFlatNormals = (DirtySmoothNormals << 1),
        DirtyIndices = (DirtyFlatNormals << 1),
        DirtyHullIndices = (DirtyIndices << 1),
        DirtyPointsIndices = (DirtyHullIndices << 1),
        DirtySelection = (DirtyPointsIndices << 1),
        DirtySelectionHighlight = (DirtySelection << 1)
    };

    HdVP2RenderDelegate* _delegate {
        nullptr
    }; //!< VP2 render delegate for which this mesh was created
    HdDirtyBits _customDirtyBitsInUse {
        0
    };                      //!< Storage for custom dirty bits. See _PropagateDirtyBits for details.
    const MString _rprimId; //!< Rprim id cached as a maya string for easier debugging and profiling
    HdVP2MeshSharedData _meshSharedData; //!< Shared data for all draw items of the Rprim

    //! Selection status of the Rprim
    HdVP2SelectionStatus _selectionStatus { kUnselected };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
