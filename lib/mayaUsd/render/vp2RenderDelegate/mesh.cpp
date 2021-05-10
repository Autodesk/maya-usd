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
#include "instancer.h"
#include "material.h"
#include "render_delegate.h"
#include "tokens.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>
#include <mayaUsd/utils/colorSpace.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/imaging/hd/meshUtil.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/smoothNormals.h>
#include <pxr/imaging/hd/version.h>
#include <pxr/imaging/hd/vertexAdjacency.h>
#include <pxr/pxr.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MFrameContext.h>
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
    HdVP2DrawItem::RenderItemData& _renderItemData;

    //! If valid, new index buffer data to commit
    int* _indexBufferData { nullptr };
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
    CommitState(HdVP2DrawItem::RenderItemData& renderItemData)
        : _renderItemData(renderItemData)
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
bool _IsUnsharedVertexLayoutRequired(const PrimvarInfoMap& primvarInfo)
{
    for (const auto& it : primvarInfo) {
        const HdInterpolation interp = it.second->_source.interpolation;
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

PrimvarInfo* _getInfo(const PrimvarInfoMap& infoMap, const TfToken& token)
{
    auto it = infoMap.find(token);
    if (it != infoMap.end()) {
        return it->second.get();
    }
    return nullptr;
}

void _getColorData(
    PrimvarInfoMap&  infoMap,
    VtVec3fArray&    colorArray,
    HdInterpolation& interpolation)
{
    PrimvarInfo* info = _getInfo(infoMap, HdTokens->displayColor);
    if (info) {
        const VtValue& value = info->_source.data;
        if (value.IsHolding<VtVec3fArray>() && value.GetArraySize() > 0) {
            colorArray = value.UncheckedGet<VtVec3fArray>();
            interpolation = info->_source.interpolation;
        }
    }

    if (colorArray.empty()) {
        // If color/opacity is not found, the 18% gray color will be used
        // to match the default color of Hydra Storm.
        colorArray.push_back(GfVec3f(0.18f, 0.18f, 0.18f));
        interpolation = HdInterpolationConstant;

        infoMap[HdTokens->displayColor] = std::make_unique<PrimvarInfo>(
            PrimvarSource(VtValue(colorArray), interpolation, PrimvarSource::CPUCompute), nullptr);
    }
}

void _getOpacityData(
    PrimvarInfoMap&  infoMap,
    VtFloatArray&    opacityArray,
    HdInterpolation& interpolation)
{
    PrimvarInfo* info = _getInfo(infoMap, HdTokens->displayOpacity);
    if (info) {
        const VtValue& value = info->_source.data;
        if (value.IsHolding<VtFloatArray>() && value.GetArraySize() > 0) {
            opacityArray = value.UncheckedGet<VtFloatArray>();
            interpolation = info->_source.interpolation;
        }
    }

    if (opacityArray.empty()) {
        opacityArray.push_back(1.0f);
        interpolation = HdInterpolationConstant;

        infoMap[HdTokens->displayOpacity] = std::make_unique<PrimvarInfo>(
            PrimvarSource(VtValue(opacityArray), interpolation, PrimvarSource::CPUCompute),
            nullptr);
    }
}

//! Access the points
VtVec3fArray _points(PrimvarInfoMap& infoMap)
{
    if (PrimvarInfo* info = _getInfo(infoMap, HdTokens->points)) {
        VtValue data = info->_source.data;
        TF_VERIFY(data.IsHolding<VtVec3fArray>());
        return data.UncheckedGet<VtVec3fArray>();
    }

    return VtVec3fArray();
}

} // namespace

void HdVP2Mesh::_InitGPUCompute()
{
    // check that the viewport is using OpenGL, we need it for the OpenGL normals computation
    MRenderer* renderer = MRenderer::theRenderer();
    // would also be nice to check the openGL version but renderer->drawAPIVersion() returns 4.
    // Compute was added in 4.3 so I don't have enough information to make the check
    if (renderer && renderer->drawAPIIsOpenGL()
        && (TfGetenvInt("HDVP2_USE_GPU_NORMAL_COMPUTATION", 0) > 0)) {
        int threshold = TfGetenvInt("HDVP2_GPU_NORMAL_COMPUTATION_MINIMUM_THRESHOLD", 8000);
        _gpuNormalsComputeThreshold = threshold >= 0 ? (size_t)threshold : SIZE_MAX;
    } else
        _gpuNormalsComputeThreshold = SIZE_MAX;
}

size_t HdVP2Mesh::_gpuNormalsComputeThreshold = SIZE_MAX;
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
    _meshSharedData = std::make_shared<HdVP2MeshSharedData>();
    // HdChangeTracker::IsVarying() can check dirty bits to tell us if an object is animated or not.
    // Not sure if it is correct on file load
#ifdef HDVP2_ENABLE_GPU_COMPUTE
    static std::once_flag initGPUComputeOnce;
    std::call_once(initGPUComputeOnce, _InitGPUCompute);
#endif
}

void HdVP2Mesh::_CommitMVertexBuffer(MHWRender::MVertexBuffer* const buffer, void* bufferData) const
{
    const MString& rprimId = _rprimId;

    _delegate->GetVP2ResourceRegistry().EnqueueCommit([buffer, bufferData, rprimId]() {
        MProfilingScope profilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2,
            "CommitBuffer",
            rprimId.asChar()); // TODO: buffer usage so we know it is positions normals etc

        buffer->commit(bufferData);
    });
}

void HdVP2Mesh::_PrepareSharedVertexBuffers(
    HdSceneDelegate*   delegate,
    const HdDirtyBits& rprimDirtyBits,
    const TfToken&     reprToken)
{
    // Normals have two possible sources. They could be authored by the scene delegate,
    // in which case we should find them in _primvarInfo, or they could be computed
    // normals. Compute the normal buffer if necessary.
    PrimvarInfo* normalsInfo = _getInfo(_meshSharedData->_primvarInfo, HdTokens->normals);
    bool         needNormals = _PrimvarIsRequired(HdTokens->normals);
    bool         computeCPUNormals = (!normalsInfo && !_gpuNormalsEnabled)
        || (normalsInfo && PrimvarSource::CPUCompute == normalsInfo->_source.dataSource);
    bool computeGPUNormals = (!normalsInfo && _gpuNormalsEnabled)
        || (normalsInfo && PrimvarSource::GPUCompute == normalsInfo->_source.dataSource);
    bool hasCleanNormals
        = normalsInfo && (0 == (rprimDirtyBits & (DirtySmoothNormals | DirtyFlatNormals)));
    if (needNormals && (computeCPUNormals || computeGPUNormals) && !hasCleanNormals) {
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

        // If there are authored normals, prepare buffer only when it is dirty.
        // otherwise, compute smooth normals from points and adjacency and we
        // have a custom dirty bit to determine whether update is needed.
        if (requireSmoothNormals && (rprimDirtyBits & DirtySmoothNormals)) {
            if (computeGPUNormals) {
#ifdef HDVP2_ENABLE_GPU_COMPUTE
                _meshSharedData->_viewportCompute->setNormalVertexBufferGPUDirty();
#endif
            }
            if (computeCPUNormals) {
                // note: normals gets dirty when points are marked as dirty,
                // at change tracker.
                Hd_VertexAdjacencySharedPtr adjacency(new Hd_VertexAdjacency());
                HdBufferSourceSharedPtr     adjacencyComputation
                    = adjacency->GetSharedAdjacencyBuilderComputation(&_meshSharedData->_topology);
                adjacencyComputation->Resolve();

                // Only the points referenced by the topology are used to compute
                // smooth normals.
                VtValue normals(Hd_SmoothNormals::ComputeSmoothNormals(
                    adjacency.get(),
                    _points(_meshSharedData->_primvarInfo).size(),
                    _points(_meshSharedData->_primvarInfo).cdata()));

                if (!normalsInfo) {
                    _meshSharedData->_primvarInfo[HdTokens->normals]
                        = std::make_unique<PrimvarInfo>(
                            PrimvarSource(
                                normals, HdInterpolationVertex, PrimvarSource::CPUCompute),
                            nullptr);
                } else {
                    normalsInfo->_source.data = normals;
                    normalsInfo->_source.interpolation = HdInterpolationVertex;
                }
            }
        }

        if (requireFlatNormals && (rprimDirtyBits & DirtyFlatNormals)) {
            // TODO:
        }
    }

    // Prepare color buffer.
    if (((rprimDirtyBits
          & (HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyInstancer
             | HdChangeTracker::DirtyInstanceIndex))
         != 0)
        && (_PrimvarIsRequired(HdTokens->displayColor)
            || _PrimvarIsRequired(HdTokens->displayOpacity))) {
        HdInterpolation colorInterp = HdInterpolationConstant;
        HdInterpolation alphaInterp = HdInterpolationConstant;
        VtVec3fArray    colorArray;
        VtFloatArray    alphaArray;

        _getColorData(_meshSharedData->_primvarInfo, colorArray, colorInterp);
        _getOpacityData(_meshSharedData->_primvarInfo, alphaArray, alphaInterp);

        PrimvarInfo* colorAndOpacityInfo
            = _getInfo(_meshSharedData->_primvarInfo, HdVP2Tokens->displayColorAndOpacity);
        if (!colorAndOpacityInfo) {
            _meshSharedData->_primvarInfo[HdVP2Tokens->displayColorAndOpacity]
                = std::make_unique<PrimvarInfo>(
                    PrimvarSource(VtValue(), HdInterpolationConstant, PrimvarSource::CPUCompute),
                    nullptr);
            colorAndOpacityInfo
                = _getInfo(_meshSharedData->_primvarInfo, HdVP2Tokens->displayColorAndOpacity);
        }

        if (colorInterp == HdInterpolationInstance || alphaInterp == HdInterpolationInstance) {
            TF_VERIFY(!GetInstancerId().IsEmpty());
            VtIntArray instanceIndices = delegate->GetInstanceIndices(GetInstancerId(), GetId());
            size_t     numInstances = instanceIndices.size();
            colorAndOpacityInfo->_extraInstanceData.setLength(
                numInstances * kNumColorChannels); // the data is a vec4
            void* bufferData = &colorAndOpacityInfo->_extraInstanceData[0];

            size_t alphaChannelOffset = 3;
            for (size_t instance = 0; instance < numInstances; instance++) {
                int      index = instanceIndices[instance];
                GfVec3f* color = reinterpret_cast<GfVec3f*>(
                    reinterpret_cast<float*>(&static_cast<GfVec4f*>(bufferData)[instance]));
                float* alpha
                    = reinterpret_cast<float*>(&static_cast<GfVec4f*>(bufferData)[instance])
                    + alphaChannelOffset;

                if (colorInterp == HdInterpolationInstance) {
                    *color = colorArray[index];
                } else if (colorInterp == HdInterpolationConstant) {
                    *color = colorArray[0];
                } else {
                    TF_WARN("Unsupported combination of display color interpolation and display "
                            "opacity interpolation instance.");
                }

                if (alphaInterp == HdInterpolationInstance) {
                    *alpha = alphaArray[index];
                } else if (alphaInterp == HdInterpolationConstant) {
                    *alpha = alphaArray[0];
                } else {
                    TF_WARN("Unsupported combination of display color interpolation instance and "
                            "display opacity interpolation.");
                }
            }
        } else {
            if (!colorAndOpacityInfo->_buffer) {
                const MHWRender::MVertexBufferDescriptor vbDesc(
                    "", MHWRender::MGeometry::kColor, MHWRender::MGeometry::kFloat, 4);

                colorAndOpacityInfo->_buffer.reset(new MHWRender::MVertexBuffer(vbDesc));
            }

            void* bufferData = _meshSharedData->_numVertices > 0
                ? colorAndOpacityInfo->_buffer->acquire(_meshSharedData->_numVertices, true)
                : nullptr;

            // Fill color and opacity into the float4 color stream.
            if (bufferData) {
                _FillPrimvarData(
                    static_cast<GfVec4f*>(bufferData),
                    _meshSharedData->_numVertices,
                    0,
                    _meshSharedData->_renderingToSceneFaceVtxIds,
                    _rprimId,
                    _meshSharedData->_topology,
                    HdTokens->displayColor,
                    colorArray,
                    colorInterp);

                _FillPrimvarData(
                    static_cast<GfVec4f*>(bufferData),
                    _meshSharedData->_numVertices,
                    3,
                    _meshSharedData->_renderingToSceneFaceVtxIds,
                    _rprimId,
                    _meshSharedData->_topology,
                    HdTokens->displayOpacity,
                    alphaArray,
                    alphaInterp);

                _CommitMVertexBuffer(colorAndOpacityInfo->_buffer.get(), bufferData);
            }
        }
    }

    // prepare the other primvar buffers
    // Prepare primvar buffers.
    if (rprimDirtyBits
        & (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyNormals
           | HdChangeTracker::DirtyPrimvar)) {
        for (const auto& it : _meshSharedData->_primvarInfo) {
            const TfToken& token = it.first;
            // Color, opacity have been prepared separately.
            if ((token == HdTokens->displayColor) || (token == HdTokens->displayOpacity)
                || (token == HdVP2Tokens->displayColorAndOpacity))
                continue;

            MHWRender::MGeometry::Semantic semantic = MHWRender::MGeometry::kTexture;
            if (token == HdTokens->points) {
                if ((rprimDirtyBits & HdChangeTracker::DirtyPoints) == 0)
                    continue;
                semantic = MHWRender::MGeometry::kPosition;
            } else if (token == HdTokens->normals) {
                if ((rprimDirtyBits & (HdChangeTracker::DirtyNormals | DirtySmoothNormals)) == 0)
                    continue;
                semantic = MHWRender::MGeometry::kNormal;
            } else if ((rprimDirtyBits & HdChangeTracker::DirtyPrimvar) == 0) {
                continue;
            }

            const VtValue&         value = it.second->_source.data;
            const HdInterpolation& interp = it.second->_source.interpolation;

            if (!value.IsArrayValued() || value.GetArraySize() == 0)
                continue;

            MHWRender::MVertexBuffer* buffer = _meshSharedData->_primvarInfo[token]->_buffer.get();

            void* bufferData = nullptr;

            if (value.IsHolding<VtFloatArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", semantic, MHWRender::MGeometry::kFloat, 1);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    _meshSharedData->_primvarInfo[token]->_buffer.reset(buffer);
                }

                if (buffer) {
                    bufferData = _meshSharedData->_numVertices > 0
                        ? buffer->acquire(_meshSharedData->_numVertices, true)
                        : nullptr;
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<float*>(bufferData),
                            _meshSharedData->_numVertices,
                            0,
                            _meshSharedData->_renderingToSceneFaceVtxIds,
                            _rprimId,
                            _meshSharedData->_topology,
                            token,
                            value.UncheckedGet<VtFloatArray>(),
                            interp);
                    }
                }
            } else if (value.IsHolding<VtVec2fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", semantic, MHWRender::MGeometry::kFloat, 2);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    _meshSharedData->_primvarInfo[token]->_buffer.reset(buffer);
                }

                if (buffer) {
                    bufferData = _meshSharedData->_numVertices > 0
                        ? buffer->acquire(_meshSharedData->_numVertices, true)
                        : nullptr;
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<GfVec2f*>(bufferData),
                            _meshSharedData->_numVertices,
                            0,
                            _meshSharedData->_renderingToSceneFaceVtxIds,
                            _rprimId,
                            _meshSharedData->_topology,
                            token,
                            value.UncheckedGet<VtVec2fArray>(),
                            interp);
                    }
                }
            } else if (value.IsHolding<VtVec3fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", semantic, MHWRender::MGeometry::kFloat, 3);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    _meshSharedData->_primvarInfo[token]->_buffer.reset(buffer);
                }

                if (buffer) {
                    bufferData = _meshSharedData->_numVertices > 0
                        ? buffer->acquire(_meshSharedData->_numVertices, true)
                        : nullptr;
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<GfVec3f*>(bufferData),
                            _meshSharedData->_numVertices,
                            0,
                            _meshSharedData->_renderingToSceneFaceVtxIds,
                            _rprimId,
                            _meshSharedData->_topology,
                            token,
                            value.UncheckedGet<VtVec3fArray>(),
                            interp);
                    }
                }
            } else if (value.IsHolding<VtVec4fArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", semantic, MHWRender::MGeometry::kFloat, 4);

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    _meshSharedData->_primvarInfo[token]->_buffer.reset(buffer);
                }

                if (buffer) {
                    bufferData = _meshSharedData->_numVertices > 0
                        ? buffer->acquire(_meshSharedData->_numVertices, true)
                        : nullptr;
                    if (bufferData) {
                        _FillPrimvarData(
                            static_cast<GfVec4f*>(bufferData),
                            _meshSharedData->_numVertices,
                            0,
                            _meshSharedData->_renderingToSceneFaceVtxIds,
                            _rprimId,
                            _meshSharedData->_topology,
                            token,
                            value.UncheckedGet<VtVec4fArray>(),
                            interp);
                    }
                }
            } else {
                TF_WARN("Unsupported primvar array");
            }

            _CommitMVertexBuffer(buffer, bufferData);
        }
    }
}

bool HdVP2Mesh::_PrimvarIsRequired(const TfToken& primvar) const
{
    const TfTokenVector& allRequiredPrimvars = _meshSharedData->_allRequiredPrimvars;

    TfTokenVector::const_iterator begin = allRequiredPrimvars.cbegin();
    TfTokenVector::const_iterator end = allRequiredPrimvars.cend();

    return (std::find(begin, end, primvar) != end);
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

    const SdfPath& id = GetId();

    // We don't update the repr if it is hidden by the render tags (purpose)
    // of the ProxyRenderDelegate. In additional, we need to hide any already
    // existing render items because they should not be drawn.
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    HdRenderIndex&       renderIndex = delegate->GetRenderIndex();
    if (!drawScene.DrawRenderTag(renderIndex.GetRenderTag(id))) {
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

    // Geom subsets are accessed through the mesh topology. I need to know about
    // the additional materialIds that get bound by geom subsets before we build the
    // _primvaInfo. So the very first thing I need to do is grab the topology.
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        // unsubscribe from material updates from the old geom subset materials
#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
        for (const auto& geomSubset : _meshSharedData->_topology.GetGeomSubsets()) {
            if (!geomSubset.materialId.IsEmpty()) {
                const SdfPath materialId
                    = dynamic_cast<UsdImagingDelegate*>(delegate)->ConvertCachePathToIndexPath(
                        geomSubset.materialId);
                HdVP2Material* material = static_cast<HdVP2Material*>(
                    renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

                if (material) {
                    material->UnsubscribeFromMaterialUpdates(id);
                }
            }
        }
#endif

        _meshSharedData->_topology = GetMeshTopology(delegate);

        // subscribe to material updates from the new geom subset materials
#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
        for (const auto& geomSubset : _meshSharedData->_topology.GetGeomSubsets()) {
            if (!geomSubset.materialId.IsEmpty()) {
                const SdfPath materialId
                    = dynamic_cast<UsdImagingDelegate*>(delegate)->ConvertCachePathToIndexPath(
                        geomSubset.materialId);
                HdVP2Material* material = static_cast<HdVP2Material*>(
                    renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

                if (material) {
                    material->SubscribeForMaterialUpdates(id);
                }
            }
        }
#endif
    }

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        const SdfPath materialId = delegate->GetMaterialId(id);

#ifdef HDVP2_MATERIAL_CONSOLIDATION_UPDATE_WORKAROUND
        const SdfPath& origMaterialId = GetMaterialId();
        if (materialId != origMaterialId) {
            if (!origMaterialId.IsEmpty()) {
                HdVP2Material* material = static_cast<HdVP2Material*>(
                    renderIndex.GetSprim(HdPrimTypeTokens->material, origMaterialId));
                if (material) {
                    material->UnsubscribeFromMaterialUpdates(id);
                }
            }

            if (!materialId.IsEmpty()) {
                HdVP2Material* material = static_cast<HdVP2Material*>(
                    renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
                if (material) {
                    material->SubscribeForMaterialUpdates(id);
                }
            }
        }
#endif

#if HD_API_VERSION < 37
        _SetMaterialId(renderIndex.GetChangeTracker(), materialId);
#else
        SetMaterialId(materialId);
#endif
    }

#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    // Update our instance topology if necessary.
    _UpdateInstancer(delegate, dirtyBits);
#endif

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)
        || HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals)
        || HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar)) {

        auto addRequiredPrimvars = [&](const SdfPath& materialId) {
            const HdVP2Material* material = static_cast<const HdVP2Material*>(
                renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));
            const TfTokenVector& requiredPrimvars = material && material->GetSurfaceShader()
                ? material->GetRequiredPrimvars()
                : sFallbackShaderPrimvars;

            for (const auto& requiredPrimvar : requiredPrimvars) {
                if (!_PrimvarIsRequired(requiredPrimvar)) {
                    _meshSharedData->_allRequiredPrimvars.push_back(requiredPrimvar);
                }
            }
        };

        // there is a chance that the geom subsets cover all the faces of the
        // mesh and that the overall material id is unused. I don't figure that
        // out until much later, so for now just accept that we might pull unnecessary
        // primvars required by the overall material but not by any of the geom subset
        // materials.
        addRequiredPrimvars(GetMaterialId());

        for (const auto& geomSubset : _meshSharedData->_topology.GetGeomSubsets()) {
            addRequiredPrimvars(
                dynamic_cast<UsdImagingDelegate*>(delegate)->ConvertCachePathToIndexPath(
                    geomSubset.materialId));
        }

        // also, we always require points
        if (!_PrimvarIsRequired(HdTokens->points))
            _meshSharedData->_allRequiredPrimvars.push_back(HdTokens->points);

        _UpdatePrimvarSources(delegate, *dirtyBits, _meshSharedData->_allRequiredPrimvars);
    }

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        const HdMeshTopology& topology = _meshSharedData->_topology;
        const VtIntArray&     faceVertexIndices = topology.GetFaceVertexIndices();
        const size_t          numFaceVertexIndices = faceVertexIndices.size();

        VtIntArray newFaceVertexIndices;
        newFaceVertexIndices.resize(numFaceVertexIndices);

        if (_IsUnsharedVertexLayoutRequired(_meshSharedData->_primvarInfo)) {
            _meshSharedData->_numVertices = numFaceVertexIndices;
            _meshSharedData->_renderingToSceneFaceVtxIds = faceVertexIndices;
            _meshSharedData->_sceneToRenderingFaceVtxIds.clear();
            _meshSharedData->_sceneToRenderingFaceVtxIds.resize(topology.GetNumPoints(), -1);

            for (size_t i = 0; i < numFaceVertexIndices; i++) {
                const int sceneFaceVtxId = faceVertexIndices[i];
                _meshSharedData->_sceneToRenderingFaceVtxIds[sceneFaceVtxId]
                    = i; // could check if the existing value is -1, but it doesn't matter.
                         // we just need to map to a vertex in the position buffer that has
                         // the correct value.
            }

            // Fill with sequentially increasing values, starting from 0. The new
            // face vertex indices will be used to populate index data for unshared
            // vertex layout. Note that _FillPrimvarData assumes this sequence to
            // be used for face-varying primvars and saves lookup and remapping
            // with _renderingToSceneFaceVtxIds, so in case we change the array we
            // should update _FillPrimvarData() code to remap indices correctly.
            std::iota(newFaceVertexIndices.begin(), newFaceVertexIndices.end(), 0);
        } else {
            _meshSharedData->_numVertices = topology.GetNumPoints();
            _meshSharedData->_renderingToSceneFaceVtxIds.clear();

            // Allocate large enough memory with initial value of -1 to indicate
            // the rendering face vertex index is not determined yet.
            _meshSharedData->_sceneToRenderingFaceVtxIds.clear();
            _meshSharedData->_sceneToRenderingFaceVtxIds.resize(numFaceVertexIndices, -1);
            unsigned int sceneToRenderingFaceVtxIdsCount = 0;

            // Sort vertices to avoid drastically jumping indices. Cache efficiency
            // is important to fast rendering performance for dense mesh.
            for (size_t i = 0; i < numFaceVertexIndices; i++) {
                const int sceneFaceVtxId = faceVertexIndices[i];

                int renderFaceVtxId = _meshSharedData->_sceneToRenderingFaceVtxIds[sceneFaceVtxId];
                if (renderFaceVtxId < 0) {
                    renderFaceVtxId = _meshSharedData->_renderingToSceneFaceVtxIds.size();
                    _meshSharedData->_renderingToSceneFaceVtxIds.push_back(sceneFaceVtxId);

                    _meshSharedData->_sceneToRenderingFaceVtxIds[sceneFaceVtxId] = renderFaceVtxId;
                    sceneToRenderingFaceVtxIdsCount++;
                }

                newFaceVertexIndices[i] = renderFaceVtxId;
            }

            _meshSharedData->_sceneToRenderingFaceVtxIds.resize(
                sceneToRenderingFaceVtxIdsCount); // drop any extra -1 values.
        }

        _meshSharedData->_renderingTopology = HdMeshTopology(
            topology.GetScheme(),
            topology.GetOrientation(),
            topology.GetFaceVertexCounts(),
            newFaceVertexIndices,
            topology.GetHoleIndices(),
            topology.GetRefineLevel());

        // All the render items to draw the shaded (Hull) style share the topology
        // calculation
        HdMeshUtil meshUtil(&_meshSharedData->_renderingTopology, GetId());
        _meshSharedData->_trianglesFaceVertexIndices.clear();
        _meshSharedData->_primitiveParam.clear();
        meshUtil.ComputeTriangleIndices(
            &_meshSharedData->_trianglesFaceVertexIndices,
            &_meshSharedData->_primitiveParam,
            nullptr);

        // Decide if we should use GPU compute, and set up compute objects for later user
#ifdef HDVP2_ENABLE_GPU_COMPUTE
        _gpuNormalsEnabled
            = _gpuNormalsEnabled && _meshSharedData->_numVertices >= _gpuNormalsComputeThreshold;
        if (_gpuNormalsEnabled) {
            _CreateViewportCompute();
#ifdef HDVP2_ENABLE_GPU_OSD
            _CreateOSDTables();
#endif
        }
#else
        _gpuNormalsEnabled = false;
#endif
    }

    _PrepareSharedVertexBuffers(delegate, *dirtyBits, reprToken);

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
        _meshSharedData->_renderTag = delegate->GetRenderTag(id);
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
        | HdChangeTracker::DirtyInstanceIndex | HdChangeTracker::DirtyRenderTag
        | DirtySelectionHighlight;

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

    // Visibility and selection result in highlight changes:
    if ((bits & HdChangeTracker::DirtyVisibility) && (bits & DirtySelection)) {
        bits |= DirtySelectionHighlight;
    }

    if (bits & HdChangeTracker::AllDirty) {
        // RPrim is dirty, propagate dirty bits to all draw items.
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
    } else {
        // RPrim is clean, find out if any drawItem about to be shown is dirty:
        for (const std::pair<TfToken, HdReprSharedPtr>& pair : _reprs) {
            const HdReprSharedPtr& repr = pair.second;
            const auto&            items = repr->GetDrawItems();
#if HD_API_VERSION < 35
            for (const HdDrawItem* item : items) {
                if (const HdVP2DrawItem* drawItem = static_cast<const HdVP2DrawItem*>(item)) {
#else
            for (const HdRepr::DrawItemUniquePtr& item : items) {
                if (const HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get())) {
#endif
                    // Is this Repr dirty and in need of a Sync?
                    if (drawItem->GetDirtyBits() & HdChangeTracker::DirtyRepr) {
                        bits |= (drawItem->GetDirtyBits() & ~HdChangeTracker::DirtyRepr);
                    }
                }
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
    _ReprVector::const_iterator it
        = std::find_if(_reprs.begin(), _reprs.end(), _ReprComparator(reprToken));
    if (it != _reprs.end()) {
        const HdReprSharedPtr& repr = it->second;
        const auto&            items = repr->GetDrawItems();
#if HD_API_VERSION < 35
        for (HdDrawItem* item : items) {
            HdVP2DrawItem* drawItem = static_cast<HdVP2DrawItem*>(item);
#else
        for (const HdRepr::DrawItemUniquePtr& item : items) {
            HdVP2DrawItem* const drawItem = static_cast<HdVP2DrawItem*>(item.get());
#endif
            if (drawItem) {
                if (drawItem->GetDirtyBits() & HdChangeTracker::AllDirty) {
                    // About to be drawn, but the Repr is dirty. Add DirtyRepr so we know in
                    // _PropagateDirtyBits that we need to propagate the dirty bits of this draw
                    // items to ensure proper Sync
                    drawItem->SetDirtyBits(HdChangeTracker::DirtyRepr);
                }
                if (drawItem->GetDirtyBits() & DirtySelection) {
                    *dirtyBits |= DirtySelectionHighlight;
                }
            }
        }
        return;
    }

#if PXR_VERSION > 2002
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

        const MString& renderItemName = drawItem->GetDrawItemName();

        MHWRender::MRenderItem* renderItem = nullptr;

        switch (desc.geomStyle) {
        case HdMeshGeomStyleHull:
            // Creating the smoothHull hull render items requires geom subsets from the topology,
            // and we can't access that here.
#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
            if (reprToken == HdVP2ReprTokens->defaultMaterial) {
                // But default material mode does not use geom subsets, so we create the render item
                renderItem = _CreateSmoothHullRenderItem(renderItemName);
                renderItem->setDefaultMaterialHandling(
                    MRenderItem::DrawOnlyWhenDefaultMaterialActive);
                renderItem->setShader(_delegate->Get3dDefaultMaterialShader());
            }
#endif
            break;
        case HdMeshGeomStyleHullEdgeOnly:
            // The smoothHull repr uses the wireframe item for selection highlight only.
#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
            if (reprToken == HdReprTokens->smoothHull
                || reprToken == HdVP2ReprTokens->defaultMaterial) {
                // Share selection highlight render item between smoothHull and defaultMaterial:
                bool                        foundShared = false;
                _ReprVector::const_iterator it = std::find_if(
                    _reprs.begin(),
                    _reprs.end(),
                    _ReprComparator(
                        reprToken == HdReprTokens->smoothHull ? HdVP2ReprTokens->defaultMaterial
                                                              : HdReprTokens->smoothHull));
                if (it != _reprs.end()) {
                    const HdReprSharedPtr& repr = it->second;
                    const auto&            items = repr->GetDrawItems();
#if HD_API_VERSION < 35
                    for (HdDrawItem* item : items) {
                        HdVP2DrawItem* shDrawItem = static_cast<HdVP2DrawItem*>(item);
#else
                    for (const HdRepr::DrawItemUniquePtr& item : items) {
                        HdVP2DrawItem* const shDrawItem = static_cast<HdVP2DrawItem*>(item.get());
#endif
                        if (shDrawItem
                            && shDrawItem->MatchesUsage(HdVP2DrawItem::kSelectionHighlight)) {
                            drawItem->SetRenderItem(shDrawItem->GetRenderItem());
                            foundShared = true;
                            break;
                        }
                    }
                }
                if (!foundShared) {
                    renderItem = _CreateSelectionHighlightRenderItem(renderItemName);
                }
                drawItem->SetUsage(HdVP2DrawItem::kSelectionHighlight);
            }
#else
            // The smoothHull repr uses the wireframe item for selection highlight only.
            if (reprToken == HdReprTokens->smoothHull) {
                renderItem = _CreateSelectionHighlightRenderItem(renderItemName);
                drawItem->SetUsage(HdVP2DrawItem::kSelectionHighlight);
            }
#endif
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
#ifndef MAYA_NEW_POINT_SNAPPING_SUPPORT
        case HdMeshGeomStylePoints: renderItem = _CreatePointsRenderItem(renderItemName); break;
#endif
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

void HdVP2Mesh::_CreateSmoothHullRenderItems(HdVP2DrawItem& drawItem)
{
    // 2021-01-29: Changing topology is not tested
    TF_VERIFY(drawItem.GetRenderItems().size() == 0);
    drawItem.GetRenderItems().clear();

    auto* const         param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (ARCH_UNLIKELY(!subSceneContainer))
        return;

    // Need to topology to check for geom subsets.
    const HdMeshTopology& topology = _meshSharedData->_topology;
    const HdGeomSubsets&  geomSubsets = topology.GetGeomSubsets();

    // If the geom subsets do not cover all the faces in the mesh we need
    // to add an additional render item for those faces.
    int numFacesWithoutRenderItem = topology.GetNumFaces();

    // Initialize the face to subset item mapping with an invalid item.
    _meshSharedData->_faceIdToRenderItem.clear();
    _meshSharedData->_faceIdToRenderItem.resize(topology.GetNumFaces(), nullptr);

    // Create the geom subset render items, and fill in the face to subset item mapping for later
    // use.
    for (const auto& geomSubset : geomSubsets) {
        // Right now geom subsets only support face sets, but edge or vertex sets
        // are possible in the future.
        TF_VERIFY(geomSubset.type == HdGeomSubset::TypeFaceSet);
        if (geomSubset.type != HdGeomSubset::TypeFaceSet)
            continue;

        // There can be geom subsets on the object which are not material subsets. I've seen
        // familyName = "object" in usda files. If there is no materialId on the subset then
        // don't create a render item for it.
        if (SdfPath::EmptyPath() == geomSubset.materialId)
            continue;

        MString renderItemName = drawItem.GetDrawItemName();
        renderItemName += std::string(1, VP2_RENDER_DELEGATE_SEPARATOR).c_str();
        renderItemName += geomSubset.id.GetString().c_str();
        MHWRender::MRenderItem* renderItem = _CreateSmoothHullRenderItem(renderItemName);
        drawItem.AddRenderItem(renderItem, &geomSubset);

        // now fill in _faceIdToRenderItem at geomSubset.indices with the subset item pointer
        for (auto faceId : geomSubset.indices) {
            // we expect that material binding geom subsets will not overlap
            TF_VERIFY(nullptr == _meshSharedData->_faceIdToRenderItem[faceId]);
            _meshSharedData->_faceIdToRenderItem[faceId] = renderItem;
        }
        numFacesWithoutRenderItem -= geomSubset.indices.size();
    }

    TF_VERIFY(numFacesWithoutRenderItem >= 0);

    if (numFacesWithoutRenderItem > 0) {
        // create an item for the remaining faces
        MHWRender::MRenderItem* renderItem
            = _CreateSmoothHullRenderItem(drawItem.GetDrawItemName());
        drawItem.AddRenderItem(renderItem);

        if (numFacesWithoutRenderItem == topology.GetNumFaces()) {
            // If there are no geom subsets that are material bind geom subsets, then we don't need
            // the _faceIdToRenderItem mapping, we'll just create one item and use the full topology
            // for it.
            _meshSharedData->_faceIdToRenderItem.clear();
            numFacesWithoutRenderItem = 0;
        } else {
            // now fill in _faceIdToRenderItem at geomSubset.indices with the MRenderItem
            for (auto& renderItemId : _meshSharedData->_faceIdToRenderItem) {
                if (nullptr == renderItemId) {
                    renderItemId = renderItem;
                    numFacesWithoutRenderItem--;
                }
            }
        }
    }

    TF_VERIFY(numFacesWithoutRenderItem == 0);
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

    auto* const         param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (ARCH_UNLIKELY(!subSceneContainer))
        return;

    _MeshReprConfig::DescArray reprDescs = _GetReprDesc(reprToken);

    // For each relevant draw item, update dirty buffer sources.
    int drawItemIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx, drawItemIndex++) {
        const HdMeshReprDesc& desc = reprDescs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleInvalid) {
            continue;
        }
        auto* drawItem = static_cast<HdVP2DrawItem*>(curRepr->GetDrawItem(drawItemIndex));
        if (!drawItem)
            continue;
        if (desc.geomStyle == HdMeshGeomStyleHull) {
            // it is possible we haven't created MRenderItems for this HdDrawItem yet.
            // if there are no MRenderItems, create them.
            if (drawItem->GetRenderItems().size() == 0) {
                _CreateSmoothHullRenderItems(*drawItem);

                for (const auto& renderItemData : drawItem->GetRenderItems()) {
                    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                        [subSceneContainer, &renderItemData]() {
                            subSceneContainer->add(renderItemData._renderItem);
                        });
                }
            }
        }

        for (auto& renderItemData : drawItem->GetRenderItems()) {
            _UpdateDrawItem(sceneDelegate, drawItem, renderItemData, desc);
        }
        // Reset dirty bits because we've prepared commit state for this draw item.
        drawItem->ResetDirtyBits();
    }
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in CommitState and enqueued for Commit on main-thread using CommitTasks
*/
void HdVP2Mesh::_UpdateDrawItem(
    HdSceneDelegate*               sceneDelegate,
    HdVP2DrawItem*                 drawItem,
    HdVP2DrawItem::RenderItemData& renderItemData,
    const HdMeshReprDesc&          desc)
{
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

    MHWRender::MRenderItem*        renderItem = renderItemData._renderItem;
    CommitState                    stateToCommit(renderItemData);
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;
    if (ARCH_UNLIKELY(!renderItem)) {
        return;
    }

    const SdfPath& id = GetId();

    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();

    const HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();

    // The bounding box item uses a globally-shared geometry data therefore it
    // doesn't need to extract index data from topology. Points use non-indexed
    // draw.
    const bool isBBoxItem = (renderItem->drawMode() == MHWRender::MGeometry::kBoundingBox);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    constexpr bool isPointSnappingItem = false;
#else
    const bool isPointSnappingItem = (renderItem->primitive() == MHWRender::MGeometry::kPoints);
#endif

#ifdef HDVP2_ENABLE_GPU_OSD
    const bool isLineItem = (renderItem->primitive() == MHWRender::MGeometry::kLines);
    // when we do OSD we don't bother creating indexing until after we have a smooth mesh
    const bool requiresIndexUpdate = !isBBoxItem && !isPointSnappingItem && isLineItem;
#else
    const bool requiresIndexUpdate = !isBBoxItem && !isPointSnappingItem;
#endif

    // Prepare index buffer.
    if (requiresIndexUpdate && (itemDirtyBits & HdChangeTracker::DirtyTopology)) {
        const HdMeshTopology& topologyToUse = _meshSharedData->_renderingTopology;

        if (desc.geomStyle == HdMeshGeomStyleHull) {
            // _trianglesFaceVertexIndices has the full triangulation calculated in
            // _updateRepr. Find the triangles which represent faces in the matching
            // geom subset and add those triangles to the index buffer for renderItem.

            VtVec3iArray trianglesFaceVertexIndices; // for this item only!
            if (_meshSharedData->_faceIdToRenderItem.size() == 0) {
                // If there is no mapping from face to render item, then all the faces are on this
                // render item.
                // VtArray has copy-on-write semantics so this is fast
                trianglesFaceVertexIndices = _meshSharedData->_trianglesFaceVertexIndices;
            } else {
                for (size_t triangleId = 0; triangleId < _meshSharedData->_primitiveParam.size();
                     triangleId++) {
                    size_t faceId = HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
                        _meshSharedData->_primitiveParam[triangleId]);
                    if (_meshSharedData->_faceIdToRenderItem[faceId] == renderItem) {
                        trianglesFaceVertexIndices.push_back(
                            _meshSharedData->_trianglesFaceVertexIndices[triangleId]);
                    }
                }
            }

            // It is possible that all elements in the opacity array are 1.
            // Due to the performance implications of transparency, we have to
            // traverse the array and enable transparency only when needed.
            renderItemData._transparent = false;
            HdInterpolation alphaInterp = HdInterpolationConstant;
            VtFloatArray    alphaArray;
            _getOpacityData(_meshSharedData->_primvarInfo, alphaArray, alphaInterp);
            if (alphaArray.size() > 0) {
                if (alphaInterp == HdInterpolationConstant) {
                    renderItemData._transparent = (alphaArray[0] < 0.999f);
                } else {
                    for (const auto& triangle : trianglesFaceVertexIndices) {

                        int x = _meshSharedData->_renderingToSceneFaceVtxIds[triangle[0]];
                        int y = _meshSharedData->_renderingToSceneFaceVtxIds[triangle[1]];
                        int z = _meshSharedData->_renderingToSceneFaceVtxIds[triangle[2]];
                        if (alphaArray[x] < 0.999f || alphaArray[y] < 0.999f
                            || alphaArray[z] < 0.999f) {
                            renderItemData._transparent = true;
                            break;
                        }
                    }
                }
            }

            const int numIndex = trianglesFaceVertexIndices.size() * 3;

            stateToCommit._indexBufferData = numIndex > 0
                ? static_cast<int*>(drawItemData._indexBuffer->acquire(numIndex, true))
                : nullptr;
            if (stateToCommit._indexBufferData) {
                memcpy(
                    stateToCommit._indexBufferData,
                    trianglesFaceVertexIndices.data(),
                    numIndex * sizeof(int));
            }
        } else if (desc.geomStyle == HdMeshGeomStyleHullEdgeOnly) {
            unsigned int numIndex = _GetNumOfEdgeIndices(topologyToUse);

            stateToCommit._indexBufferData = numIndex
                ? static_cast<int*>(drawItemData._indexBuffer->acquire(numIndex, true))
                : nullptr;
            _FillEdgeIndices(stateToCommit._indexBufferData, topologyToUse);
        }
    }

#ifdef HDVP2_ENABLE_GPU_COMPUTE
    if (_gpuNormalsEnabled) {
        renderItem->addViewportComputeItem(_meshSharedData->_viewportCompute);
    }
#endif

    if (desc.geomStyle == HdMeshGeomStyleHull
        && desc.shadingTerminal == HdMeshReprDescTokens->surfaceShader) {
        bool dirtyMaterialId = (itemDirtyBits & HdChangeTracker::DirtyMaterialId) != 0;
        if (dirtyMaterialId) {
            SdfPath materialId = GetMaterialId(); // This is an index path
            if (drawItemData._geomSubset.id != SdfPath::EmptyPath()) {
                SdfPath cachePathMaterialId = drawItemData._geomSubset.materialId;
                // This is annoying! The saved materialId is a cache path, but to look up the
                // material in the render index we need the index path.
                materialId = dynamic_cast<UsdImagingDelegate*>(sceneDelegate)
                                 ->ConvertCachePathToIndexPath(cachePathMaterialId);
            }
            const HdVP2Material* material = static_cast<const HdVP2Material*>(
                renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

            if (material) {
                MHWRender::MShaderInstance* shader = material->GetSurfaceShader();
                if (shader != nullptr && shader != drawItemData._shader) {
                    drawItemData._shader = shader;
                    drawItemData._shaderIsFallback = false;
                    stateToCommit._shader = shader;
                    stateToCommit._isTransparent
                        = shader->isTransparent() || renderItemData._transparent;
                }
            } else {
                drawItemData._shaderIsFallback = true;
            }
        }

        bool useFallbackMaterial
            = drawItemData._shaderIsFallback && _PrimvarIsRequired(HdTokens->displayColor);
        bool updateFallbackMaterial = useFallbackMaterial && _meshSharedData->_fallbackColorDirty;

        // Use fallback shader if there is no material binding or we failed to create a shader
        // instance for the material.
        if (updateFallbackMaterial) {
            MHWRender::MShaderInstance* shader = nullptr;

            HdInterpolation colorInterp = HdInterpolationConstant;
            HdInterpolation alphaInterp = HdInterpolationConstant;
            VtVec3fArray    colorArray;
            VtFloatArray    alphaArray;

            _getColorData(_meshSharedData->_primvarInfo, colorArray, colorInterp);
            _getOpacityData(_meshSharedData->_primvarInfo, alphaArray, alphaInterp);

            if ((colorInterp == HdInterpolationConstant || colorInterp == HdInterpolationInstance)
                && (alphaInterp == HdInterpolationConstant
                    || alphaInterp == HdInterpolationInstance)) {
                const GfVec3f& clr3f = MayaUsd::utils::ConvertLinearToMaya(colorArray[0]);
                const MColor   color(clr3f[0], clr3f[1], clr3f[2], alphaArray[0]);
                shader = _delegate->GetFallbackShader(color);
                // The color of the fallback shader is ignored when the interpolation is
                // instance
            } else {
                shader = _delegate->GetFallbackCPVShader();
            }

            if (shader != nullptr && shader != drawItemData._shader) {
                drawItemData._shader = shader;
                stateToCommit._shader = shader;
                stateToCommit._isTransparent = renderItemData._transparent;
                _meshSharedData->_fallbackColorDirty = false;
            }
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

        // If the Rprim has empty bounds, we will assign a null bounding box to the render item and
        // Maya will compute the bounding box from the position data.
        if (!rangeToUse.IsEmpty()) {
            const GfVec3d& min = rangeToUse.GetMin();
            const GfVec3d& max = rangeToUse.GetMax();

            bool boundingBoxExpanded = false;

            const MPoint pntMin(min[0], min[1], min[2]);
            if (!drawItemData._boundingBox.contains(pntMin)) {
                drawItemData._boundingBox.expand(pntMin);
                boundingBoxExpanded = true;
            }

            const MPoint pntMax(max[0], max[1], max[2]);
            if (!drawItemData._boundingBox.contains(pntMax)) {
                drawItemData._boundingBox.expand(pntMax);
                boundingBoxExpanded = true;
            }

            if (boundingBoxExpanded) {
                stateToCommit._boundingBox = &drawItemData._boundingBox;
            }
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
        bool enable = drawItem->GetVisible() && !_points(_meshSharedData->_primvarInfo).empty()
            && !instancerWithNoInstances;

        if (isDedicatedSelectionHighlightItem) {
            enable = enable && (_selectionStatus != kUnselected);
        } else if (isPointSnappingItem) {
            enable = enable && (_selectionStatus == kUnselected);
        } else if (isBBoxItem) {
            enable = enable && !range.IsEmpty();
        }

        enable = enable && drawScene.DrawRenderTag(_meshSharedData->_renderTag);

        if (drawItemData._enabled != enable) {
            drawItemData._enabled = enable;
            stateToCommit._enabled = &drawItemData._enabled;
        }
    }

    stateToCommit._geometryDirty
        = (itemDirtyBits
           & (HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyNormals
              | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyTopology));

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    if (!isBBoxItem && !isDedicatedSelectionHighlightItem
        && (itemDirtyBits & DirtySelectionHighlight)) {
        MSelectionMask selectionMask(MSelectionMask::kSelectMeshes);

        // Only unselected Rprims can be used for point snapping.
        if (_selectionStatus == kUnselected) {
            selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
        }

        // The function is thread-safe, thus called in place to keep simple.
        renderItem->setSelectionMask(selectionMask);
    }
#endif

    // Capture buffers we need
    MHWRender::MIndexBuffer* indexBuffer = drawItemData._indexBuffer.get();
    PrimvarInfoMap*          primvarInfo = &_meshSharedData->_primvarInfo;
    const HdVP2BBoxGeom&     sharedBBoxGeom = _delegate->GetSharedBBoxGeom();
    if (isBBoxItem) {
        indexBuffer = const_cast<MHWRender::MIndexBuffer*>(sharedBBoxGeom.GetIndexBuffer());
    }

    _delegate->GetVP2ResourceRegistry().EnqueueCommit(
        [stateToCommit, param, primvarInfo, indexBuffer, isBBoxItem, &sharedBBoxGeom]() {
            const HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;
            MHWRender::MRenderItem*              renderItem = drawItemData._renderItem;
            if (ARCH_UNLIKELY(!renderItem))
                return;

            MProfilingScope profilingScope(
                HdVP2RenderDelegate::sProfilerCategory,
                MProfiler::kColorC_L2,
                renderItem->name().asChar(),
                "Commit");

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

            // TODO: this is now including all buffers for the requirements of all
            // the render items on this rprim. We could filter it down based on the
            // requirements of the shader.
            if (stateToCommit._geometryDirty || stateToCommit._boundingBox) {
                MHWRender::MVertexBufferArray vertexBuffers;

                for (auto& entry : *primvarInfo) {
                    const TfToken&            primvarName = entry.first;
                    MHWRender::MVertexBuffer* primvarBuffer = nullptr;
                    if (isBBoxItem && primvarName == HdTokens->points) {
                        primvarBuffer = const_cast<MHWRender::MVertexBuffer*>(
                            sharedBBoxGeom.GetPositionBuffer());
                    } else {
                        primvarBuffer = entry.second->_buffer.get();
                    }
                    if (primvarBuffer) { // this filters out the separate color & alpha entries
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
            auto& oldInstanceCount = stateToCommit._renderItemData._instanceCount;
            auto  newInstanceCount = stateToCommit._instanceTransforms.length();

            // GPU instancing has been enabled. We cannot switch to consolidation
            // without recreating render item, so we keep using GPU instancing.
            if (stateToCommit._renderItemData._usingInstancedDraw) {
                if (oldInstanceCount == newInstanceCount) {
                    for (unsigned int i = 0; i < newInstanceCount; i++) {
                        // VP2 defines instance ID of the first instance to be 1.
                        drawScene.updateInstanceTransform(
                            *renderItem, i + 1, stateToCommit._instanceTransforms[i]);
                    }
                } else {
                    drawScene.setInstanceTransformArray(
                        *renderItem, stateToCommit._instanceTransforms);
                }

                // upload any extra instance data
                for (auto& entry : *primvarInfo) {
                    const MFloatArray& extraInstanceData = entry.second->_extraInstanceData;
                    if (extraInstanceData.length() == 0)
                        continue;

                    const TfToken& primvarName = entry.first;
                    if (primvarName == HdVP2Tokens->displayColorAndOpacity) {
                        drawScene.setExtraInstanceData(
                            *renderItem, kDiffuseColorStr, extraInstanceData);
                    }
                }

                if (stateToCommit._instanceColors.length()
                    == newInstanceCount * kNumColorChannels) {
                    drawScene.setExtraInstanceData(
                        *renderItem, kSolidColorStr, stateToCommit._instanceColors);
                }
            }
#if MAYA_API_VERSION >= 20210000
            else if (newInstanceCount >= 1) {
#else
            // In Maya 2020 and before, GPU instancing and consolidation are two separate systems
            // that cannot be used by a render item at the same time. In case of single instance, we
            // keep the original render item to allow consolidation with other prims. In case of
            // multiple instances, we need to disable consolidation to allow GPU instancing to be
            // used.
            else if (newInstanceCount == 1) {
                renderItem->setMatrix(&stateToCommit._instanceTransforms[0]);
            } else if (newInstanceCount > 1) {
                setWantConsolidation(*renderItem, false);
#endif
                drawScene.setInstanceTransformArray(*renderItem, stateToCommit._instanceTransforms);

                if (stateToCommit._instanceColors.length()
                    == newInstanceCount * kNumColorChannels) {
                    drawScene.setExtraInstanceData(
                        *renderItem, kSolidColorStr, stateToCommit._instanceColors);
                }

                // upload any extra instance data
                for (auto& entry : *primvarInfo) {
                    const MFloatArray& extraInstanceData = entry.second->_extraInstanceData;
                    if (extraInstanceData.length() == 0)
                        continue;

                    const TfToken& primvarName = entry.first;
                    if (primvarName == HdVP2Tokens->displayColorAndOpacity) {
                        drawScene.setExtraInstanceData(
                            *renderItem, kDiffuseColorStr, extraInstanceData);
                    }
                }

                stateToCommit._renderItemData._usingInstancedDraw = true;
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

        for (auto& renderItemData : drawItem->GetRenderItems()) {
            renderItemData._enabled = false;
            _delegate->GetVP2ResourceRegistry().EnqueueCommit(
                [&]() { renderItemData._renderItem->enable(false); });
        }
    }
}

#ifdef HDVP2_ENABLE_GPU_COMPUTE
/*! \brief  Save topology information for later GPGPU evaluation

    This function pulls topology and UV data from the scene delegate and save that
    information to be used as an input to the normal calculation later.
*/
void HdVP2Mesh::_CreateViewportCompute()
{
    if (!_meshSharedData->_viewportCompute) {
        _meshSharedData->_viewportCompute
            = MSharedPtr<MeshViewportCompute>::make<>(_meshSharedData);
    }
}
#endif

#ifdef HDVP2_ENABLE_GPU_OSD
void HdVP2Mesh::_CreateOSDTables()
{
#if defined(DO_CPU_OSD) || defined(DO_OPENGL_OSD)

    assert(_meshSharedData->_viewportCompute);
    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "createOSDTables");

    // create topology refiner
    PxOsdTopologyRefinerSharedPtr refiner;

    OpenSubdiv::Far::StencilTable const* vertexStencils = nullptr;
    OpenSubdiv::Far::StencilTable const* varyingStencils = nullptr;
    OpenSubdiv::Far::PatchTable const*   patchTable = nullptr;

    HdMeshTopology* topology
        = &_meshSharedData->_renderingTopology; // TODO: something with _topology?

    // for empty topology, we don't need to refine anything.
    // but still need to return the typed buffer for codegen
    if (topology->GetFaceVertexCounts().size() == 0) {
        // leave refiner empty
    } else {
        refiner = PxOsdRefinerFactory::Create(
            topology->GetPxOsdMeshTopology(), TfToken(_meshSharedData->_renderTag.GetText()));
    }

    if (refiner) {
        OpenSubdiv::Far::PatchTableFactory::Options patchOptions(
            _meshSharedData->_viewportCompute->level);
        if (_meshSharedData->_viewportCompute->adaptive) {
            patchOptions.endCapType
                = OpenSubdiv::Far::PatchTableFactory::Options::ENDCAP_BSPLINE_BASIS;
#if OPENSUBDIV_VERSION_NUMBER >= 30400
            // Improve fidelity when refining to limit surface patches
            // These options supported since v3.1.0 and v3.2.0 respectively.
            patchOptions.useInfSharpPatch = true;
            patchOptions.generateLegacySharpCornerPatches = false;
#endif
        }

        // split trace scopes.
        {
            MProfilingScope subProfilingScope(
                HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "refine");
            if (_meshSharedData->_viewportCompute->adaptive) {
                OpenSubdiv::Far::TopologyRefiner::AdaptiveOptions adaptiveOptions(
                    _meshSharedData->_viewportCompute->level);
#if OPENSUBDIV_VERSION_NUMBER >= 30400
                adaptiveOptions = patchOptions.GetRefineAdaptiveOptions();
#endif
                refiner->RefineAdaptive(adaptiveOptions);
            } else {
                refiner->RefineUniform(_meshSharedData->_viewportCompute->level);
            }
        }
#define GENERATE_SOURCE_TABLES
#ifdef GENERATE_SOURCE_TABLES
        {
            MProfilingScope subProfilingScope(
                HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "stencilFactory");
            OpenSubdiv::Far::StencilTableFactory::Options options;
            options.generateOffsets = true;
            options.generateIntermediateLevels = _meshSharedData->_viewportCompute->adaptive;
            options.interpolationMode = OpenSubdiv::Far::StencilTableFactory::INTERPOLATE_VERTEX;
            vertexStencils = OpenSubdiv::Far::StencilTableFactory::Create(*refiner, options);

            options.interpolationMode = OpenSubdiv::Far::StencilTableFactory::INTERPOLATE_VARYING;
            varyingStencils = OpenSubdiv::Far::StencilTableFactory::Create(*refiner, options);
        }
        {
            MProfilingScope subProfilingScope(
                HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "patchFactory");
            patchTable = OpenSubdiv::Far::PatchTableFactory::Create(*refiner, patchOptions);
        }
#else
        // grab the values we need from the refiner.
        const OpenSubdiv::Far::TopologyLevel& refinedLevel
            = refiner->GetLevel(refiner->GetMaxLevel());
        size_t indexLength = refinedLevel.GetNumFaces()
            * 4; // i know it is quads but not always? can we do this more safely?
        size_t vertexLength = GetNumVerticesTotal();
        // save these values and use them to create the updated geometry index mapping.
#endif
    }
#ifdef GENERATE_SOURCE_TABLES
    // merge endcap
    if (patchTable && patchTable->GetLocalPointStencilTable()) {
        // append stencils
        if (OpenSubdiv::Far::StencilTable const* vertexStencilsWithLocalPoints
            = OpenSubdiv::Far::StencilTableFactory::AppendLocalPointStencilTable(
                *refiner, vertexStencils, patchTable->GetLocalPointStencilTable())) {
            delete vertexStencils;
            vertexStencils = vertexStencilsWithLocalPoints;
        }
        if (OpenSubdiv::Far::StencilTable const* varyingStencilsWithLocalPoints
            = OpenSubdiv::Far::StencilTableFactory::AppendLocalPointStencilTable(
                *refiner, varyingStencils, patchTable->GetLocalPointStencilTable())) {
            delete varyingStencils;
            varyingStencils = varyingStencilsWithLocalPoints;
        }
    }

    // save values for the next loop
    _meshSharedData->_viewportCompute->vertexStencils.reset(vertexStencils);
    _meshSharedData->_viewportCompute->varyingStencils.reset(varyingStencils);
    _meshSharedData->_viewportCompute->patchTable.reset(patchTable);
#endif

    // if there is a sourceMeshSharedData it should have entries for every vertex in that geometry
    // source.

#endif
}
#endif

/*! \brief  Update the _primvarInfo's _source information for all required primvars.

    This function pulls data from the scene delegate & caches it, but defers processing.

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

    auto updatePrimvarInfo
        = [&](const TfToken& name, const VtValue& value, const HdInterpolation interpolation) {
              PrimvarInfo* info = _getInfo(_meshSharedData->_primvarInfo, name);
              if (info) {
                  info->_source.data = value;
                  info->_source.interpolation = interpolation;
                  info->_source.dataSource = PrimvarSource::Primvar;
              } else {
                  _meshSharedData->_primvarInfo[name] = std::make_unique<PrimvarInfo>(
                      PrimvarSource(value, interpolation, PrimvarSource::Primvar), nullptr);
              }
          };

    TfTokenVector::const_iterator begin = requiredPrimvars.cbegin();
    TfTokenVector::const_iterator end = requiredPrimvars.cend();

    // inspired by HdStInstancer::_SyncPrimvars
    // Get any required instanced primvars from the instancer. Get these before we get
    // any rprims from the rprim itself. If both are present, the rprim's values override
    // the instancer's value.
    const SdfPath& instancerId = GetInstancerId();
    if (!instancerId.IsEmpty()) {
        HdPrimvarDescriptorVector instancerPrimvars
            = sceneDelegate->GetPrimvarDescriptors(instancerId, HdInterpolationInstance);
        for (const HdPrimvarDescriptor& pv : instancerPrimvars) {
            if (std::find(begin, end, pv.name) == end) {
                // erase the unused primvar so we don't hold onto stale data
                _meshSharedData->_primvarInfo.erase(pv.name);
            } else {
                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, instancerId, pv.name)) {
                    const VtValue value = sceneDelegate->Get(instancerId, pv.name);
                    updatePrimvarInfo(pv.name, value, HdInterpolationInstance);
                }
            }
        }
    }

    for (size_t i = 0; i < HdInterpolationCount; i++) {
        const HdInterpolation           interp = static_cast<HdInterpolation>(i);
        const HdPrimvarDescriptorVector primvars = GetPrimvarDescriptors(sceneDelegate, interp);

        for (const HdPrimvarDescriptor& pv : primvars) {
            if (std::find(begin, end, pv.name) == end) {
                // erase the unused primvar so we don't hold onto stale data
                _meshSharedData->_primvarInfo.erase(pv.name);
            } else {
                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                    const VtValue value = GetPrimvar(sceneDelegate, pv.name);
                    updatePrimvarInfo(pv.name, value, interp);

                    // if the primvar color changes then we might need to use a different fallback
                    // material
                    if (interp == HdInterpolationConstant && pv.name == HdTokens->displayColor) {
                        _meshSharedData->_fallbackColorDirty = true;
                    }
                }
            }
        }
    }
}

#ifndef MAYA_NEW_POINT_SNAPPING_SUPPORT
/*! \brief  Create render item for points repr.
 */
MHWRender::MRenderItem* HdVP2Mesh::_CreatePointsRenderItem(const MString& name) const
{
    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        name, MHWRender::MRenderItem::DecorationItem, MHWRender::MGeometry::kPoints);

    renderItem->setDrawMode(MHWRender::MGeometry::kSelectionOnly);
    renderItem->depthPriority(MHWRender::MRenderItem::sDormantPointDepthPriority);
    renderItem->castsShadows(false);
    renderItem->receivesShadows(false);
    renderItem->setShader(_delegate->Get3dFatPointShader());

    MSelectionMask selectionMask(MSelectionMask::kSelectPointsForGravity);
    selectionMask.addMask(MSelectionMask::kSelectMeshVerts);
    renderItem->setSelectionMask(selectionMask);

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeMeshes);
#endif

    setWantConsolidation(*renderItem, true);

    return renderItem;
}
#endif

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

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MSelectionMask selectionMask(MSelectionMask::kSelectMeshes);
    selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
    renderItem->setSelectionMask(selectionMask);
#else
    renderItem->setSelectionMask(MSelectionMask::kSelectMeshes);
#endif

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeMeshes);
#endif

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

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeMeshes);
#endif

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

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    MSelectionMask selectionMask(MSelectionMask::kSelectMeshes);
    selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
    renderItem->setSelectionMask(selectionMask);
#else
    renderItem->setSelectionMask(MSelectionMask::kSelectMeshes);
#endif

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeMeshes);
#endif

#ifdef HAS_DEFAULT_MATERIAL_SUPPORT_API
    renderItem->setDefaultMaterialHandling(MRenderItem::SkipWhenDefaultMaterialActive);
#endif

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

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeMeshes);
#endif

    setWantConsolidation(*renderItem, true);

    return renderItem;
}

PXR_NAMESPACE_CLOSE_SCOPE
