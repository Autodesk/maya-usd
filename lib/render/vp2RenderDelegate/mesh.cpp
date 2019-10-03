//
// Copyright 2019 Autodesk, Inc. All rights reserved.
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

#include "mesh.h"
#include "debugCodes.h"
#include "draw_item.h"
#include "material.h"
#include "instancer.h"
#include "proxyRenderDelegate.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include <maya/MMatrix.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionMask.h>

#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

    //! A primvar vertex buffer data map indexed by primvar name.
    using PrimvarBufferDataMap = std::unordered_map<
        TfToken,
        float*,
        TfToken::HashFunctor
    >;

    //! \brief  Helper struct used to package all the changes into single commit task
    //!         (such commit task will be executed on main-thread)
    struct CommitState {
        HdVP2DrawItem::RenderItemData& _drawItemData;

        //! If valid, new position buffer data to commit
        float*  _positionBufferData{ nullptr };
        //! If valid, new index buffer data to commit
        int*    _indexBufferData{ nullptr };
        //! If valid, new color buffer data to commit
        float*  _colorBufferData{ nullptr };
        //! If valid, new normals buffer data to commit
        float*  _normalsBufferData{ nullptr };
        //! If valid, new primvar buffer data to commit
        PrimvarBufferDataMap _primvarBufferDataMap;

        //! If valid, new shader instance to set
        MHWRender::MShaderInstance* _surfaceShader{ nullptr };

        //! Instancing doesn't have dirty bits, every time we do update, we must update instance transforms
        MMatrixArray _instanceTransforms;

        //! Is this object transparent
        bool _isTransparent{ false };

        //! Capture of what has changed on this rprim
        HdDirtyBits _dirtyBits;

        //! Construct valid commit state
        CommitState(HdVP2DrawItem& drawItem, HdDirtyBits& dirtyBits)
            : _drawItemData(drawItem.GetRenderItemData())
            , _dirtyBits(dirtyBits) {}

        //! No default constructor, we need draw item and dirty bits.
        CommitState() = delete;
    };

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
    void _FillPrimvarData(DEST_TYPE* vertexBuffer,
        size_t numVertices,
        size_t channelOffset,
        bool requiresUnsharedVertices,
        const SdfPath& meshId,
        const HdMeshTopology& topology,
        const TfToken& primvarName,
        const VtArray<SRC_TYPE>& primvarData,
        const HdInterpolation& primvarInterp)
    {
        switch (primvarInterp) {
        case HdInterpolationConstant:
            for (size_t v = 0; v < numVertices; v++) {
                SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                    reinterpret_cast<float*>(&vertexBuffer[v]) + channelOffset);
                *pointer = primvarData[0];
            }
            break;
        case HdInterpolationVarying:
        case HdInterpolationVertex:
            if (requiresUnsharedVertices) {
                const VtIntArray& faceVertexIndices = topology.GetFaceVertexIndices();
                if (numVertices == faceVertexIndices.size()) {
                    for (size_t v = 0; v < numVertices; v++) {
                        SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                            reinterpret_cast<float*>(&vertexBuffer[v]) + channelOffset);
                        *pointer = primvarData[faceVertexIndices[v]];
                    }
                }
                else {
                    // numVertices must have been assigned with the number of
                    // face vertices as required by vertex unsharing.
                    TF_CODING_ERROR("Invalid Hydra prim '%s': primvar %s "
                        "requires %zu unshared elements, while the number of "
                        "face vertices is %zu. Skipping primvar update.",
                        meshId.GetText(), primvarName.GetText(),
                        numVertices, faceVertexIndices.size());
                }
            }
            else if (numVertices <= primvarData.size()) {
                // The primvar has more data than needed, we issue a warning but
                // don't skip update. Truncate the buffer to the expected length.
                if (numVertices < primvarData.size()) {
                    TF_DEBUG(HDVP2_DEBUG_MESH).Msg("Invalid Hydra prim '%s': "
                        "primvar %s has %zu elements, while its topology "
                        "references only upto element index %zu.\n",
                        meshId.GetText(), primvarName.GetText(),
                        primvarData.size(), numVertices);
                }

                if (channelOffset == 0 && sizeof(DEST_TYPE) == sizeof(SRC_TYPE)) {
                    memcpy(vertexBuffer, primvarData.cdata(), sizeof(DEST_TYPE) * numVertices);
                }
                else {
                    for (size_t v = 0; v < numVertices; v++) {
                        SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                            reinterpret_cast<float*>(&vertexBuffer[v]) + channelOffset);
                        *pointer = primvarData[v];
                    }
                }
            }
            else {
                // The primvar has less data than needed. Issue warning and skip
                // update like what is done in HdStMesh.
                TF_DEBUG(HDVP2_DEBUG_MESH).Msg("Invalid Hydra prim '%s': "
                    "primvar %s has only %zu elements, while its topology expects "
                    "at least %zu elements. Skipping primvar update.\n",
                    meshId.GetText(), primvarName.GetText(),
                    primvarData.size(), numVertices);

                memset(vertexBuffer, 0, sizeof(DEST_TYPE) * numVertices);
            }
            break;
        case HdInterpolationUniform:
            if (requiresUnsharedVertices) {
                const VtIntArray& faceVertexCounts = topology.GetFaceVertexCounts();
                const size_t numFaces = faceVertexCounts.size();
                if (numFaces <= primvarData.size()) {
                    // The primvar has more data than needed, we issue a warning but
                    // don't skip update. Truncate the buffer to the expected length.
                    if (numFaces < primvarData.size()) {
                        TF_DEBUG(HDVP2_DEBUG_MESH).Msg("Invalid Hydra prim '%s': "
                            "primvar %s has %zu elements, while its topology "
                            "references only upto element index %zu.\n",
                            meshId.GetText(), primvarName.GetText(),
                            primvarData.size(), numFaces);
                    }

                    for (size_t f = 0, v = 0; f < numFaces; f++) {
                        const size_t faceVertexCount = faceVertexCounts[f];
                        const size_t faceVertexEnd = v + faceVertexCount;
                        for (; v < faceVertexEnd; v++) {
                            SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                                reinterpret_cast<float*>(&vertexBuffer[v]) + channelOffset);
                            *pointer = primvarData[f];
                        }
                    }
                }
                else {
                    // The primvar has less data than needed. Issue warning and skip
                    // update like what is done in HdStMesh.
                    TF_DEBUG(HDVP2_DEBUG_MESH).Msg("Invalid Hydra prim '%s': "
                        "primvar %s has only %zu elements, while its topology expects "
                        "at least %zu elements. Skipping primvar update.\n",
                        meshId.GetText(), primvarName.GetText(),
                        primvarData.size(), numFaces);

                    memset(vertexBuffer, 0, sizeof(DEST_TYPE) * numVertices);
                }
            }
            else {
                TF_CODING_ERROR("Invalid Hydra prim '%s': "
                    "vertex unsharing is required for uniform primvar %s",
                    meshId.GetText(), primvarName.GetText());
            }
            break;
        case HdInterpolationFaceVarying:
            if (requiresUnsharedVertices) {
                if (numVertices <= primvarData.size()) {
                    // If the primvar has more data than needed, we issue a warning,
                    // but don't skip the primvar update. Truncate the buffer to the
                    // expected length.
                    if (numVertices < primvarData.size()) {
                        TF_DEBUG(HDVP2_DEBUG_MESH).Msg("Invalid Hydra prim '%s': "
                            "primvar %s has %zu elements, while its topology references "
                            "only upto element index %zu.\n",
                            meshId.GetText(), primvarName.GetText(),
                            primvarData.size(), numVertices);
                    }

                    if (channelOffset == 0 && sizeof(DEST_TYPE) == sizeof(SRC_TYPE)) {
                        memcpy(vertexBuffer, primvarData.cdata(), sizeof(DEST_TYPE) * numVertices);
                    }
                    else {
                        for (size_t v = 0; v < numVertices; v++) {
                            SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                                reinterpret_cast<float*>(&vertexBuffer[v]) + channelOffset);
                            *pointer = primvarData[v];
                        }
                    }
                }
                else {
                    // It is unexpected to have less data than we index into. Issue
                    // a warning and skip update.
                    TF_DEBUG(HDVP2_DEBUG_MESH).Msg("Invalid Hydra prim '%s': "
                        "primvar %s has only %zu elements, while its topology expects "
                        "at least %zu elements. Skipping primvar update.\n",
                        meshId.GetText(), primvarName.GetText(),
                        primvarData.size(), numVertices);

                    memset(vertexBuffer, 0, sizeof(DEST_TYPE) * numVertices);
                }
            }
            else {
                TF_CODING_ERROR("Invalid Hydra prim '%s': "
                    "vertex unsharing is required face-varying primvar %s",
                    meshId.GetText(), primvarName.GetText());
            }
            break;
        default:
            TF_CODING_ERROR("Invalid Hydra prim '%s': "
                "unimplemented interpolation %zu for primvar %s",
                meshId.GetText(), (int)primvarInterp, primvarName.GetText());
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

    const bool topologyDirty =
        HdChangeTracker::IsTopologyDirty(*dirtyBits, id);
    const bool pointsDirty =
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points);
    const bool normalsDirty =
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals);
    const bool primvarDirty =
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar);

    if (topologyDirty) {
        _topology = GetMeshTopology(sceneDelegate);
    }

    if (pointsDirty) {
         const VtValue value = sceneDelegate->Get(id, HdTokens->points);
         _points = value.Get<VtVec3fArray>();
    }

    if (normalsDirty || primvarDirty) {
        _UpdatePrimvarSources(sceneDelegate, *dirtyBits);
    }

    TfTokenVector matPrimvars;

    const HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();
    const HdVP2Material* material = static_cast<const HdVP2Material*>(
        renderIndex.GetSprim(HdPrimTypeTokens->material, GetMaterialId()));
    if (material && material->GetSurfaceShader()) {
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

    const size_t numVertices = requiresUnsharedVertices ?
        _topology.GetFaceVertexIndices().size() : _topology.GetNumPoints();

    // Prepare index buffer.
    if (topologyDirty) {
        const HdMeshTopology* topologyToUse = &_topology;
        HdMeshTopology unsharedTopology;

        if (requiresUnsharedVertices) {
            // Fill with sequentially increasing values, starting from 0. The
            // new face vertex indices will then be implicitly used to assemble
            // all primvar vertex buffers.
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
            // Index buffer data is not required for point representation.
        }
    }

    // Prepare position buffer. It is shared among all render items of the rprim
    // so it should be updated only once when it gets dirty.
    if (pointsDirty) {
        void* bufferData = _positionsBuffer->acquire(numVertices, writeOnly);
        if (bufferData) {
            _FillPrimvarData(static_cast<GfVec3f*>(bufferData),
                numVertices, 0, requiresUnsharedVertices,
                id, _topology, HdTokens->points, _points, HdInterpolationVertex);

            stateToCommit._positionBufferData = static_cast<float*>(bufferData);
        }
    }

    // Prepare normal buffer.
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
            prepareNormals = normalsDirty;
        }
        else if (requireSmoothNormals && (*dirtyBits & DirtySmoothNormals)) {
            // note: normals gets dirty when points are marked as dirty,
            // at change tracker.
            // clear DirtySmoothNormals (this is not a scene dirtybit)
            *dirtyBits &= ~DirtySmoothNormals;

            prepareNormals = true;

            Hd_VertexAdjacencySharedPtr adjacency(new Hd_VertexAdjacency());
            HdBufferSourceSharedPtr adjacencyComputation =
                adjacency->GetSharedAdjacencyBuilderComputation(&_topology);
            adjacencyComputation->Resolve(); // IS the adjacency updated now?

            // The topology doesn't have to reference all of the points, thus
            // we compute the number of normals as required by the topology.
            normals = Hd_SmoothNormals::ComputeSmoothNormals(
                adjacency.get(), _topology.GetNumPoints(), _points.cdata());

            interp = HdInterpolationVertex;
        }

        if (prepareNormals && !normals.empty()) {
            void* bufferData = drawItemData._normalsBuffer->acquire(numVertices, writeOnly);
            if (bufferData) {
                _FillPrimvarData(static_cast<GfVec3f*>(bufferData),
                    numVertices, 0, requiresUnsharedVertices,
                    id, _topology, HdTokens->normals, normals, interp);

                stateToCommit._normalsBufferData = static_cast<float*>(bufferData);
            }
        }
    }

    // Prepare color buffer.
    if (desc.geomStyle == HdMeshGeomStyleHull) {
        if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
            if (material) {
                stateToCommit._surfaceShader = material->GetSurfaceShader();
                if (stateToCommit._surfaceShader &&
                    stateToCommit._surfaceShader->isTransparent()) {
                    stateToCommit._isTransparent = true;
                }
            }
        }

        TfTokenVector::const_iterator begin = matPrimvars.cbegin();
        TfTokenVector::const_iterator end = matPrimvars.cend();

        if (primvarDirty &&
            (std::find(begin, end, HdTokens->displayColor) != end ||
             std::find(begin, end, HdTokens->displayOpacity) != end)) {
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
                    colorArray = value.UncheckedGet<VtVec3fArray>();
                    colorInterp = it->second.interpolation;
                }
            }

            it = _primvarSourceMap.find(HdTokens->displayOpacity);
            if (it != _primvarSourceMap.end()) {
                const VtValue& value = it->second.data;
                if (value.IsHolding<VtFloatArray>() && value.GetArraySize() > 0) {
                    alphaArray = value.UncheckedGet<VtFloatArray>();
                    alphaInterp = it->second.interpolation;
                }
            }

            if (colorInterp == HdInterpolationConstant &&
                alphaInterp == HdInterpolationConstant) {
                const GfVec3f& color = colorArray[0];
                const float alpha = alphaArray[0];

                MColor mayaColor(color[0], color[1], color[2], alpha);

                // Pre-multiplied with alpha
                // HdC_TODO: investigate why it is needed.
                mayaColor *= alpha;

                // Use fallback shader if there is no material binding or we
                // failed to create a shader instance from the material.
                if (!stateToCommit._surfaceShader) {
                    stateToCommit._surfaceShader = _delegate->GetFallbackShader(mayaColor);
                    stateToCommit._isTransparent = (alpha < 0.999f);
                }
            }
            else {
                if (!drawItemData._colorBuffer) {
                    const MHWRender::MVertexBufferDescriptor desc("",
                        MHWRender::MGeometry::kColor,
                        MHWRender::MGeometry::kFloat,
                        4);

                    drawItemData._colorBuffer.reset(
                        new MHWRender::MVertexBuffer(desc));
                }

                void* bufferData =
                    drawItemData._colorBuffer->acquire(numVertices, writeOnly);

                // Fill color and opacity into the float4 color stream.
                if (bufferData) {
                    _FillPrimvarData(static_cast<GfVec4f*>(bufferData),
                        numVertices, 0, requiresUnsharedVertices,
                        id, _topology, HdTokens->displayColor, colorArray, colorInterp);

                    _FillPrimvarData(static_cast<GfVec4f*>(bufferData),
                        numVertices, 3, requiresUnsharedVertices,
                        id, _topology, HdTokens->displayOpacity, alphaArray, alphaInterp);

                    stateToCommit._colorBufferData = static_cast<float*>(bufferData);
                }

                // Use fallback CPV shader if there is no material binding or
                // we failed to create a shader instance from the material.
                if (!stateToCommit._surfaceShader) {
                    stateToCommit._surfaceShader = _delegate->GetColorPerVertexShader();
                    stateToCommit._isTransparent =
                        (alphaInterp != HdInterpolationConstant || alphaArray[0] < 0.999f);
                }
            }
        }
    }

    // Prepare primvar buffers required by the material.
    if (material) {
        for (const TfToken& pv : matPrimvars) {
            // Color, opacity and normal have been prepared separately.
            if ((pv == HdTokens->displayColor) ||
                (pv == HdTokens->displayOpacity) ||
                (pv == HdTokens->normals))
                continue;

            if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, pv))
                continue;

            const auto it = _primvarSourceMap.find(pv);
            if (it == _primvarSourceMap.end())
                continue;

            const VtValue& value = it->second.data;
            const HdInterpolation& interp = it->second.interpolation;

            if (!value.IsArrayValued() || value.GetArraySize() == 0)
                continue;

            MHWRender::MVertexBuffer* buffer =
                drawItemData._primvarBuffers[pv].get();

            void* bufferData = nullptr;

            if (value.IsHolding<VtFloatArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor desc("",
                        MHWRender::MGeometry::kTexture,
                        MHWRender::MGeometry::kFloat, 1);

                    buffer = new MHWRender::MVertexBuffer(desc);
                    drawItemData._primvarBuffers[pv].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, writeOnly);
                    if (bufferData) {
                        _FillPrimvarData(static_cast<float*>(bufferData),
                            numVertices, 0, requiresUnsharedVertices,
                            id, _topology,
                            pv, value.UncheckedGet<VtFloatArray>(), interp);
                    }
                }
            }
            else if (value.IsHolding<VtVec2fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor desc("",
                        MHWRender::MGeometry::kTexture,
                        MHWRender::MGeometry::kFloat, 2);

                    buffer = new MHWRender::MVertexBuffer(desc);
                    drawItemData._primvarBuffers[pv].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, writeOnly);
                    if (bufferData) {
                        _FillPrimvarData(static_cast<GfVec2f*>(bufferData),
                            numVertices, 0, requiresUnsharedVertices,
                            id, _topology,
                            pv, value.UncheckedGet<VtVec2fArray>(), interp);
                    }
                }
            }
            else if (value.IsHolding<VtVec3fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor desc("",
                        MHWRender::MGeometry::kTexture,
                        MHWRender::MGeometry::kFloat, 3);

                    buffer = new MHWRender::MVertexBuffer(desc);
                    drawItemData._primvarBuffers[pv].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, writeOnly);
                    if (bufferData) {
                        _FillPrimvarData(static_cast<GfVec3f*>(bufferData),
                            numVertices, 0, requiresUnsharedVertices,
                            id, _topology,
                            pv, value.UncheckedGet<VtVec3fArray>(), interp);
                    }
                }
            }
            else if (value.IsHolding<VtVec4fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor desc("",
                        MHWRender::MGeometry::kTexture,
                        MHWRender::MGeometry::kFloat, 4);

                    buffer = new MHWRender::MVertexBuffer(desc);
                    drawItemData._primvarBuffers[pv].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, writeOnly);
                    if (bufferData) {
                        _FillPrimvarData(static_cast<GfVec4f*>(bufferData),
                            numVertices, 0, requiresUnsharedVertices,
                            id, _topology,
                            pv, value.UncheckedGet<VtVec4fArray>(), interp);
                    }
                }
            }
            else {
                TF_WARN("Unsupported primvar array");
            }

            stateToCommit._primvarBufferDataMap[pv] = static_cast<float*>(bufferData);
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

        MMatrix instanceMatrix;

        if ((desc.geomStyle == HdMeshGeomStyleHullEdgeOnly) &&
            !param->GetDrawScene().IsProxySelected()) {
            if (auto state = param->GetDrawScene().GetPrimSelectionState(id)) {
                for (const auto& indexArray : state->instanceIndices) {
                    for (const auto index : indexArray) {
                        transforms[index].Get(instanceMatrix.matrix);
                        instanceMatrix = worldMatrix * instanceMatrix;
                        stateToCommit._instanceTransforms.append(instanceMatrix);
                    }
                }
            }
        }
        else {
            for (size_t i = 0; i < transforms.size(); ++i) {
                transforms[i].Get(instanceMatrix.matrix);
                instanceMatrix = worldMatrix * instanceMatrix;
                stateToCommit._instanceTransforms.append(instanceMatrix);
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
        MHWRender::MIndexBuffer* indexBuffer = stateToCommit._drawItemData._indexBuffer.get();

        HdVP2DrawItem::PrimvarBufferMap& primvarBuffers = stateToCommit._drawItemData._primvarBuffers;

        MHWRender::MVertexBufferArray vertexBuffers;
        vertexBuffers.addBuffer("positions", positionsBuffer);

        if (colorBuffer)
            vertexBuffers.addBuffer("diffuseColor", colorBuffer);

        if (normalsBuffer)
            vertexBuffers.addBuffer("normals", normalsBuffer);

        for (auto& entry : primvarBuffers) {
            const TfToken& primvarName = entry.first;
            MHWRender::MVertexBuffer* primvarBuffer = entry.second.get();
            if (primvarBuffer) {
                vertexBuffers.addBuffer(primvarName.GetText(), primvarBuffer);
            }
        }

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
        for (const auto& entry : stateToCommit._primvarBufferDataMap) {
            const TfToken& primvarName = entry.first;
            float* primvarBufferData = entry.second;

            const auto it = primvarBuffers.find(primvarName);
            if (it != primvarBuffers.end()) {
                MHWRender::MVertexBuffer* primvarBuffer = it->second.get();
                if (primvarBuffer && primvarBufferData) {
                    primvarBuffer->commit(primvarBufferData);
                }
            }
        }

        // If available, something changed
        if (stateToCommit._indexBufferData && indexBuffer)
            indexBuffer->commit(stateToCommit._indexBufferData);

        // If available, something changed
        if (stateToCommit._surfaceShader != nullptr &&
            stateToCommit._surfaceShader != renderItem->getShader()) {
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

        // Special case for single instance prims. We will keep the original render item
        // to allow consolidation. We also keep drawItemData._instanceCount at 0, since
        // we can't remove instancing without recreating the render item.
        if(newInstanceCount == 1 && oldInstanceCount == 0) {
            setWantConsolidation(*renderItem, true);
        } else if(newInstanceCount > 0) {
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
    // While iterating primvars, we skip "points" (vertex positions) because
    // the points primvar is processed separately for direct access later. We
    // only call GetPrimvar on primvars that have been marked dirty.
    //
    // Currently, hydra doesn't have a good way of communicating changes in
    // the set of primvars, so we only ever add and update to the primvar set.

    for (size_t i = 0; i < HdInterpolationCount; i++) {
        const HdInterpolation interp = static_cast<HdInterpolation>(i);
        const HdPrimvarDescriptorVector primvars =
            GetPrimvarDescriptors(sceneDelegate, interp);

        for (const HdPrimvarDescriptor& pv: primvars) {
            if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name) &&
                pv.name != HdTokens->points) {
                const VtValue value = GetPrimvar(sceneDelegate, pv.name);
                _primvarSourceMap[pv.name] = { value, interp };
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
