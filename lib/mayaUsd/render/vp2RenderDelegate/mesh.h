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

#include "draw_item.h"
#include "mayaPrimCommon.h"
#include "meshViewportCompute.h"
#include "primvarInfo.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/vertexAdjacency.h>
#include <pxr/pxr.h>

#include <maya/MHWGeometry.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdVP2DrawItem;
class HdVP2RenderDelegate;

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

    //! Adjacency based off of _topology
    Hd_VertexAdjacencySharedPtr _adjacency;

    //! The rendering topology is to create unshared or sorted vertice layout
    //! for efficient GPU rendering.
    HdMeshTopology _renderingTopology;

    //! Defines whether or not the vertex layout used for drawing is unshared
    bool _isVertexLayoutUnshared { false };

    //! An array to store original scene face vertex index of each rendering
    //! face vertex index.
    VtIntArray _renderingToSceneFaceVtxIds;

    //! An array to store a rendering face vertex index for each original scene
    //! face vertex index.
    std::vector<int> _sceneToRenderingFaceVtxIds;

    //! triangulation of the _renderingTopology
    VtVec3iArray _trianglesFaceVertexIndices;

    //! encoded triangleId to faceId of _trianglesFaceVertexIndices, use
    //! HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam when accessing.
    VtIntArray _primitiveParam;

    //! Map from the original topology faceId to the void* pointer to
    //! the MRenderItem that face is a part of
    std::vector<SdfPath> _faceIdToGeomSubsetId;

    //! The number of vertices in each vertex buffer.
    size_t _numVertices;

    //! The primvar tokens of all the smooth hull material bindings (overall object + geom subsets)
    TfTokenVector _allRequiredPrimvars;

    //! Cache of the primvar data on this mesh, along with the MVertexBuffer holding that data.
    PrimvarInfoMap _primvarInfo;

    //! Render tag of the Rprim.
    TfToken _renderTag;
#ifdef HDVP2_ENABLE_GPU_COMPUTE
    MSharedPtr<MeshViewportCompute> _viewportCompute;
#endif
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
class HdVP2Mesh final
    : public HdMesh
    , public MayaUsdRPrim
{
public:
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    HdVP2Mesh(HdVP2RenderDelegate*, const SdfPath&);
#else
    HdVP2Mesh(HdVP2RenderDelegate*, const SdfPath&, const SdfPath& instancerId = SdfPath());
#endif

    //! Destructor.
    ~HdVP2Mesh() override = default;

    void Sync(HdSceneDelegate*, HdRenderParam*, HdDirtyBits*, const TfToken& reprToken) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits) const override;

    void _InitRepr(const TfToken&, HdDirtyBits*) override;

    void _UpdateRepr(HdSceneDelegate*, const TfToken&);

    bool _PrimvarIsRequired(const TfToken&) const;

#ifdef HDVP2_ENABLE_GPU_COMPUTE
    void _CreateViewportCompute();
#endif
#ifdef HDVP2_ENABLE_GPU_OSD
    void _CreateOSDTables();
#endif

    TfToken& _RenderTag() override { return _meshSharedData->_renderTag; }

    void _AddNewRenderItem(
        HdVP2DrawItem*        drawItem,
        const HdMeshReprDesc& desc,
        const TfToken&        reprToken,
        MSubSceneContainer*   subSceneContainer,
        const bool            shareHighlightItem = true);

    void _UpdateMods(
        HdRprim&              refThis,
        HdVP2DrawItem*        drawItem,
        HdSceneDelegate*      sceneDelegate,
        const TfToken&        reprToken,
        const HdMeshReprDesc& desc,
        MSubSceneContainer*   subSceneContainer);

    void _UpdateDrawItem(
        HdSceneDelegate*,
        HdVP2DrawItem*,
        HdVP2DrawItem::RenderItemData&,
        const HdMeshReprDesc& desc,
        const TfToken&        reprToken);

    void _UpdatePrimvarSources(
        HdSceneDelegate*     sceneDelegate,
        HdDirtyBits          dirtyBits,
        const TfTokenVector& requiredPrimvars);

    void _PrepareSharedVertexBuffers(
        HdSceneDelegate*   delegate,
        const HdDirtyBits& rprimDirtyBits,
        const TfToken&     reprToken);

    void _CreateSmoothHullRenderItems(
        HdVP2DrawItem&      drawItem,
        const TfToken&      reprToken,
        MSubSceneContainer& subSceneContainer);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MHWRender::MRenderItem* _CreateShadedSelectedInstancesItem(
        const MString&      name,
        HdVP2DrawItem&      drawItem,
        const TfToken&      reprToken,
        MSubSceneContainer& subSceneContainer,
        const HdGeomSubset* geomSubset) const;
#endif
    HdVP2DrawItem::RenderItemData& _CreateSmoothHullRenderItem(
        const MString&      name,
        HdVP2DrawItem&      drawItem,
        const TfToken&      reprToken,
        MSubSceneContainer& subSceneContainer,
        const HdGeomSubset* geomSubset) const;
    MHWRender::MRenderItem* _CreateSelectionHighlightRenderItem(const MString& name) const;

    void _ResetRenderingTopology();

    static void _InitGPUCompute();

    //! Custom dirty bits used by this mesh
    enum DirtyBits : HdDirtyBits
    {
        DirtySmoothNormals = MayaUsdRPrim::DirtyBitLast,
        DirtyFlatNormals = (DirtySmoothNormals << 1),
        //! "Forward" the enumerated types here so we don't have to keep writing MayaUsdRPrim in
        //! the cpp file.
        DirtySelectionHighlight = MayaUsdRPrim::DirtySelectionHighlight,
        DirtySelectionMode = MayaUsdRPrim::DirtySelectionMode
    };

    HdDirtyBits _customDirtyBitsInUse {
        0
    }; //!< Storage for custom dirty bits. See _PropagateDirtyBits for details.

    std::shared_ptr<HdVP2MeshSharedData>
        _meshSharedData; //!< Shared data for all draw items of the Rprim

    //! Control GPU compute behavior
    //! Having these in place even without HDVP2_ENABLE_GPU_COMPUTE or HDVP2_ENABLE_GPU_OSD
    //! defined makes the expressions using these variables much simpler
    bool _gpuNormalsEnabled { true }; //!< Use GPU Compute for normal calculation, only used
                                      //!< when HDVP2_ENABLE_GPU_COMPUTE is defined

    // Record if the points position are generated by a UsdSkel.
    bool _pointsFromSkel { false };

    static size_t _gpuNormalsComputeThreshold;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
