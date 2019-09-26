// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "mesh.h"
#include "draw_item.h"
#include "material.h"
#include "instancer.h"
#include "proxyRenderDelegate.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/pxOsd/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/imaging/hd/vertexAdjacency.h"

#include <maya/MMatrix.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionMask.h>

#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

    //! Helper utility function to get number of draw items required for given representation
    size_t _GetNumDrawItemsForDesc(const HdMeshReprDesc& reprDesc)
    {
        // By default, each repr desc item maps to 1 draw item
        size_t numDrawItems = 1;

        // Different representations may require different number of draw items
        // See HdSt for an example.
        switch (reprDesc.geomStyle) {
        case HdMeshGeomStyleInvalid:
            numDrawItems = 0;
            break;
        default:
            break;
        }

        return numDrawItems;
    }

    //! Helper utility function to fill primvar data to vertex buffer.
    template <class DEST_TYPE, class SRC_TYPE>
    void _FillPrimvarData(DEST_TYPE* vertexBuffer, size_t numVertices, int channelIndex,
        const VtArray<SRC_TYPE>& primvarData, HdInterpolation interpolation,
        const HdMeshTopology& topology, bool requiresUnsharedVertices)
    {
        const VtIntArray& faceVertexCounts = topology.GetFaceVertexCounts();
        const VtIntArray& faceVertexIndices = topology.GetFaceVertexIndices();

        switch (interpolation) {
        case HdInterpolationConstant:
            for (size_t v = 0; v < numVertices; v++) {
                SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                    reinterpret_cast<float*>(&vertexBuffer[v]) + channelIndex);
                *pointer = primvarData[0];
            }
            break;
        case HdInterpolationVarying:
        case HdInterpolationVertex:
            if (requiresUnsharedVertices) {
                for (size_t v = 0; v < numVertices; v++) {
                    SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                        reinterpret_cast<float*>(&vertexBuffer[v]) + channelIndex);
                    *pointer = primvarData[faceVertexIndices[v]];
                }
            }
            else if (channelIndex == 0 && sizeof(DEST_TYPE) == sizeof(SRC_TYPE)) {
                memcpy(vertexBuffer, primvarData.cdata(), sizeof(DEST_TYPE) * numVertices);
            }
            else {
                for (size_t v = 0; v < numVertices; v++) {
                    SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                        reinterpret_cast<float*>(&vertexBuffer[v]) + channelIndex);
                    *pointer = primvarData[v];
                }
            }
            break;
        case HdInterpolationUniform:
            if (requiresUnsharedVertices) {
                const size_t numFaces = faceVertexCounts.size();
                for (size_t f = 0, v = 0; f < numFaces; f++) {
                    const size_t faceVertexCount = faceVertexCounts[f];
                    const size_t faceVertexEnd = v + faceVertexCount;
                    for (; v < faceVertexEnd; v++) {
                        SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                            reinterpret_cast<float*>(&vertexBuffer[v]) + channelIndex);
                        *pointer = primvarData[f];
                    }
                }
            }
            else {
                TF_WARN("Unshared vertices are required for uniform primvar.");
            }
            break;
        case HdInterpolationFaceVarying:
            if (requiresUnsharedVertices) {
                if (channelIndex == 0 && sizeof(DEST_TYPE) == sizeof(SRC_TYPE)) {
                    memcpy(vertexBuffer, primvarData.cdata(), sizeof(DEST_TYPE) * numVertices);
                }
                else {
                    for (size_t v = 0; v < numVertices; v++) {
                        SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                            reinterpret_cast<float*>(&vertexBuffer[v]) + channelIndex);
                        *pointer = primvarData[v];
                    }
                }
            }
            else {
                TF_WARN("Unshared vertices are required for face-varying primvar.");
            }
            break;
        default:
            TF_WARN("Unsupported interpolation.");
            break;
        }
    }

    //! Helper utility function to get number of edge indices
    unsigned int _GetNumOfEdgeIndices(const HdMeshTopology& topology)
    {
        const VtIntArray &faceVertexCounts = topology.GetFaceVertexCounts();

        unsigned int numIndex = 0;
        for (int i = 0; i < faceVertexCounts.size(); i++)
        {
            numIndex += faceVertexCounts[i];
        }
        numIndex *= 2; // each edge has two ends.
        return numIndex;
    }

    //! Helper utility function to extract edge indices
    void _FillEdgeIndices(int* indices, const HdMeshTopology& topology)
    {
        const VtIntArray &faceVertexCounts = topology.GetFaceVertexCounts();
        const int* currentFaceStart = topology.GetFaceVertexIndices().cdata();
        for (int faceId = 0; faceId < faceVertexCounts.size(); faceId++)
        {
            int numVertexIndicesInFace = faceVertexCounts[faceId];
            if (numVertexIndicesInFace >= 2)
            {
                for (int faceVertexId = 0; faceVertexId < numVertexIndicesInFace; faceVertexId++)
                {
                    bool isLastVertex = faceVertexId == numVertexIndicesInFace - 1;
                    *(indices++) = *(currentFaceStart + faceVertexId);
                    *(indices++) = isLastVertex ? *currentFaceStart : *(currentFaceStart + faceVertexId + 1);
                }
            }
            currentFaceStart += numVertexIndicesInFace;
        }
    }

    //! Helper utility function to adapt Maya API changes.
    void setWantConsolidation(MHWRender::MRenderItem& renderItem, bool state)
    {
#if MAYA_API_VERSION >= 20190000
        renderItem.setWantConsolidation(state);
#else
        renderItem.setWantSubSceneConsolidation(state);
#endif
    }
} //namespace


//! \brief  Constructor
HdVP2Mesh::HdVP2Mesh(HdVP2RenderDelegate* delegate, const SdfPath& id, const SdfPath& instancerId)
 : HdMesh(id, instancerId)
 , _delegate(delegate) {
    const MHWRender::MVertexBufferDescriptor vbDesc(
        "", MHWRender::MGeometry::kPosition, MHWRender::MGeometry::kFloat, 3);
    _positionsBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));
}

//! \brief  Destructor
HdVP2Mesh::~HdVP2Mesh() {
}

//! \brief  Synchronize VP2 state with scene delegate state based on dirty bits and representation
void HdVP2Mesh::Sync(
    HdSceneDelegate* delegate, HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits, const TfToken& reprToken) {

    MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2, GetId().GetText(), "HdVP2Mesh Sync");

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(),
            delegate->GetMaterialId(GetId()));
    }

    _UpdateRepr(delegate, reprToken, dirtyBits);

    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    // HdC_TODO: Currently we are running selection highlighting update in a separate execution
    // and this execution will update only wire representation. The next execution will update
    // remaining representations, but if we clear dirty bits, nothing will get updated.
    // We leave the dirty bits unmodified during selection highlighting to workaround the issue, 
    // but this is not ideal - we shouldn't have to evaluate the same data twice.
    if (!param->GetDrawScene().InSelectionHighlightUpdate())
        *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

/*! \brief  Returns the minimal set of dirty bits to place in the
            change tracker for use in the first sync of this prim.
*/
HdDirtyBits HdVP2Mesh::GetInitialDirtyBitsMask() const {
    return HdChangeTracker::Clean | HdChangeTracker::InitRepr |
        HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTopology |
        HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyMaterialId |
        HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyVisibility | HdChangeTracker::DirtyInstanceIndex;
}

/*! \brief  Add additional dirty bits

    This callback from Rprim gives the prim an opportunity to set
    additional dirty bits based on those already set.  This is done
    before the dirty bits are passed to the scene delegate, so can be
    used to communicate that extra information is needed by the prim to
    process the changes.

    The return value is the new set of dirty bits, which replaces the bits
    passed in.

    See HdRprim::PropagateRprimDirtyBits()
*/
HdDirtyBits HdVP2Mesh::_PropagateDirtyBits(HdDirtyBits bits) const {
    // If subdiv tags are dirty, topology needs to be recomputed.
    // The latter implies we'll need to recompute all primvar data.
    // Any data fetched by the scene delegate should be marked dirty here.
    if (bits & HdChangeTracker::DirtySubdivTags) {
        bits |= (HdChangeTracker::DirtyPoints |
            HdChangeTracker::DirtyNormals |
            HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyTopology |
            HdChangeTracker::DirtyDisplayStyle);
    }
    else if (bits & HdChangeTracker::DirtyTopology) {
        // Unlike basis curves, we always request refineLevel when topology is
        // dirty
        bits |= HdChangeTracker::DirtySubdivTags |
            HdChangeTracker::DirtyDisplayStyle;
    }

    // A change of material means that the Quadrangulate state may have
    // changed.
    if (bits & HdChangeTracker::DirtyMaterialId) {
        bits |= (HdChangeTracker::DirtyPoints |
            HdChangeTracker::DirtyNormals |
            HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyTopology);
    }

    // If points, display style, or topology changed, recompute normals.
    if (bits & (HdChangeTracker::DirtyPoints |
        HdChangeTracker::DirtyDisplayStyle |
        HdChangeTracker::DirtyTopology)) {
        bits |= _customDirtyBitsInUse &
            (DirtySmoothNormals | DirtyFlatNormals);
    }

    // If the topology is dirty, recompute custom indices resources.
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse &
            (DirtyIndices |
                DirtyHullIndices |
                DirtyPointsIndices);
    }

    // If normals are dirty and we are doing CPU normals
    // then the normals computation needs the points primvar
    // so mark points as dirty, so that the scene delegate will provide
    // the data.
    if ((bits & (DirtySmoothNormals | DirtyFlatNormals))/* &&
        !HdStGLUtils::IsGpuComputeEnabled()*/) {
        bits |= HdChangeTracker::DirtyPoints;
    }

    // Sometimes we don't get dirty extent notification
    if (bits & (HdChangeTracker::DirtyPoints)) {
        bits |= HdChangeTracker::DirtyExtent;
    }

    return bits;
}

/*! \brief  Initialize the given representation of this Rprim.

    This is called prior to syncing the prim, the first time the repr
    is used.

    \param  reprToken   the name of the repr to initalize.  HdRprim has already
                        resolved the reprName to its final value.

    \param  dirtyBits   an in/out value.  It is initialized to the dirty bits
                        from the change tracker.  InitRepr can then set additional
                        dirty bits if additional data is required from the scene
                        delegate when this repr is synced.

    InitRepr occurs before dirty bit propagation.

    See HdRprim::InitRepr()
*/
void HdVP2Mesh::_InitRepr(const TfToken& reprToken, HdDirtyBits* dirtyBits) {
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(), _ReprComparator(reprToken));

    HdReprSharedPtr repr;

    const bool isNew = (it == _reprs.end());
    if (isNew) {
        _reprs.emplace_back(reprToken, boost::make_shared<HdRepr>());
        repr = _reprs.back().second;
    }
    else {
        repr = it->second;
    }

    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (!subSceneContainer)
        return;

    // Selection highlight uses the wire repr for now, hoping it will be able
    // to share draw items with future implementation of wireframe mode. If
    // it won't, we can then define a customized "selectionHighlight" repr.
    if (reprToken == HdReprTokens->wire) {
        const bool selected = param->GetDrawScene().IsProxySelected() ||
            (param->GetDrawScene().GetPrimSelectionState(GetId()) != nullptr);
        if (_EnableWireDrawItems(repr, dirtyBits, selected))
            return;
    }
    else if (!isNew) {
        return;
    }

    // set dirty bit to say we need to sync a new repr (buffer array
    // ranges may change)
    *dirtyBits |= HdChangeTracker::NewRepr;

    _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);

    for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
        const HdMeshReprDesc& desc = descs[descIdx];

        size_t numDrawItems = _GetNumDrawItemsForDesc(desc);
        if (numDrawItems == 0) continue;

        for (size_t itemId = 0; itemId < numDrawItems; itemId++) {
            auto* drawItem = new HdVP2DrawItem(_delegate, &_sharedData, desc);
            repr->AddDrawItem(drawItem);

            const MString& renderItemName = drawItem->GetRenderItemName();

            MHWRender::MRenderItem* const renderItem =
                _CreateRenderItem(renderItemName, desc);

            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [subSceneContainer, renderItem]() {
                    subSceneContainer->add(renderItem);
                }
            );
        }

        if (desc.geomStyle == HdMeshGeomStyleHull) {
            if (desc.flatShadingEnabled) {
                if (!(_customDirtyBitsInUse & DirtyFlatNormals)) {
                    _customDirtyBitsInUse |= DirtyFlatNormals;
                    *dirtyBits |= DirtyFlatNormals;
                }
            }
            else {
                if (!(_customDirtyBitsInUse & DirtySmoothNormals)) {
                    _customDirtyBitsInUse |= DirtySmoothNormals;
                    *dirtyBits |= DirtySmoothNormals;
                }
            }
        }
        else if (desc.geomStyle == HdMeshGeomStyleHullEdgeOnly) {
            *dirtyBits |= HdChangeTracker::DirtyTopology;
        }
    }
}

/*! \brief  Update the named repr object for this Rprim.

    Repr objects are created to support specific reprName tokens, and contain a list of
    HdVP2DrawItems and corresponding RenderItems.
*/
void HdVP2Mesh::_UpdateRepr(HdSceneDelegate *sceneDelegate, const TfToken& reprToken, HdDirtyBits *dirtyBits) {

    HdReprSharedPtr const &curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    _MeshReprConfig::DescArray reprDescs = _GetReprDesc(reprToken);

    // Iterate through all reprdescs for the current repr to figure out if any
    // of them requires smooth normals or flat normals. If either (or both)
    // are required, we will calculate them once and clean the bits.
    bool requireSmoothNormals = false;
    bool requireFlatNormals = false;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc &desc = reprDescs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleHull) {
            if (desc.flatShadingEnabled) {
                requireFlatNormals = true;
            }
            else {
                requireSmoothNormals = true;
            }
        }
    }

    // For each relevant draw item, update dirty buffer sources.
    const HdRepr::DrawItems& items = curRepr->GetDrawItems();
    for (HdDrawItem* item : items) {
        if (HdChangeTracker::IsDirty(*dirtyBits)) {
            if (auto* drawItem = static_cast<HdVP2DrawItem*>(item)) {
                const HdMeshReprDesc &desc = drawItem->GetReprDesc();
                _UpdateDrawItem(sceneDelegate, drawItem, dirtyBits, desc,
                    requireSmoothNormals, requireFlatNormals);
            }
        }
    }
}

namespace {
    //! \brief  Helper struct used to package all the changes into single commit task
    //!         (such commit task will be executed on main-thread)
    struct CommitState {
        HdVP2DrawItem::RenderItemData& _drawItemData;

        //! If valid, new position buffer data to commit
        float*	_positionBufferData{ nullptr };
        //! If valid, new index buffer data to commit
        int*	_indexBufferData{ nullptr };
        //! If valid, new color buffer data to commit
        float*	_colorBufferData{ nullptr };
        //! If valid, new normals buffer data to commit
        float*	_normalsBufferData{ nullptr };
        //! If valid, new UV buffer data to commit
        float*	_uvBufferData{ nullptr };
        //! If valid, new shader instance to set
        MHWRender::MShaderInstance* _surfaceShader{ nullptr };
        //! Instancing doesn't have dirty bits, every time we do update, we must update instance transforms
        MMatrixArray	_instanceTransforms;
        //! Is this object transparent
        bool			_isTransparent{ false };
        //! Capture of what has changed on this rprim
        HdDirtyBits _dirtyBits;

        //! No empty constructor, we need draw item and dirty bits.
        CommitState() = delete;
        //! Construct valid commit state
        CommitState(HdVP2DrawItem& drawItem, HdDirtyBits& dirtyBits)
            : _drawItemData(drawItem.GetRenderItemData())
            , _dirtyBits(dirtyBits) {}
    };
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in CommitState and enqueued for Commit on main-thread using CommitTasks
*/
void HdVP2Mesh::_UpdateDrawItem(
    HdSceneDelegate* sceneDelegate,
    HdVP2DrawItem* drawItem,
    HdDirtyBits* dirtyBits,
    const HdMeshReprDesc &desc,
    bool requireSmoothNormals,
    bool requireFlatNormals
) {
    auto* const param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (!subSceneContainer)
        return;

    CommitState stateToCommit(*drawItem, *dirtyBits);
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._drawItemData;

    static constexpr bool writeOnly = true;

    const SdfPath& id = GetId();
    const bool isTopologyDirty = HdChangeTracker::IsTopologyDirty(*dirtyBits, id);

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _UpdatePrimvarSources(sceneDelegate, *dirtyBits);
    }

    if (isTopologyDirty) {
        _topology = GetMeshTopology(sceneDelegate);
    }

    TfTokenVector matPrimvars;

    const HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();
    const HdVP2Material* material = static_cast<const HdVP2Material*>(
        renderIndex.GetSprim(HdPrimTypeTokens->material, GetMaterialId()));
    if (material) {
        matPrimvars = material->GetRequiredPrimvars();
    }
    else {
        matPrimvars.push_back(HdTokens->displayColor);
        matPrimvars.push_back(HdTokens->displayOpacity);
        matPrimvars.push_back(HdTokens->normals);
    }

    // If there is uniform or face-varying primvar, we have to expand shared
    // vertices in CPU because OpenGL SSBO technique is not widely supported
    // on GPUs and 3D APIs.
    bool requiresUnsharedVertices = false;
    for (const TfToken& pv : matPrimvars) {
        const auto it = _primvarSourceMap.find(pv);
        if (it != _primvarSourceMap.end()) {
            const HdInterpolation interpolation = it->second.interpolation;
            if (interpolation == HdInterpolationUniform ||
                interpolation == HdInterpolationFaceVarying) {
                requiresUnsharedVertices = true;
                break;
            }
        }
    }

    // Prepare index buffer.
    if (isTopologyDirty) {
        const HdMeshTopology* topologyToUse = &_topology;
        HdMeshTopology unsharedTopology;

        if (requiresUnsharedVertices) {
            // Fill with sequentially increasing values, starting from 0.
            VtIntArray newFaceVtxIds;
            newFaceVtxIds.resize(_topology.GetFaceVertexIndices().size());
            std::iota(newFaceVtxIds.begin(), newFaceVtxIds.end(), 0);

            unsharedTopology = HdMeshTopology(
                _topology.GetScheme(),
                _topology.GetOrientation(),
                _topology.GetFaceVertexCounts(),
                newFaceVtxIds,
                _topology.GetHoleIndices(),
                _topology.GetRefineLevel()
            );

            topologyToUse = &unsharedTopology;
        }

        if (desc.geomStyle == HdMeshGeomStyleHull) {
            HdMeshUtil meshUtil(topologyToUse, id);
            VtVec3iArray trianglesFaceVertexIndices;
            VtIntArray primitiveParam;
            meshUtil.ComputeTriangleIndices(&trianglesFaceVertexIndices, &primitiveParam, nullptr);

            const int numIndex = trianglesFaceVertexIndices.size() * 3;

            stateToCommit._indexBufferData = static_cast<int*>(
                drawItemData._indexBuffer->acquire(numIndex, writeOnly));

            memcpy(stateToCommit._indexBufferData, trianglesFaceVertexIndices.data(), numIndex * sizeof(int));
        }
        else if (desc.geomStyle == HdMeshGeomStyleHullEdgeOnly) {
            unsigned int numIndex = _GetNumOfEdgeIndices(*topologyToUse);

            stateToCommit._indexBufferData = static_cast<int*>(
                drawItemData._indexBuffer->acquire(numIndex, writeOnly));

            _FillEdgeIndices(stateToCommit._indexBufferData, *topologyToUse);
        }
        else {
            // We don't need an index buffer for points repr.
        }
    }

    // Prepare position buffer. It is shared among all render items of the rprim
    // so it should be updated only once when it gets dirty.
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        const auto it = _primvarSourceMap.find(HdTokens->points);
        if (ARCH_LIKELY(it != _primvarSourceMap.end())) {
            const VtValue& value = it->second.data;
            if(ARCH_LIKELY(value.IsHolding<VtVec3fArray>())) {
                const VtVec3fArray& points = value.UncheckedGet<VtVec3fArray>();

                const size_t numVertices = requiresUnsharedVertices ?
                    _topology.GetFaceVertexIndices().size() : points.size();

                void* bufferData = _positionsBuffer->acquire(numVertices, writeOnly);

                if (bufferData) {
                    _FillPrimvarData(static_cast<GfVec3f*>(bufferData), numVertices, 0,
                        points, HdInterpolationVertex, _topology, requiresUnsharedVertices);

                    stateToCommit._positionBufferData = static_cast<float*>(bufferData);
                }
            }
        }
    }

    // Prepare normals.
    if (drawItemData._normalsBuffer) {
        VtVec3fArray normals;
        HdInterpolation interp;

        const auto it = _primvarSourceMap.find(HdTokens->normals);
        if (it != _primvarSourceMap.end()) {
            const VtValue& value = it->second.data;
            if (ARCH_LIKELY(value.IsHolding<VtVec3fArray>())) {
                normals = value.UncheckedGet<VtVec3fArray>();
                interp = it->second.interpolation;
            }
        }

        bool prepareNormals = false;

        // If there is authored normals, prepare buffer only when it is dirty.
        // otherwise, compute smooth normals from points and adjacency and we
        // have a custom dirty bit to determine whether update is needed.
        if (!normals.empty()) {
            prepareNormals = HdChangeTracker::IsPrimvarDirty(
                *dirtyBits, id, HdTokens->normals);
        }
        else if (requireSmoothNormals && (*dirtyBits & DirtySmoothNormals)) {
            // note: normals gets dirty when points are marked as dirty,
            // at change tracker.
            // clear DirtySmoothNormals (this is not a scene dirtybit)
            *dirtyBits &= ~DirtySmoothNormals;

            prepareNormals = true;

            VtValue valuePoints;
            const auto it = _primvarSourceMap.find(HdTokens->points);
            if (it != _primvarSourceMap.end()) {
                valuePoints = it->second.data;
            }

            if (ARCH_LIKELY(valuePoints.IsHolding<VtVec3fArray>())) {
                const VtVec3fArray& points = valuePoints.UncheckedGet<VtVec3fArray>();

                Hd_VertexAdjacencySharedPtr adjacency(new Hd_VertexAdjacency());
                HdBufferSourceSharedPtr adjacencyComputation =
                    adjacency->GetSharedAdjacencyBuilderComputation(&_topology);
                adjacencyComputation->Resolve(); // IS the adjacency updated now?

                normals = Hd_SmoothNormals::ComputeSmoothNormals(
                    adjacency.get(), _topology.GetNumPoints(), points.cdata());

                interp = HdInterpolationVertex;
            }
        }

        if (prepareNormals && !normals.empty()) {
            const size_t numVertices = requiresUnsharedVertices ?
                _topology.GetFaceVertexIndices().size() : normals.size();

            void* bufferData = drawItemData._normalsBuffer->acquire(numVertices, writeOnly);
            if (bufferData) {
                _FillPrimvarData(static_cast<GfVec3f*>(bufferData), numVertices, 0,
                    normals, interp, _topology, requiresUnsharedVertices);

                stateToCommit._normalsBufferData = static_cast<float*>(bufferData);
            }
        }
    }

    // Prepare other primvars required by the material.
    // HdC_TODO: per-instance constant data support.
    // HdC_TODO: multi uv
    if (material && drawItemData._uvBuffer)
    {
        for (const TfToken& pv : matPrimvars) {
            if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, pv))
                continue;

            const auto it = _primvarSourceMap.find(pv);
            if (it == _primvarSourceMap.end())
                continue;

            const VtValue& value = it->second.data;
            const HdInterpolation& interp = it->second.interpolation;

            if (!value.IsArrayValued() || value.GetArraySize() == 0)
                continue;

            // HdC_TODO: support other vector types.
            if (!value.IsHolding<VtVec2fArray>())
                continue;

            size_t numVertices = 0;

            if (requiresUnsharedVertices) {
                numVertices = _topology.GetFaceVertexIndices().size();
            }
            else {
                VtValue valuePoints;
                const auto it = _primvarSourceMap.find(HdTokens->points);
                if (it != _primvarSourceMap.end()) {
                    valuePoints = it->second.data;
                }

                if (ARCH_LIKELY(valuePoints.IsHolding<VtVec3fArray>())) {
                    numVertices = valuePoints.UncheckedGet<VtVec3fArray>().size();
                }
            }

            if (numVertices == 0)
                continue;

            void* bufferData = drawItemData._uvBuffer->acquire(numVertices, writeOnly);

            if (value.IsHolding<VtFloatArray>()) {
                _FillPrimvarData(
                    static_cast<float*>(bufferData), numVertices, 0,
                    value.UncheckedGet<VtFloatArray>(), interp,
                    _topology, requiresUnsharedVertices);
            }
            else if (value.IsHolding<VtVec2fArray>()) {
                _FillPrimvarData(
                    static_cast<GfVec2f*>(bufferData), numVertices, 0,
                    value.UncheckedGet<VtVec2fArray>(), interp,
                    _topology, requiresUnsharedVertices);
            }
            else if (value.IsHolding<VtVec3fArray>()) {
                _FillPrimvarData(
                    static_cast<GfVec3f*>(bufferData), numVertices, 0,
                    value.UncheckedGet<VtVec3fArray>(), interp,
                    _topology, requiresUnsharedVertices);
            }
            else if (value.IsHolding<VtVec4fArray>()) {
                _FillPrimvarData(
                    static_cast<GfVec4f*>(bufferData), numVertices, 0,
                    value.UncheckedGet<VtVec4fArray>(), interp,
                    _topology, requiresUnsharedVertices);
            }
            else {
                TF_WARN("Unsupported primvar array");
            }

            stateToCommit._uvBufferData = static_cast<float*>(bufferData);
            drawItemData._hasUVs = true;
            break;
        }
    }

    // Prepare color buffer.
    if (desc.geomStyle == HdMeshGeomStyleHull) {
        if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
            if (material) {
                stateToCommit._surfaceShader = material->GetSurfaceShader();

                // HdC_TODO
                drawItemData._colorBuffer.reset(nullptr);
            }

            // HdC_TODO: can also need the fallback shader for constant color.
            if (!stateToCommit._surfaceShader) {
                stateToCommit._surfaceShader = _delegate->GetColorPerVertexShader();
            }
        }

        if (drawItemData._colorBuffer && (*dirtyBits & HdChangeTracker::DirtyPrimvar)) {
            const size_t numFaces = _topology.GetFaceVertexCounts().size();

            // If color/opacity is not found, the 18% gray color will be used
            // to match the default color of Hydra Storm.
            VtVec3fArray colorArray(1, GfVec3f(0.18f, 0.18f, 0.18f));
            VtFloatArray alphaArray(1, 1.0f);

            HdInterpolation colorInterp = HdInterpolationConstant;
            HdInterpolation alphaInterp = HdInterpolationConstant;

            auto it = _primvarSourceMap.find(HdTokens->displayColor);
            if (it != _primvarSourceMap.end()) {
                const VtValue& value = it->second.data;
                if (value.IsHolding<VtVec3fArray>() && value.GetArraySize() > 0) {
                    if (value.GetArraySize() != numFaces &&
                        it->second.interpolation == HdInterpolationUniform) {
                        TF_WARN("# of faces mismatch (%d != %d) for %s.%s",
                            (int)value.GetArraySize(), numFaces,
                            id.GetText(), it->first.GetText());
                    }
                    else {
                        colorArray = value.UncheckedGet<VtVec3fArray>();
                        colorInterp = it->second.interpolation;
                    }
                }
            }

            it = _primvarSourceMap.find(HdTokens->displayOpacity);
            if (it != _primvarSourceMap.end()) {
                const VtValue& value = it->second.data;
                if (value.IsHolding<VtFloatArray>() && value.GetArraySize() > 0) {
                    if (value.GetArraySize() != numFaces &&
                        it->second.interpolation == HdInterpolationUniform) {
                        TF_WARN("# of faces mismatch (%d != %d) for %s.%s",
                            (int)value.GetArraySize(), numFaces,
                            id.GetText(), it->first.GetText());
                    }
                    else {
                        alphaArray = value.UncheckedGet<VtFloatArray>();
                        alphaInterp = it->second.interpolation;
                    }
                }
            }

            if (colorInterp == HdInterpolationConstant &&
                alphaInterp == HdInterpolationConstant) {
                const GfVec3f& color = colorArray[0];

                MColor mayaColor(color[0], color[1], color[2], alphaArray[0]);

                // Pre-multiplied with alpha
                mayaColor *= mayaColor.a;

                stateToCommit._surfaceShader = _delegate->GetFallbackShader(mayaColor);
                stateToCommit._isTransparent = (mayaColor.a < 0.99f);

                // HdC_TODO: allocate on demand.
                drawItemData._colorBuffer.reset(nullptr);
            }
            else {
                size_t numVertices = 0;

                if (requiresUnsharedVertices) {
                    numVertices = _topology.GetFaceVertexIndices().size();
                }
                else {
                    VtValue valuePoints;
                    const auto it = _primvarSourceMap.find(HdTokens->points);
                    if (it != _primvarSourceMap.end()) {
                        valuePoints = it->second.data;
                    }

                    if (ARCH_LIKELY(valuePoints.IsHolding<VtVec3fArray>())) {
                        numVertices = valuePoints.UncheckedGet<VtVec3fArray>().size();
                    }
                }

                void* bufferData = drawItemData._colorBuffer->acquire(numVertices, writeOnly);

                if (bufferData) {
                    _FillPrimvarData(static_cast<GfVec4f*>(bufferData), numVertices, 0,
                        colorArray, colorInterp, _topology, requiresUnsharedVertices);

                    _FillPrimvarData(static_cast<GfVec4f*>(bufferData), numVertices, 3,
                        alphaArray, alphaInterp, _topology, requiresUnsharedVertices);

                    stateToCommit._colorBufferData = static_cast<float*>(bufferData);
                }
            }
        }
    }

    // Bounding box is per-prim shared data.
    GfRange3d range;

    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        range = GetExtent(sceneDelegate);
        if (!range.IsEmpty()) {
            _sharedData.bounds.SetRange(range);
        }

        *dirtyBits &= ~HdChangeTracker::DirtyExtent;
    }
    else {
        range = _sharedData.bounds.GetRange();
    }

    const GfVec3d& min = range.GetMin();
    const GfVec3d& max = range.GetMax();
    const MBoundingBox bounds(MPoint(min[0], min[1], min[2]), MPoint(max[0], max[1], max[2]));

    // World matrix is per-prim shared data.
    GfMatrix4d transform;

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        transform = sceneDelegate->GetTransform(id);
        _sharedData.bounds.SetMatrix(transform);

        *dirtyBits &= ~HdChangeTracker::DirtyTransform;
    }
    else {
        transform = _sharedData.bounds.GetMatrix();
    }

    MMatrix worldMatrix;
    transform.Get(worldMatrix.matrix);

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _UpdateVisibility(sceneDelegate, dirtyBits);
    }

    // If the mesh is instanced, create one new instance per transform.
    // The current instancer invalidation tracking makes it hard for
    // us to tell whether transforms will be dirty, so this code
    // pulls them every time something changes.
    if (!GetInstancerId().IsEmpty()) {

        // Retrieve instance transforms from the instancer.
        HdInstancer *instancer = renderIndex.GetInstancer(GetInstancerId());
        VtMatrix4dArray transforms =
            static_cast<HdVP2Instancer*>(instancer)->
            ComputeInstanceTransforms(id);

        double instanceMatrix[4][4];

        if ((desc.geomStyle == HdMeshGeomStyleHullEdgeOnly) &&
            !param->GetDrawScene().IsProxySelected()) {
            if (auto state = param->GetDrawScene().GetPrimSelectionState(id)) {
                for (const auto& indexArray : state->instanceIndices) {
                    for (const auto index : indexArray) {
                        transforms[index].Get(instanceMatrix);
                        stateToCommit._instanceTransforms.append(MMatrix(instanceMatrix));
                    }
                }
            }
        }
        else {
            for (size_t i = 0; i < transforms.size(); ++i) {
                transforms[i].Get(instanceMatrix);
                stateToCommit._instanceTransforms.append(MMatrix(instanceMatrix));
            }
        }
    }

    // Capture class member for lambda
    MHWRender::MVertexBuffer* const positionsBuffer = _positionsBuffer.get();

    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [drawItem, stateToCommit, param, positionsBuffer, bounds, worldMatrix]()
    {
        const MString& renderItemName = drawItem->GetRenderItemName();

        MProfilingScope profilingScope(HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2, renderItemName.asChar(), "HdVP2Mesh Commit Buffers");

        MHWRender::MRenderItem* renderItem = param->GetContainer()->find(renderItemName);
        if(!renderItem) {
            TF_CODING_ERROR("Invalid render item: %s\n", renderItemName.asChar());
            return;
        }

        MHWRender::MVertexBuffer* colorBuffer = stateToCommit._drawItemData._colorBuffer.get();
        MHWRender::MVertexBuffer* normalsBuffer = stateToCommit._drawItemData._normalsBuffer.get();
        MHWRender::MVertexBuffer* uvBuffer =
            stateToCommit._drawItemData._hasUVs ?
            stateToCommit._drawItemData._uvBuffer.get() : nullptr;

        MHWRender::MIndexBuffer* indexBuffer = stateToCommit._drawItemData._indexBuffer.get();

        MHWRender::MVertexBufferArray vertexBuffers;
        vertexBuffers.addBuffer("positions", positionsBuffer);

        if (colorBuffer)
            vertexBuffers.addBuffer("diffuseColor", colorBuffer);

        if (normalsBuffer)
            vertexBuffers.addBuffer("normals", normalsBuffer);

        if (uvBuffer)
            vertexBuffers.addBuffer("UVs", uvBuffer);

        // If available, something changed
        if(stateToCommit._positionBufferData)
            positionsBuffer->commit(stateToCommit._positionBufferData);

        // If available, something changed
        if (stateToCommit._colorBufferData && colorBuffer)
            colorBuffer->commit(stateToCommit._colorBufferData);

        // If available, something changed
        if (stateToCommit._normalsBufferData && normalsBuffer)
            normalsBuffer->commit(stateToCommit._normalsBufferData);

        // If available, something changed
        if (stateToCommit._uvBufferData && uvBuffer)
            uvBuffer->commit(stateToCommit._uvBufferData);

        // If available, something changed
        if (stateToCommit._indexBufferData && indexBuffer)
            indexBuffer->commit(stateToCommit._indexBufferData);

        // If available, something changed
        if (stateToCommit._surfaceShader) {
            renderItem->setShader(stateToCommit._surfaceShader);
            renderItem->setTreatAsTransparent(stateToCommit._isTransparent);
        }

        if ((stateToCommit._dirtyBits & HdChangeTracker::DirtyVisibility) != 0) {
            renderItem->enable(drawItem->IsEnabled() && drawItem->GetVisible());
        }

        param->GetDrawScene().setGeometryForRenderItem(*renderItem, vertexBuffers, *indexBuffer, &bounds);
        renderItem->setMatrix(&worldMatrix);

        // Important, update instance transforms after setting geometry on render items!
        auto& oldInstanceCount = stateToCommit._drawItemData._instanceCount;
        auto newInstanceCount = stateToCommit._instanceTransforms.length();
        if(newInstanceCount > 0) {
            setWantConsolidation(*renderItem, false);
            if(oldInstanceCount == newInstanceCount) {
                for (unsigned int i = 0; i < newInstanceCount; i++) {
                    // VP2 defines instance ID of the first instance to be 1.
                    param->GetDrawScene().updateInstanceTransform(*renderItem, i+1, stateToCommit._instanceTransforms[i]);
                }
            } else {
                param->GetDrawScene().setInstanceTransformArray(*renderItem, stateToCommit._instanceTransforms);
            }
            oldInstanceCount = newInstanceCount;
        } else if(oldInstanceCount > 0) {
            setWantConsolidation(*renderItem, true);
            param->GetDrawScene().removeAllInstances(*renderItem);
            oldInstanceCount = 0;
        }
    });
}

void HdVP2Mesh::_UpdatePrimvarSources(
    HdSceneDelegate* sceneDelegate,
    HdDirtyBits dirtyBits)
{
    const SdfPath& id = GetId();

    // Update _primvarSourceMap, our local cache of raw primvar data.
    // This function pulls data from the scene delegate, but defers processing.
    //
    // We only call GetPrimvar on primvars that have been marked dirty.
    //
    // Currently, hydra doesn't have a good way of communicating changes in
    // the set of primvars, so we only ever add and update to the primvar set.

    for (size_t i = 0; i < HdInterpolationCount; i++) {
        const HdInterpolation interpolation = static_cast<HdInterpolation>(i);
        const HdPrimvarDescriptorVector primvars =
            GetPrimvarDescriptors(sceneDelegate, interpolation);

        for (const HdPrimvarDescriptor& pv: primvars) {
            if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                const VtValue value = GetPrimvar(sceneDelegate, pv.name);
                _primvarSourceMap[pv.name] = { value, interpolation };
            }
        }
    }
}

/*! \brief  Create render item for points repr.
*/
MHWRender::MRenderItem* HdVP2Mesh::_CreatePointsRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::DecorationItem,
        MHWRender::MGeometry::kPoints
    );

    renderItem->setDrawMode(MHWRender::MGeometry::kSelectionOnly);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dFatPointShader());

    MSelectionMask selectionMask(MSelectionMask::kSelectPointsForGravity);
    selectionMask.addMask(MSelectionMask::kSelectMeshVerts);
    renderItem->setSelectionMask(selectionMask);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for wireframe repr.
*/
MHWRender::MRenderItem* HdVP2Mesh::_CreateWireframeRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::DecorationItem,
        MHWRender::MGeometry::kLines
    );

    renderItem->depthPriority(MHWRender::MRenderItem::sActiveWireDepthPriority);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader());

    renderItem->setSelectionMask(MSelectionMask());

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for smoothHull repr.
*/
MHWRender::MRenderItem* HdVP2Mesh::_CreateSmoothHullRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name,
        MHWRender::MRenderItem::MaterialSceneItem,
        MHWRender::MGeometry::kTriangles
    );

    renderItem->setExcludedFromPostEffects(false);
    renderItem->castsShadows(true);
    renderItem->receivesShadows(true);
    renderItem->setShader(_delegate->GetFallbackShader());

    renderItem->setSelectionMask(MSelectionMask::kSelectMeshes);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for the specified desc.
*/
MHWRender::MRenderItem* HdVP2Mesh::_CreateRenderItem(
    const MString& name,
    const HdMeshReprDesc& desc) const
{
    MHWRender::MRenderItem* renderItem = nullptr;

    switch (desc.geomStyle) {
    case HdMeshGeomStyleHull:
        renderItem = _CreateSmoothHullRenderItem(name);
        break;
    case HdMeshGeomStyleHullEdgeOnly:
        renderItem = _CreateWireframeRenderItem(name);
        break;
    case HdMeshGeomStylePoints:
        renderItem = _CreatePointsRenderItem(name);
        break;
    default:
        TF_WARN("Unexpected geomStyle");
        break;
    }

    return renderItem;
}

/*! \brief  Enable or disable the wireframe draw item.

    \return True if no draw items should be created for the repr.
*/
bool HdVP2Mesh::_EnableWireDrawItems(
    const HdReprSharedPtr& repr,
    HdDirtyBits* dirtyBits,
    bool enable)
{
    if (_wireItemsEnabled == enable) {
        return true;
    }

    _wireItemsEnabled = enable;

    if (repr) {
        const HdRepr::DrawItems& items = repr->GetDrawItems();
        for (HdDrawItem* item : items) {
            if (auto drawItem = static_cast<HdVP2DrawItem*>(item)) {
                const HdMeshReprDesc& reprDesc = drawItem->GetReprDesc();
                if (reprDesc.geomStyle == HdMeshGeomStyleHullEdgeOnly) {
                    drawItem->Enable(enable);
                    *dirtyBits |= HdChangeTracker::DirtyVisibility;
                    return true;
                }
            }
        }
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
