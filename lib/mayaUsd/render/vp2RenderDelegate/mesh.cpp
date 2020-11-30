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
#include "mesh.h"

#include "bboxGeom.h"
#include "debugCodes.h"
#include "draw_item.h"
#include "instancer.h"
#include "material.h"
#include "render_delegate.h"
#include "tokens.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>
#include <mayaUsd/utils/colorSpace.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/meshUtil.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/smoothNormals.h>
#include <pxr/imaging/hd/version.h>
#include <pxr/imaging/hd/vertexAdjacency.h>

#include <maya/MMatrix.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionMask.h>

#include <numeric>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

//! Required primvars when there is no material binding.
const TfTokenVector sFallbackShaderPrimvars
    = { HdTokens->displayColor, HdTokens->displayOpacity, HdTokens->normals };

const MColor       kOpaqueBlue(0.0f, 0.0f, 1.0f, 1.0f); //!< Opaque blue
const MColor       kOpaqueGray(.18f, .18f, .18f, 1.0f); //!< Opaque gray
const unsigned int kNumColorChannels = 4;               //!< The number of color channels

const MString kPositionsStr("positions");       //!< Cached string for efficiency
const MString kNormalsStr("normals");           //!< Cached string for efficiency
const MString kDiffuseColorStr("diffuseColor"); //!< Cached string for efficiency
const MString kSolidColorStr("solidColor");     //!< Cached string for efficiency

//! A primvar vertex buffer data map indexed by primvar name.
using PrimvarBufferDataMap = std::unordered_map<TfToken, void*, TfToken::HashFunctor>;

//! \brief  Helper struct used to package all the changes into single commit task
//!         (such commit task will be executed on main-thread)
struct CommitState
{
    HdVP2DrawItem::RenderItemData& _drawItemData;

    //! If valid, new index buffer data to commit
    int* _indexBufferData { nullptr };
    //! If valid, new color buffer data to commit
    void* _colorBufferData { nullptr };
    //! If valid, new normals buffer data to commit
    void* _normalsBufferData { nullptr };
    //! If valid, new primvar buffer data to commit
    PrimvarBufferDataMap _primvarBufferDataMap;

    //! If valid, world matrix to set on the render item
    MMatrix* _worldMatrix { nullptr };

    //! If valid, bounding box to set on the render item
    MBoundingBox* _boundingBox { nullptr };

    //! if valid, enable or disable the render item
    bool* _enabled { nullptr };

    //! Instancing doesn't have dirty bits, every time we do update, we must update instance
    //! transforms
    MMatrixArray _instanceTransforms;

    //! Color array to support per-instance color and selection highlight.
    MFloatArray _instanceColors;

    //! If valid, new shader instance to set
    MHWRender::MShaderInstance* _shader { nullptr };

    //! Is this object transparent
    bool _isTransparent { false };

    //! If true, associate geometric buffers to the render item and trigger consolidation/instancing
    //! update
    bool _geometryDirty { false };

    //! Construct valid commit state
    CommitState(HdVP2DrawItem& item)
        : _drawItemData(item.GetRenderItemData())
    {
    }

    //! No default constructor, we need draw item and dirty bits.
    CommitState() = delete;
};

//! Helper utility function to fill primvar data to vertex buffer.
template <class DEST_TYPE, class SRC_TYPE>
void _FillPrimvarData(
    DEST_TYPE*               vertexBuffer,
    size_t                   numVertices,
    size_t                   channelOffset,
    const VtIntArray&        renderingToSceneFaceVtxIds,
    const MString&           rprimId,
    const HdMeshTopology&    topology,
    const TfToken&           primvarName,
    const VtArray<SRC_TYPE>& primvarData,
    const HdInterpolation&   primvarInterp)
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
        if (numVertices <= renderingToSceneFaceVtxIds.size()) {
            const unsigned int dataSize = primvarData.size();
            for (size_t v = 0; v < numVertices; v++) {
                unsigned int index = renderingToSceneFaceVtxIds[v];
                if (index < dataSize) {
                    SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                        reinterpret_cast<float*>(&vertexBuffer[v]) + channelOffset);
                    *pointer = primvarData[index];
                } else {
                    TF_DEBUG(HDVP2_DEBUG_MESH)
                        .Msg(
                            "Invalid Hydra prim '%s': "
                            "primvar %s has %u elements, while its topology "
                            "references face vertex index %u.\n",
                            rprimId.asChar(),
                            primvarName.GetText(),
                            dataSize,
                            index);
                }
            }
        } else {
            TF_CODING_ERROR(
                "Invalid Hydra prim '%s': "
                "requires %zu vertices, while the number of elements in "
                "renderingToSceneFaceVtxIds is %zu. Skipping primvar update.",
                rprimId.asChar(),
                numVertices,
                renderingToSceneFaceVtxIds.size());

            memset(vertexBuffer, 0, sizeof(DEST_TYPE) * numVertices);
        }
        break;
    case HdInterpolationUniform: {
        const VtIntArray& faceVertexCounts = topology.GetFaceVertexCounts();
        const size_t      numFaces = faceVertexCounts.size();
        if (numFaces <= primvarData.size()) {
            // The primvar has more data than needed, we issue a warning but
            // don't skip update. Truncate the buffer to the expected length.
            if (numFaces < primvarData.size()) {
                TF_DEBUG(HDVP2_DEBUG_MESH)
                    .Msg(
                        "Invalid Hydra prim '%s': "
                        "primvar %s has %zu elements, while its topology "
                        "references only upto element index %zu.\n",
                        rprimId.asChar(),
                        primvarName.GetText(),
                        primvarData.size(),
                        numFaces);
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
        } else {
            // The primvar has less data than needed. Issue warning and skip
            // update like what is done in HdStMesh.
            TF_DEBUG(HDVP2_DEBUG_MESH)
                .Msg(
                    "Invalid Hydra prim '%s': "
                    "primvar %s has only %zu elements, while its topology expects "
                    "at least %zu elements. Skipping primvar update.\n",
                    rprimId.asChar(),
                    primvarName.GetText(),
                    primvarData.size(),
                    numFaces);

            memset(vertexBuffer, 0, sizeof(DEST_TYPE) * numVertices);
        }
        break;
    }
    case HdInterpolationFaceVarying:
        // Unshared vertex layout is required for face-varying primvars, in
        // this case renderingToSceneFaceVtxIds is a natural sequence starting
        // from 0, thus we can save a lookup into the table. If the assumption
        // about the natural sequence is changed, we will need the lookup and
        // remap indices.
        if (numVertices <= primvarData.size()) {
            // If the primvar has more data than needed, we issue a warning,
            // but don't skip the primvar update. Truncate the buffer to the
            // expected length.
            if (numVertices < primvarData.size()) {
                TF_DEBUG(HDVP2_DEBUG_MESH)
                    .Msg(
                        "Invalid Hydra prim '%s': "
                        "primvar %s has %zu elements, while its topology references "
                        "only upto element index %zu.\n",
                        rprimId.asChar(),
                        primvarName.GetText(),
                        primvarData.size(),
                        numVertices);
            }

            if (channelOffset == 0 && std::is_same<DEST_TYPE, SRC_TYPE>::value) {
                const void* source = static_cast<const void*>(primvarData.cdata());
                memcpy(vertexBuffer, source, sizeof(DEST_TYPE) * numVertices);
            } else {
                for (size_t v = 0; v < numVertices; v++) {
                    SRC_TYPE* pointer = reinterpret_cast<SRC_TYPE*>(
                        reinterpret_cast<float*>(&vertexBuffer[v]) + channelOffset);
                    *pointer = primvarData[v];
                }
            }
        } else {
            // It is unexpected to have less data than we index into. Issue
            // a warning and skip update.
            TF_DEBUG(HDVP2_DEBUG_MESH)
                .Msg(
                    "Invalid Hydra prim '%s': "
                    "primvar %s has only %zu elements, while its topology expects "
                    "at least %zu elements. Skipping primvar update.\n",
                    rprimId.asChar(),
                    primvarName.GetText(),
                    primvarData.size(),
                    numVertices);

            memset(vertexBuffer, 0, sizeof(DEST_TYPE) * numVertices);
        }
        break;
    default:
        TF_CODING_ERROR(
            "Invalid Hydra prim '%s': "
            "unimplemented interpolation %d for primvar %s",
            rprimId.asChar(),
            (int)primvarInterp,
            primvarName.GetText());
        break;
    }
}

//! If there is uniform or face-varying primvar, we have to create unshared
//! vertex layout on CPU because SSBO technique is not widely supported by
//! GPUs and 3D APIs.
bool _IsUnsharedVertexLayoutRequired(const PrimvarSourceMap& primvarSources)
{
    for (const auto& it : primvarSources) {
        const HdInterpolation interp = it.second.interpolation;
        if (interp == HdInterpolationUniform || interp == HdInterpolationFaceVarying) {
            return true;
        }
    }

    return false;
}

//! Helper utility function to get number of edge indices
unsigned int _GetNumOfEdgeIndices(const HdMeshTopology& topology)
{
    const VtIntArray& faceVertexCounts = topology.GetFaceVertexCounts();

    unsigned int numIndex = 0;
    for (std::size_t i = 0; i < faceVertexCounts.size(); i++) {
        numIndex += faceVertexCounts[i];
    }
    numIndex *= 2; // each edge has two ends.
    return numIndex;
}

//! Helper utility function to extract edge indices
void _FillEdgeIndices(int* indices, const HdMeshTopology& topology)
{
    const VtIntArray& faceVertexCounts = topology.GetFaceVertexCounts();
    const int*        currentFaceStart = topology.GetFaceVertexIndices().cdata();
    for (std::size_t faceId = 0; faceId < faceVertexCounts.size(); faceId++) {
        int numVertexIndicesInFace = faceVertexCounts[faceId];
        if (numVertexIndicesInFace >= 2) {
            for (int faceVertexId = 0; faceVertexId < numVertexIndicesInFace; faceVertexId++) {
                bool isLastVertex = faceVertexId == numVertexIndicesInFace - 1;
                *(indices++) = *(currentFaceStart + faceVertexId);
                *(indices++)
                    = isLastVertex ? *currentFaceStart : *(currentFaceStart + faceVertexId + 1);
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
} // namespace

//! \brief  Constructor
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
HdVP2Mesh::HdVP2Mesh(HdVP2RenderDelegate* delegate, const SdfPath& id)
    : HdMesh(id)
#else
HdVP2Mesh::HdVP2Mesh(HdVP2RenderDelegate* delegate, const SdfPath& id, const SdfPath& instancerId)
    : HdMesh(id, instancerId)
#endif
    , _delegate(delegate)
    , _rprimId(id.GetText())
{
    const MHWRender::MVertexBufferDescriptor vbDesc(
        "", MHWRender::MGeometry::kPosition, MHWRender::MGeometry::kFloat, 3);
    _meshSharedData._positionsBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));
}

//! \brief  Synchronize VP2 state with scene delegate state based on dirty bits and representation
void HdVP2Mesh::Sync(
    HdSceneDelegate* delegate,
    HdRenderParam*   renderParam,
    HdDirtyBits*     dirtyBits,
    const TfToken&   reprToken)
{
    // We don't create a repr for the selection token because this token serves
    // for selection state update only. Return early to reserve dirty bits so
    // they can be used to sync regular reprs later.
    if (reprToken == HdVP2ReprTokens->selection) {
        return;
    }

    // We don't update the repr if it is hidden by the render tags (purpose)
    // of the ProxyRenderDelegate. In additional, we need to hide any already
    // existing render items because they should not be drawn.
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    if (!drawScene.DrawRenderTag(delegate->GetRenderIndex().GetRenderTag(GetId()))) {
        _HideAllDrawItems(reprToken);
        *dirtyBits &= ~(
            HdChangeTracker::DirtyRenderTag
#ifdef ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
            | HdChangeTracker::DirtyVisibility
#endif
        );
        return;
    }

    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2,
        _rprimId.asChar(),
        "HdVP2Mesh::Sync");

    const SdfPath& id = GetId();

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(), delegate->GetMaterialId(id));
    }

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals)
        || HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)) {
        const HdVP2Material* material = static_cast<const HdVP2Material*>(
            delegate->GetRenderIndex().GetSprim(HdPrimTypeTokens->material, GetMaterialId()));

        const TfTokenVector& requiredPrimvars = material && material->GetSurfaceShader()
            ? material->GetRequiredPrimvars()
            : sFallbackShaderPrimvars;

        _UpdatePrimvarSources(delegate, *dirtyBits, requiredPrimvars);
    }

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        _meshSharedData._topology = GetMeshTopology(delegate);

        const HdMeshTopology& topology = _meshSharedData._topology;
        const VtIntArray&     faceVertexIndices = topology.GetFaceVertexIndices();
        const size_t          numFaceVertexIndices = faceVertexIndices.size();

        VtIntArray newFaceVertexIndices;
        newFaceVertexIndices.resize(numFaceVertexIndices);

        if (_IsUnsharedVertexLayoutRequired(_meshSharedData._primvarSourceMap)) {
            _meshSharedData._numVertices = numFaceVertexIndices;
            _meshSharedData._renderingToSceneFaceVtxIds = faceVertexIndices;

            // Fill with sequentially increasing values, starting from 0. The new
            // face vertex indices will be used to populate index data for unshared
            // vertex layout. Note that _FillPrimvarData assumes this sequence to
            // be used for face-varying primvars and saves lookup and remapping
            // with _renderingToSceneFaceVtxIds, so in case we change the array we
            // should update _FillPrimvarData() code to remap indices correctly.
            std::iota(newFaceVertexIndices.begin(), newFaceVertexIndices.end(), 0);
        } else {
            _meshSharedData._numVertices = topology.GetNumPoints();
            _meshSharedData._renderingToSceneFaceVtxIds.clear();

            // Allocate large enough memory with initial value of -1 to indicate
            // the rendering face vertex index is not determined yet.
            std::vector<int> authorToRenderFaceVtxIds(numFaceVertexIndices, -1);

            // Sort vertices to avoid drastically jumping indices. Cache efficiency
            // is important to fast rendering performance for dense mesh.
            for (size_t i = 0; i < numFaceVertexIndices; i++) {
                const int authorFaceVtxId = faceVertexIndices[i];

                int renderFaceVtxId = authorToRenderFaceVtxIds[authorFaceVtxId];
                if (renderFaceVtxId < 0) {
                    renderFaceVtxId = _meshSharedData._renderingToSceneFaceVtxIds.size();
                    _meshSharedData._renderingToSceneFaceVtxIds.push_back(authorFaceVtxId);

                    authorToRenderFaceVtxIds[authorFaceVtxId] = renderFaceVtxId;
                }

                newFaceVertexIndices[i] = renderFaceVtxId;
            }
        }

        _meshSharedData._renderingTopology = HdMeshTopology(
            topology.GetScheme(),
            topology.GetOrientation(),
            topology.GetFaceVertexCounts(),
            newFaceVertexIndices,
            topology.GetHoleIndices(),
            topology.GetRefineLevel());
    }

    // Prepare position buffer. It is shared among all draw items so it should
    // be updated only once when it gets dirty.
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        const VtValue value = delegate->Get(id, HdTokens->points);
        _meshSharedData._points = value.Get<VtVec3fArray>();

        const HdMeshTopology& topology = _meshSharedData._topology;
        const size_t          numVertices = _meshSharedData._numVertices;

        void* bufferData = _meshSharedData._positionsBuffer->acquire(numVertices, true);
        if (bufferData) {
            _FillPrimvarData(
                static_cast<GfVec3f*>(bufferData),
                numVertices,
                0,
                _meshSharedData._renderingToSceneFaceVtxIds,
                _rprimId,
                topology,
                HdTokens->points,
                _meshSharedData._points,
                HdInterpolationVertex);

            // Capture class member for lambda
            MHWRender::MVertexBuffer* const positionsBuffer
                = _meshSharedData._positionsBuffer.get();
            const MString& rprimId = _rprimId;

            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [positionsBuffer, bufferData, rprimId]() {
                    MProfilingScope profilingScope(
                        HdVP2RenderDelegate::sProfilerCategory,
                        MProfiler::kColorC_L2,
                        rprimId.asChar(),
                        "CommitPositions");

                    positionsBuffer->commit(bufferData);
                });
        }
    }

    if (HdChangeTracker::IsExtentDirty(*dirtyBits, id)) {
        _sharedData.bounds.SetRange(delegate->GetExtent(id));
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        _sharedData.bounds.SetMatrix(delegate->GetTransform(id));
    }

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _sharedData.visible = delegate->GetVisible(id);
    }

    if (*dirtyBits
        & (HdChangeTracker::DirtyRenderTag
#ifdef ENABLE_RENDERTAG_VISIBILITY_WORKAROUND
           | HdChangeTracker::DirtyVisibility
#endif
           )) {
        _meshSharedData._renderTag = delegate->GetRenderTag(id);
    }

    *dirtyBits = HdChangeTracker::Clean;

    // Draw item update is controlled by its own dirty bits.
    _UpdateRepr(delegate, reprToken);
}

/*! \brief  Returns the minimal set of dirty bits to place in the
            change tracker for use in the first sync of this prim.
*/
HdDirtyBits HdVP2Mesh::GetInitialDirtyBitsMask() const
{
    constexpr HdDirtyBits bits = HdChangeTracker::InitRepr | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyMaterialId | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyVisibility | HdChangeTracker::DirtyInstancer
        | HdChangeTracker::DirtyRenderTag | DirtySelectionHighlight;

    return bits;
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
HdDirtyBits HdVP2Mesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // If subdiv tags are dirty, topology needs to be recomputed.
    // The latter implies we'll need to recompute all primvar data.
    // Any data fetched by the scene delegate should be marked dirty here.
    if (bits & HdChangeTracker::DirtySubdivTags) {
        bits
            |= (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyNormals
                | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyTopology
                | HdChangeTracker::DirtyDisplayStyle);
    } else if (bits & HdChangeTracker::DirtyTopology) {
        // Unlike basis curves, we always request refineLevel when topology is
        // dirty
        bits |= HdChangeTracker::DirtySubdivTags | HdChangeTracker::DirtyDisplayStyle;
    }

    // A change of material means that the Quadrangulate state may have
    // changed.
    if (bits & HdChangeTracker::DirtyMaterialId) {
        bits
            |= (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyNormals
                | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyTopology);
    }

    // If points, display style, or topology changed, recompute normals.
    if (bits
        & (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyDisplayStyle
           | HdChangeTracker::DirtyTopology)) {
        bits |= _customDirtyBitsInUse & (DirtySmoothNormals | DirtyFlatNormals);
    }

    // If the topology is dirty, recompute custom indices resources.
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse & (DirtyIndices | DirtyHullIndices | DirtyPointsIndices);
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

    // Propagate dirty bits to all draw items.
    for (const std::pair<TfToken, HdReprSharedPtr>& pair : _reprs) {
        const HdReprSharedPtr& repr = pair.second;
        const auto&            items = repr->GetDrawItems();
#if HD_API_VERSION < 35
        for (HdDrawItem* item : items) {
            if (HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item)) {
#else
        for (const HdRepr::DrawItemUniquePtr& item : items) {
            if (HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get())) {
#endif
                drawItem->SetDirtyBits(bits);
            }
        }
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
void HdVP2Mesh::_InitRepr(const TfToken& reprToken, HdDirtyBits* dirtyBits)
{
    auto* const         param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (ARCH_UNLIKELY(!subSceneContainer))
        return;

    // Update selection state on demand or when it is a new Rprim. DirtySelection
    // will be propagated to all draw items, to trigger sync for each repr.
    if (reprToken == HdVP2ReprTokens->selection || _reprs.empty()) {
        const HdVP2SelectionStatus selectionStatus
            = param->GetDrawScene().GetSelectionStatus(GetId());
        if (_selectionStatus != selectionStatus) {
            _selectionStatus = selectionStatus;
            *dirtyBits |= DirtySelection;
        } else if (_selectionStatus == kPartiallySelected) {
            *dirtyBits |= DirtySelection;
        }

        // We don't create a repr for the selection token because it serves for
        // selection state update only. Return from here.
        if (reprToken == HdVP2ReprTokens->selection)
            return;
    }

    // If the repr has any draw item with the DirtySelection bit, mark the
    // DirtySelectionHighlight bit to invoke the synchronization call.
    _ReprVector::iterator it
        = std::find_if(_reprs.begin(), _reprs.end(), _ReprComparator(reprToken));
    if (it != _reprs.end()) {
        const HdReprSharedPtr& repr = it->second;
        const auto&            items = repr->GetDrawItems();
#if HD_API_VERSION < 35
        for (const HdDrawItem* item : items) {
            const HdVP2DrawItem* drawItem = static_cast<const HdVP2DrawItem*>(item);
#else
        for (const HdRepr::DrawItemUniquePtr& item : items) {
            const HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get());
#endif
            if (drawItem && (drawItem->GetDirtyBits() & DirtySelection)) {
                *dirtyBits |= DirtySelectionHighlight;
                break;
            }
        }
        return;
    }

#if USD_VERSION_NUM > 2002
    _reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
#else
    _reprs.emplace_back(reprToken, boost::make_shared<HdRepr>());
#endif
    HdReprSharedPtr repr = _reprs.back().second;

    // set dirty bit to say we need to sync a new repr
    *dirtyBits |= HdChangeTracker::NewRepr;

    _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);

    for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
        const HdMeshReprDesc& desc = descs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleInvalid) {
            continue;
        }

#if HD_API_VERSION < 35
        auto* drawItem = new HdVP2DrawItem(_delegate, &_sharedData);
#else
        std::unique_ptr<HdVP2DrawItem> drawItem
            = std::make_unique<HdVP2DrawItem>(_delegate, &_sharedData);
#endif

        const MString& renderItemName = drawItem->GetRenderItemName();

        MHWRender::MRenderItem* renderItem = nullptr;

        switch (desc.geomStyle) {
        case HdMeshGeomStyleHull: renderItem = _CreateSmoothHullRenderItem(renderItemName); break;
        case HdMeshGeomStyleHullEdgeOnly:
            // The smoothHull repr uses the wireframe item for selection
            // highlight only.
            if (reprToken == HdReprTokens->smoothHull) {
                renderItem = _CreateSelectionHighlightRenderItem(renderItemName);
                drawItem->SetUsage(HdVP2DrawItem::kSelectionHighlight);
            }
            // The item is used for wireframe display and selection highlight.
            else if (reprToken == HdReprTokens->wire) {
                renderItem = _CreateWireframeRenderItem(renderItemName);
                drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
            }
            // The item is used for bbox display and selection highlight.
            else if (reprToken == HdVP2ReprTokens->bbox) {
                renderItem = _CreateBoundingBoxRenderItem(renderItemName);
                drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
            }
            break;
        case HdMeshGeomStylePoints: renderItem = _CreatePointsRenderItem(renderItemName); break;
        default: TF_WARN("Unsupported geomStyle"); break;
        }

        if (renderItem) {
            // Store the render item pointer to avoid expensive lookup in the
            // subscene container.
            drawItem->SetRenderItem(renderItem);

            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [subSceneContainer, renderItem]() { subSceneContainer->add(renderItem); });
        }

        if (desc.geomStyle == HdMeshGeomStyleHull) {
            if (desc.flatShadingEnabled) {
                if (!(_customDirtyBitsInUse & DirtyFlatNormals)) {
                    _customDirtyBitsInUse |= DirtyFlatNormals;
                    *dirtyBits |= DirtyFlatNormals;
                }
            } else {
                if (!(_customDirtyBitsInUse & DirtySmoothNormals)) {
                    _customDirtyBitsInUse |= DirtySmoothNormals;
                    *dirtyBits |= DirtySmoothNormals;
                }
            }
        }

#if HD_API_VERSION < 35
        repr->AddDrawItem(drawItem);
#else
        repr->AddDrawItem(std::move(drawItem));
#endif
    }
}

/*! \brief  Update the named repr object for this Rprim.

    Repr objects are created to support specific reprName tokens, and contain a list of
    HdVP2DrawItems and corresponding RenderItems.
*/
void HdVP2Mesh::_UpdateRepr(HdSceneDelegate* sceneDelegate, const TfToken& reprToken)
{
    HdReprSharedPtr const& curRepr = _GetRepr(reprToken);
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
        const HdMeshReprDesc& desc = reprDescs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleHull) {
            if (desc.flatShadingEnabled) {
                requireFlatNormals = true;
            } else {
                requireSmoothNormals = true;
            }
        }
    }

    // For each relevant draw item, update dirty buffer sources.
    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc& desc = reprDescs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleInvalid) {
            continue;
        }

        auto* drawItem = static_cast<HdVP2DrawItem*>(curRepr->GetDrawItem(drawItemIndex++));
        if (drawItem) {
            _UpdateDrawItem(
                sceneDelegate, drawItem, desc, requireSmoothNormals, requireFlatNormals);
        }
    }
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in CommitState and enqueued for Commit on main-thread using CommitTasks
*/
void HdVP2Mesh::_UpdateDrawItem(
    HdSceneDelegate*      sceneDelegate,
    HdVP2DrawItem*        drawItem,
    const HdMeshReprDesc& desc,
    bool                  requireSmoothNormals,
    bool                  requireFlatNormals)
{
    const MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
    if (ARCH_UNLIKELY(!renderItem)) {
        return;
    }

    HdDirtyBits itemDirtyBits = drawItem->GetDirtyBits();

    // We don't need to update the dedicated selection highlight item when there
    // is no selection highlight change and the mesh is not selected. Draw item
    // has its own dirty bits, so update will be done when it shows in viewport.
    const bool isDedicatedSelectionHighlightItem
        = drawItem->MatchesUsage(HdVP2DrawItem::kSelectionHighlight);
    if (isDedicatedSelectionHighlightItem && ((itemDirtyBits & DirtySelectionHighlight) == 0)
        && (_selectionStatus == kUnselected)) {
        return;
    }

    CommitState                    stateToCommit(*drawItem);
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._drawItemData;

    const SdfPath& id = GetId();

    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    const HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();

    const HdMeshTopology& topology = _meshSharedData._topology;
    const auto&           primvarSourceMap = _meshSharedData._primvarSourceMap;
    const size_t          numVertices = _meshSharedData._numVertices;

    // The bounding box item uses a globally-shared geometry data therefore it
    // doesn't need to extract index data from topology. Points use non-indexed
    // draw.
    const bool isBBoxItem = (renderItem->drawMode() == MHWRender::MGeometry::kBoundingBox);
    const bool isPointSnappingItem = (renderItem->primitive() == MHWRender::MGeometry::kPoints);
    const bool requiresIndexUpdate = !isBBoxItem && !isPointSnappingItem;

    // Prepare index buffer.
    if (requiresIndexUpdate && (itemDirtyBits & HdChangeTracker::DirtyTopology)) {
        const HdMeshTopology& topologyToUse = _meshSharedData._renderingTopology;

        if (desc.geomStyle == HdMeshGeomStyleHull) {
            HdMeshUtil   meshUtil(&topologyToUse, id);
            VtVec3iArray trianglesFaceVertexIndices;
            VtIntArray   primitiveParam;
            meshUtil.ComputeTriangleIndices(&trianglesFaceVertexIndices, &primitiveParam, nullptr);

            const int numIndex = trianglesFaceVertexIndices.size() * 3;

            stateToCommit._indexBufferData
                = static_cast<int*>(drawItemData._indexBuffer->acquire(numIndex, true));

            memcpy(
                stateToCommit._indexBufferData,
                trianglesFaceVertexIndices.data(),
                numIndex * sizeof(int));
        } else if (desc.geomStyle == HdMeshGeomStyleHullEdgeOnly) {
            unsigned int numIndex = _GetNumOfEdgeIndices(topologyToUse);

            stateToCommit._indexBufferData
                = static_cast<int*>(drawItemData._indexBuffer->acquire(numIndex, true));

            _FillEdgeIndices(stateToCommit._indexBufferData, topologyToUse);
        }
    }

    if (desc.geomStyle == HdMeshGeomStyleHull) {
        // Prepare normal buffer.
        VtVec3fArray    normals;
        HdInterpolation interp = HdInterpolationConstant;

        const auto it = primvarSourceMap.find(HdTokens->normals);
        if (it != primvarSourceMap.end()) {
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
            prepareNormals = ((itemDirtyBits & HdChangeTracker::DirtyNormals) != 0);
        } else if (requireSmoothNormals && (itemDirtyBits & DirtySmoothNormals)) {
            // note: normals gets dirty when points are marked as dirty,
            // at change tracker.
            // HdC_TODO: move the normals computation to GPU to save expensive
            // computation and buffer transfer.
            Hd_VertexAdjacencySharedPtr adjacency(new Hd_VertexAdjacency());
            HdBufferSourceSharedPtr     adjacencyComputation
                = adjacency->GetSharedAdjacencyBuilderComputation(&topology);
            adjacencyComputation->Resolve(); // IS the adjacency updated now?

            // Only the points referenced by the topology are used to compute
            // smooth normals.
            normals = Hd_SmoothNormals::ComputeSmoothNormals(
                adjacency.get(), _meshSharedData._points.size(), _meshSharedData._points.cdata());

            interp = HdInterpolationVertex;

            prepareNormals = !normals.empty();
        }

        if (prepareNormals) {
            if (!drawItemData._normalsBuffer) {
                const MHWRender::MVertexBufferDescriptor vbDesc(
                    "", MHWRender::MGeometry::kNormal, MHWRender::MGeometry::kFloat, 3);

                drawItemData._normalsBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));
            }

            void* bufferData = drawItemData._normalsBuffer->acquire(numVertices, true);
            if (bufferData) {
                _FillPrimvarData(
                    static_cast<GfVec3f*>(bufferData),
                    numVertices,
                    0,
                    _meshSharedData._renderingToSceneFaceVtxIds,
                    _rprimId,
                    topology,
                    HdTokens->normals,
                    normals,
                    interp);

                stateToCommit._normalsBufferData = bufferData;
            }
        }

        // Prepare color buffer.
        if ((itemDirtyBits & HdChangeTracker::DirtyMaterialId) != 0) {
            const HdVP2Material* material = static_cast<const HdVP2Material*>(
                renderIndex.GetSprim(HdPrimTypeTokens->material, GetMaterialId()));

            if (material) {
                MHWRender::MShaderInstance* shader = material->GetSurfaceShader();
                if (shader != nullptr && shader != drawItemData._shader) {
                    drawItemData._shader = shader;
                    stateToCommit._shader = shader;
                    stateToCommit._isTransparent = shader->isTransparent();
                }
            }
        }

        auto itColor = primvarSourceMap.find(HdTokens->displayColor);
        auto itOpacity = primvarSourceMap.find(HdTokens->displayOpacity);

        if (((itemDirtyBits & HdChangeTracker::DirtyPrimvar) != 0)
            && (itColor != primvarSourceMap.end() || itOpacity != primvarSourceMap.end())) {
            VtVec3fArray colorArray;
            VtFloatArray alphaArray;

            HdInterpolation colorInterp = HdInterpolationConstant;
            HdInterpolation alphaInterp = HdInterpolationConstant;

            if (itColor != primvarSourceMap.end()) {
                const VtValue& value = itColor->second.data;
                if (value.IsHolding<VtVec3fArray>() && value.GetArraySize() > 0) {
                    colorArray = value.UncheckedGet<VtVec3fArray>();
                    colorInterp = itColor->second.interpolation;
                }
            }

            if (itOpacity != primvarSourceMap.end()) {
                const VtValue& value = itOpacity->second.data;
                if (value.IsHolding<VtFloatArray>() && value.GetArraySize() > 0) {
                    alphaArray = value.UncheckedGet<VtFloatArray>();
                    alphaInterp = itOpacity->second.interpolation;
                }
            }

            // If color/opacity is not found, the 18% gray color will be used
            // to match the default color of Hydra Storm.
            if (colorArray.empty()) {
                colorArray.push_back(GfVec3f(0.18f, 0.18f, 0.18f));
                colorInterp = HdInterpolationConstant;
            }

            if (alphaArray.empty()) {
                alphaArray.push_back(1.0f);
                alphaInterp = HdInterpolationConstant;
            }

            bool prepareCPVBuffer = true;

            // Use fallback shader if there is no material binding or we failed to create a shader
            // instance for the material.
            if (!stateToCommit._shader) {
                MHWRender::MShaderInstance* shader = nullptr;

                if (colorInterp == HdInterpolationConstant
                    && alphaInterp == HdInterpolationConstant) {
                    prepareCPVBuffer = false;

                    const GfVec3f& clr3f = UsdMayaColorSpace::ConvertLinearToMaya(colorArray[0]);
                    const MColor   color(clr3f[0], clr3f[1], clr3f[2], alphaArray[0]);
                    shader = _delegate->GetFallbackShader(color);
                } else {
                    shader = _delegate->GetFallbackCPVShader();
                }

                if (shader != nullptr && shader != drawItemData._shader) {
                    drawItemData._shader = shader;
                    stateToCommit._shader = shader;
                }
            }

            if (prepareCPVBuffer) {
                if (!drawItemData._colorBuffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", MHWRender::MGeometry::kColor, MHWRender::MGeometry::kFloat, 4);

                    drawItemData._colorBuffer.reset(new MHWRender::MVertexBuffer(vbDesc));
                }

                void* bufferData = drawItemData._colorBuffer->acquire(numVertices, true);

                // Fill color and opacity into the float4 color stream.
                if (bufferData) {
                    _FillPrimvarData(
                        static_cast<GfVec4f*>(bufferData),
                        numVertices,
                        0,
                        _meshSharedData._renderingToSceneFaceVtxIds,
                        _rprimId,
                        topology,
                        HdTokens->displayColor,
                        colorArray,
                        colorInterp);

                    _FillPrimvarData(
                        static_cast<GfVec4f*>(bufferData),
                        numVertices,
                        3,
                        _meshSharedData._renderingToSceneFaceVtxIds,
                        _rprimId,
                        topology,
                        HdTokens->displayOpacity,
                        alphaArray,
                        alphaInterp);

                    stateToCommit._colorBufferData = bufferData;
                }
            }

            // It is possible that all elements in the opacity array are 1.
            // Due to the performance indication about transparency, we have to
            // traverse the array and enable transparency only when needed.
            if (!stateToCommit._isTransparent) {
                if (alphaInterp == HdInterpolationConstant) {
                    stateToCommit._isTransparent = (alphaArray[0] < 0.999f);
                } else {
                    for (size_t i = 0; i < alphaArray.size(); i++) {
                        if (alphaArray[i] < 0.999f) {
                            stateToCommit._isTransparent = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Prepare primvar buffers.
    if ((desc.geomStyle == HdMeshGeomStyleHull)
        && (itemDirtyBits & HdChangeTracker::DirtyPrimvar)) {
        for (const auto& it : primvarSourceMap) {
            const TfToken& token = it.first;
            // Color, opacity and normal have been prepared separately.
            if ((token == HdTokens->displayColor) || (token == HdTokens->displayOpacity)
                || (token == HdTokens->normals))
                continue;

            const VtValue&         value = it.second.data;
            const HdInterpolation& interp = it.second.interpolation;

            if (!value.IsArrayValued() || value.GetArraySize() == 0)
                continue;

            MHWRender::MVertexBuffer* buffer = drawItemData._primvarBuffers[token].get();

            void* bufferData = nullptr;

            if (value.IsHolding<VtFloatArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 1);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    drawItemData._primvarBuffers[token].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, true);
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<float*>(bufferData),
                            numVertices,
                            0,
                            _meshSharedData._renderingToSceneFaceVtxIds,
                            _rprimId,
                            topology,
                            token,
                            value.UncheckedGet<VtFloatArray>(),
                            interp);
                    }
                }
            } else if (value.IsHolding<VtVec2fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 2);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    drawItemData._primvarBuffers[token].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, true);
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<GfVec2f*>(bufferData),
                            numVertices,
                            0,
                            _meshSharedData._renderingToSceneFaceVtxIds,
                            _rprimId,
                            topology,
                            token,
                            value.UncheckedGet<VtVec2fArray>(),
                            interp);
                    }
                }
            } else if (value.IsHolding<VtVec3fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 3);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    drawItemData._primvarBuffers[token].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, true);
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<GfVec3f*>(bufferData),
                            numVertices,
                            0,
                            _meshSharedData._renderingToSceneFaceVtxIds,
                            _rprimId,
                            topology,
                            token,
                            value.UncheckedGet<VtVec3fArray>(),
                            interp);
                    }
                }
            } else if (value.IsHolding<VtVec4fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", MHWRender::MGeometry::kTexture, MHWRender::MGeometry::kFloat, 4);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    drawItemData._primvarBuffers[token].reset(buffer);
                }

                if (buffer) {
                    bufferData = buffer->acquire(numVertices, true);
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<GfVec4f*>(bufferData),
                            numVertices,
                            0,
                            _meshSharedData._renderingToSceneFaceVtxIds,
                            _rprimId,
                            topology,
                            token,
                            value.UncheckedGet<VtVec4fArray>(),
                            interp);
                    }
                }
            } else {
                TF_WARN("Unsupported primvar array");
            }

            stateToCommit._primvarBufferDataMap[token] = bufferData;
        }
    }

    // Local bounds
    const GfRange3d& range = _sharedData.bounds.GetRange();

    // Bounds are updated through MPxSubSceneOverride::setGeometryForRenderItem()
    // which is expensive, so it is updated only when it gets expanded in order
    // to reduce calling frequence.
    if (itemDirtyBits & HdChangeTracker::DirtyExtent) {
        const GfRange3d& rangeToUse
            = isBBoxItem ? _delegate->GetSharedBBoxGeom().GetRange() : range;

        bool boundingBoxExpanded = false;

        const GfVec3d& min = rangeToUse.GetMin();
        const MPoint   pntMin(min[0], min[1], min[2]);
        if (!drawItemData._boundingBox.contains(pntMin)) {
            drawItemData._boundingBox.expand(pntMin);
            boundingBoxExpanded = true;
        }

        const GfVec3d& max = rangeToUse.GetMax();
        const MPoint   pntMax(max[0], max[1], max[2]);
        if (!drawItemData._boundingBox.contains(pntMax)) {
            drawItemData._boundingBox.expand(pntMax);
            boundingBoxExpanded = true;
        }

        if (boundingBoxExpanded) {
            stateToCommit._boundingBox = &drawItemData._boundingBox;
        }
    }

    // Local-to-world transformation
    MMatrix& worldMatrix = drawItemData._worldMatrix;
    _sharedData.bounds.GetMatrix().Get(worldMatrix.matrix);

    // The bounding box draw item uses a globally-shared unit wire cube as the
    // geometry and transfers scale and offset of the bounds to world matrix.
    if (isBBoxItem) {
        if ((itemDirtyBits & (HdChangeTracker::DirtyExtent | HdChangeTracker::DirtyTransform))
            && !range.IsEmpty()) {
            const GfVec3d midpoint = range.GetMidpoint();
            const GfVec3d size = range.GetSize();

            MPoint midp(midpoint[0], midpoint[1], midpoint[2]);
            midp *= worldMatrix;

            auto& m = worldMatrix.matrix;
            m[0][0] *= size[0];
            m[0][1] *= size[0];
            m[0][2] *= size[0];
            m[0][3] *= size[0];
            m[1][0] *= size[1];
            m[1][1] *= size[1];
            m[1][2] *= size[1];
            m[1][3] *= size[1];
            m[2][0] *= size[2];
            m[2][1] *= size[2];
            m[2][2] *= size[2];
            m[2][3] *= size[2];
            m[3][0] = midp[0];
            m[3][1] = midp[1];
            m[3][2] = midp[2];
            m[3][3] = midp[3];

            stateToCommit._worldMatrix = &drawItemData._worldMatrix;
        }
    } else if (itemDirtyBits & HdChangeTracker::DirtyTransform) {
        stateToCommit._worldMatrix = &drawItemData._worldMatrix;
    }

    // If the mesh is instanced, create one new instance per transform.
    // The current instancer invalidation tracking makes it hard for
    // us to tell whether transforms will be dirty, so this code
    // pulls them every time something changes.
    // If the mesh is instanced but has 0 instance transforms remember that
    // so the render item can be hidden.

#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    // Update our instance topology if necessary.
    _UpdateInstancer(sceneDelegate, &itemDirtyBits);
#endif

    bool instancerWithNoInstances = false;
    if (!GetInstancerId().IsEmpty()) {

        // Retrieve instance transforms from the instancer.
        HdInstancer*    instancer = renderIndex.GetInstancer(GetInstancerId());
        VtMatrix4dArray transforms
            = static_cast<HdVP2Instancer*>(instancer)->ComputeInstanceTransforms(id);

        MMatrix            instanceMatrix;
        const unsigned int instanceCount = transforms.size();

        if (0 == instanceCount) {
            instancerWithNoInstances = true;
        } else if (!drawItem->ContainsUsage(HdVP2DrawItem::kSelectionHighlight)) {
            stateToCommit._instanceTransforms.setLength(instanceCount);
            for (unsigned int i = 0; i < instanceCount; ++i) {
                transforms[i].Get(instanceMatrix.matrix);
                stateToCommit._instanceTransforms[i] = worldMatrix * instanceMatrix;
            }
        } else if (_selectionStatus == kFullyLead || _selectionStatus == kFullyActive) {
            const bool    lead = (_selectionStatus == kFullyLead);
            const MColor& color = drawScene.GetSelectionHighlightColor(lead);
            unsigned int  offset = 0;

            stateToCommit._instanceTransforms.setLength(instanceCount);
            stateToCommit._instanceColors.setLength(instanceCount * kNumColorChannels);

            for (unsigned int i = 0; i < instanceCount; ++i) {
                transforms[i].Get(instanceMatrix.matrix);
                stateToCommit._instanceTransforms[i] = worldMatrix * instanceMatrix;

                for (unsigned int j = 0; j < kNumColorChannels; j++) {
                    stateToCommit._instanceColors[offset++] = color[j];
                }
            }
        } else {
            const MColor colors[] = { drawScene.GetWireframeColor(),
                                      drawScene.GetSelectionHighlightColor(false),
                                      drawScene.GetSelectionHighlightColor(true) };

            // Store the indices to colors.
            std::vector<unsigned char> colorIndices;

            // Assign with the index to the dormant wireframe color by default.
            colorIndices.resize(instanceCount, 0);

            // Assign with the index to the active selection highlight color.
            if (const auto state = drawScene.GetActiveSelectionState(id)) {
                for (const auto& indexArray : state->instanceIndices) {
                    for (const auto index : indexArray) {
                        colorIndices[index] = 1;
                    }
                }
            }

            // Assign with the index to the lead selection highlight color.
            if (const auto state = drawScene.GetLeadSelectionState(id)) {
                for (const auto& indexArray : state->instanceIndices) {
                    for (const auto index : indexArray) {
                        colorIndices[index] = 2;
                    }
                }
            }

            // Fill per-instance colors. Skip unselected instances for the dedicated selection
            // highlight item.
            for (unsigned int i = 0; i < instanceCount; i++) {
                unsigned char colorIndex = colorIndices[i];
                if (isDedicatedSelectionHighlightItem && colorIndex == 0)
                    continue;

                transforms[i].Get(instanceMatrix.matrix);
                stateToCommit._instanceTransforms.append(worldMatrix * instanceMatrix);

                const MColor& color = colors[colorIndex];
                for (unsigned int j = 0; j < kNumColorChannels; j++) {
                    stateToCommit._instanceColors.append(color[j]);
                }
            }
        }
    } else {
        // Non-instanced Rprims.
        if ((itemDirtyBits & DirtySelectionHighlight)
            && drawItem->ContainsUsage(HdVP2DrawItem::kSelectionHighlight)) {
            const MColor& color
                = (_selectionStatus != kUnselected
                       ? drawScene.GetSelectionHighlightColor(_selectionStatus == kFullyLead)
                       : drawScene.GetWireframeColor());

            MHWRender::MShaderInstance* shader = _delegate->Get3dSolidShader(color);
            if (shader != nullptr && shader != drawItemData._shader) {
                drawItemData._shader = shader;
                stateToCommit._shader = shader;
                stateToCommit._isTransparent = false;
            }
        }
    }

    // Determine if the render item should be enabled or not.
    if (!GetInstancerId().IsEmpty()
        || (itemDirtyBits
            & (HdChangeTracker::DirtyVisibility | HdChangeTracker::DirtyRenderTag
               | HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent
               | DirtySelectionHighlight))) {
        bool enable = drawItem->GetVisible() && !_meshSharedData._points.empty()
            && !instancerWithNoInstances;

        if (isDedicatedSelectionHighlightItem) {
            enable = enable && (_selectionStatus != kUnselected);
        } else if (isPointSnappingItem) {
            enable = enable && (_selectionStatus == kUnselected);
        } else if (isBBoxItem) {
            enable = enable && !range.IsEmpty();
        }

        enable = enable && drawScene.DrawRenderTag(_meshSharedData._renderTag);

        if (drawItemData._enabled != enable) {
            drawItemData._enabled = enable;
            stateToCommit._enabled = &drawItemData._enabled;
        }
    }

    stateToCommit._geometryDirty
        = (itemDirtyBits
           & (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyNormals
              | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyTopology));

    // Reset dirty bits because we've prepared commit state for this draw item.
    drawItem->ResetDirtyBits();

    // Capture the valid position buffer and index buffer
    MHWRender::MVertexBuffer* positionsBuffer = _meshSharedData._positionsBuffer.get();
    MHWRender::MIndexBuffer*  indexBuffer = drawItemData._indexBuffer.get();

    if (isBBoxItem) {
        const HdVP2BBoxGeom& sharedBBoxGeom = _delegate->GetSharedBBoxGeom();
        positionsBuffer = const_cast<MHWRender::MVertexBuffer*>(sharedBBoxGeom.GetPositionBuffer());
        indexBuffer = const_cast<MHWRender::MIndexBuffer*>(sharedBBoxGeom.GetIndexBuffer());
    }

    _delegate->GetVP2ResourceRegistry().EnqueueCommit([drawItem,
                                                       stateToCommit,
                                                       param,
                                                       positionsBuffer,
                                                       indexBuffer]() {
        MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
        if (ARCH_UNLIKELY(!renderItem))
            return;

        MProfilingScope profilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2,
            drawItem->GetRenderItemName().asChar(),
            "Commit");

        const HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._drawItemData;

        MHWRender::MVertexBuffer* colorBuffer = drawItemData._colorBuffer.get();
        MHWRender::MVertexBuffer* normalsBuffer = drawItemData._normalsBuffer.get();

        const HdVP2DrawItem::PrimvarBufferMap& primvarBuffers = drawItemData._primvarBuffers;

        // If available, something changed
        if (stateToCommit._colorBufferData)
            colorBuffer->commit(stateToCommit._colorBufferData);

        // If available, something changed
        if (stateToCommit._normalsBufferData)
            normalsBuffer->commit(stateToCommit._normalsBufferData);

        // If available, something changed
        for (const auto& entry : stateToCommit._primvarBufferDataMap) {
            const TfToken& primvarName = entry.first;
            void*          primvarBufferData = entry.second;
            if (primvarBufferData) {
                const auto it = primvarBuffers.find(primvarName);
                if (it != primvarBuffers.end()) {
                    MHWRender::MVertexBuffer* primvarBuffer = it->second.get();
                    if (primvarBuffer) {
                        primvarBuffer->commit(primvarBufferData);
                    }
                }
            }
        }

        // If available, something changed
        if (stateToCommit._indexBufferData)
            indexBuffer->commit(stateToCommit._indexBufferData);

        // If available, something changed
        if (stateToCommit._shader != nullptr) {
            renderItem->setShader(stateToCommit._shader);
            renderItem->setTreatAsTransparent(stateToCommit._isTransparent);
        }

        // If the enable state is changed, then update it.
        if (stateToCommit._enabled != nullptr) {
            renderItem->enable(*stateToCommit._enabled);
        }

        ProxyRenderDelegate& drawScene = param->GetDrawScene();

        if (stateToCommit._geometryDirty || stateToCommit._boundingBox) {
            MHWRender::MVertexBufferArray vertexBuffers;
            vertexBuffers.addBuffer(kPositionsStr, positionsBuffer);

            if (colorBuffer)
                vertexBuffers.addBuffer(kDiffuseColorStr, colorBuffer);

            if (normalsBuffer)
                vertexBuffers.addBuffer(kNormalsStr, normalsBuffer);

            for (auto& entry : primvarBuffers) {
                const TfToken&            primvarName = entry.first;
                MHWRender::MVertexBuffer* primvarBuffer = entry.second.get();
                if (primvarBuffer) {
                    vertexBuffers.addBuffer(primvarName.GetText(), primvarBuffer);
                }
            }

            // The API call does three things:
            // - Associate geometric buffers with the render item.
            // - Update bounding box.
            // - Trigger consolidation/instancing update.
            drawScene.setGeometryForRenderItem(
                *renderItem, vertexBuffers, *indexBuffer, stateToCommit._boundingBox);
        }

        // Important, update instance transforms after setting geometry on render items!
        auto& oldInstanceCount = stateToCommit._drawItemData._instanceCount;
        auto  newInstanceCount = stateToCommit._instanceTransforms.length();

        // GPU instancing has been enabled. We cannot switch to consolidation
        // without recreating render item, so we keep using GPU instancing.
        if (stateToCommit._drawItemData._usingInstancedDraw) {
            if (oldInstanceCount == newInstanceCount) {
                for (unsigned int i = 0; i < newInstanceCount; i++) {
                    // VP2 defines instance ID of the first instance to be 1.
                    drawScene.updateInstanceTransform(
                        *renderItem, i + 1, stateToCommit._instanceTransforms[i]);
                }
            } else {
                drawScene.setInstanceTransformArray(*renderItem, stateToCommit._instanceTransforms);
            }

            if (stateToCommit._instanceColors.length() == newInstanceCount * kNumColorChannels) {
                drawScene.setExtraInstanceData(
                    *renderItem, kSolidColorStr, stateToCommit._instanceColors);
            }
        }
#if MAYA_API_VERSION >= 20210000
        else if (newInstanceCount >= 1) {
#else
        // In Maya 2020 and before, GPU instancing and consolidation are two separate systems that
        // cannot be used by a render item at the same time. In case of single instance, we keep
        // the original render item to allow consolidation with other prims. In case of multiple
        // instances, we need to disable consolidation to allow GPU instancing to be used.
        else if (newInstanceCount == 1) {
            renderItem->setMatrix(&stateToCommit._instanceTransforms[0]);
        } else if (newInstanceCount > 1) {
            setWantConsolidation(*renderItem, false);
#endif
            drawScene.setInstanceTransformArray(*renderItem, stateToCommit._instanceTransforms);

            if (stateToCommit._instanceColors.length() == newInstanceCount * kNumColorChannels) {
                drawScene.setExtraInstanceData(
                    *renderItem, kSolidColorStr, stateToCommit._instanceColors);
            }

            stateToCommit._drawItemData._usingInstancedDraw = true;
        } else if (stateToCommit._worldMatrix != nullptr) {
            // Regular non-instanced prims. Consolidation has been turned on by
            // default and will be kept enabled on this case.
            renderItem->setMatrix(stateToCommit._worldMatrix);
        }

        oldInstanceCount = newInstanceCount;
    });
}

void HdVP2Mesh::_HideAllDrawItems(const TfToken& reprToken)
{
    HdReprSharedPtr const& curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    _MeshReprConfig::DescArray reprDescs = _GetReprDesc(reprToken);

    // For each relevant draw item, update dirty buffer sources.
    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc& desc = reprDescs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleInvalid) {
            continue;
        }

        auto* drawItem = static_cast<HdVP2DrawItem*>(curRepr->GetDrawItem(drawItemIndex++));
        if (!drawItem)
            continue;
        MHWRender::MRenderItem* renderItem = drawItem->GetRenderItem();
        if (!renderItem)
            continue;

        drawItem->GetRenderItemData()._enabled = false;

        _delegate->GetVP2ResourceRegistry().EnqueueCommit(
            [renderItem]() { renderItem->enable(false); });
    }
}

/*! \brief  Update _primvarSourceMap, our local cache of raw primvar data.

    This function pulls data from the scene delegate, but defers processing.

    While iterating primvars, we skip "points" (vertex positions) because
    the points primvar is processed separately for direct access later. We
    only call GetPrimvar on primvars that have been marked dirty.
*/
void HdVP2Mesh::_UpdatePrimvarSources(
    HdSceneDelegate*     sceneDelegate,
    HdDirtyBits          dirtyBits,
    const TfTokenVector& requiredPrimvars)
{
    const SdfPath& id = GetId();

    TfTokenVector::const_iterator begin = requiredPrimvars.cbegin();
    TfTokenVector::const_iterator end = requiredPrimvars.cend();

    for (size_t i = 0; i < HdInterpolationCount; i++) {
        const HdInterpolation           interp = static_cast<HdInterpolation>(i);
        const HdPrimvarDescriptorVector primvars = GetPrimvarDescriptors(sceneDelegate, interp);

        for (const HdPrimvarDescriptor& pv : primvars) {
            if (std::find(begin, end, pv.name) != end) {
                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                    const VtValue value = GetPrimvar(sceneDelegate, pv.name);
                    _meshSharedData._primvarSourceMap[pv.name] = { value, interp };
                }
            } else {
                _meshSharedData._primvarSourceMap.erase(pv.name);
            }
        }
    }
}

/*! \brief  Create render item for points repr.
 */
MHWRender::MRenderItem* HdVP2Mesh::_CreatePointsRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kPoints);

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
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kLines);

    renderItem->setDrawMode(MHWRender::MGeometry::kWireframe);
    renderItem->depthPriority(MHWRender::MRenderItem::sDormantWireDepthPriority);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(kOpaqueBlue));
    renderItem->setSelectionMask(MSelectionMask::kSelectMeshes);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for bbox repr.
 */
MHWRender::MRenderItem* HdVP2Mesh::_CreateBoundingBoxRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kLines);

    renderItem->setDrawMode(MHWRender::MGeometry::kBoundingBox);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(kOpaqueBlue));
    renderItem->setSelectionMask(MSelectionMask::kSelectMeshes);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item for smoothHull repr.
 */
MHWRender::MRenderItem* HdVP2Mesh::_CreateSmoothHullRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::MaterialSceneItem, MHWRender::MGeometry::kTriangles);

    constexpr MHWRender::MGeometry::DrawMode drawMode = static_cast<MHWRender::MGeometry::DrawMode>(
        MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured);
    renderItem->setDrawMode(drawMode);
    renderItem->setExcludedFromPostEffects(false);
    renderItem->castsShadows(true);
    renderItem->receivesShadows(true);
    renderItem->setShader(_delegate->GetFallbackShader(kOpaqueGray));
    renderItem->setSelectionMask(MSelectionMask::kSelectMeshes);

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

/*! \brief  Create render item to support selection highlight for smoothHull repr.
 */
MHWRender::MRenderItem* HdVP2Mesh::_CreateSelectionHighlightRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kLines);

    constexpr MHWRender::MGeometry::DrawMode drawMode = static_cast<MHWRender::MGeometry::DrawMode>(
        MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured);
    renderItem->setDrawMode(drawMode);
    renderItem->depthPriority(MHWRender::MRenderItem::sActiveWireDepthPriority);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dSolidShader(kOpaqueBlue));
    renderItem->setSelectionMask(MSelectionMask());

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

PXR_NAMESPACE_CLOSE_SCOPE
