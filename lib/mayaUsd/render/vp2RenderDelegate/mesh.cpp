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
#include <pxr/imaging/hd/extCompCpuComputation.h>
#include <pxr/imaging/hd/extCompPrimvarBufferSource.h>
#include <pxr/imaging/hd/extComputation.h>
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
    } else {
        for (size_t i = 0; i < colorArray.size(); ++i) {
            colorArray[i] = MayaUsd::utils::ConvertLinearToMaya(colorArray[i]);
        }
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
    , MayaUsdRPrim(delegate, id)
{
    _meshSharedData = std::make_shared<HdVP2MeshSharedData>();
    // HdChangeTracker::IsVarying() can check dirty bits to tell us if an object is animated or not.
    // Not sure if it is correct on file load

#ifdef HDVP2_ENABLE_GPU_COMPUTE
    static std::once_flag initGPUComputeOnce;
    std::call_once(initGPUComputeOnce, _InitGPUCompute);
#endif
}

void HdVP2Mesh::_PrepareSharedVertexBuffers(
    HdSceneDelegate*   delegate,
    const HdDirtyBits& rprimDirtyBits,
    const TfToken&     reprToken)
{
    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2,
        _rprimId.asChar(),
        "HdVP2Mesh::_PrepareSharedVertexBuffers");

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
                if (!_meshSharedData->_adjacency) {
                    _meshSharedData->_adjacency.reset(new Hd_VertexAdjacency());

                    HdBufferSourceSharedPtr adjacencyComputation
                        = _meshSharedData->_adjacency->GetSharedAdjacencyBuilderComputation(
                            &_meshSharedData->_topology);
                    MProfilingScope profilingScope(
                        HdVP2RenderDelegate::sProfilerCategory,
                        MProfiler::kColorC_L2,
                        _rprimId.asChar(),
                        "HdVP2Mesh::computeAdjacency");

                    adjacencyComputation->Resolve();
                }

                // Only the points referenced by the topology are used to compute
                // smooth normals.
                VtValue normals(Hd_SmoothNormals::ComputeSmoothNormals(
                    _meshSharedData->_adjacency.get(),
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
            colorAndOpacityInfo->_source.interpolation = HdInterpolationInstance;

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
            } else if (value.IsHolding<VtIntArray>()) {
                if (!buffer) {
                    const MHWRender::MVertexBufferDescriptor vbDesc(
                        "", semantic, MHWRender::MGeometry::kFloat, 1); // kInt32

                    buffer = new MHWRender::MVertexBuffer(vbDesc);
                    _meshSharedData->_primvarInfo[token]->_buffer.reset(buffer);
                }

                if (buffer) {
                    bufferData = _meshSharedData->_numVertices > 0
                        ? buffer->acquire(_meshSharedData->_numVertices, true)
                        : nullptr;
                    if (bufferData) {
                        const VtIntArray& primvarData = value.UncheckedGet<VtIntArray>();

                        VtFloatArray convertedPrimvarData;
                        convertedPrimvarData.reserve(primvarData.size());
                        for (auto& source : primvarData) {
                            convertedPrimvarData.push_back(static_cast<float>(source));
                        }

                        _FillPrimvarData(
                            static_cast<float*>(bufferData),
                            _meshSharedData->_numVertices,
                            0,
                            _meshSharedData->_renderingToSceneFaceVtxIds,
                            _rprimId,
                            _meshSharedData->_topology,
                            token,
                            convertedPrimvarData,
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
    TfToken const&   reprToken)
{
    if (!_SyncCommon(*this, delegate, renderParam, dirtyBits, _GetRepr(reprToken), reprToken)) {
        return;
    }

    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2,
        _rprimId.asChar(),
        "HdVP2Mesh::Sync");

    const SdfPath&       id = GetId();
    HdRenderIndex&       renderIndex = delegate->GetRenderIndex();
    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    UsdImagingDelegate*  usdImagingDelegate = drawScene.GetUsdImagingDelegate();

    // Geom subsets are accessed through the mesh topology. I need to know about
    // the additional materialIds that get bound by geom subsets before we build the
    // _primvaInfo. So the very first thing I need to do is grab the topology.
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        // unsubscribe from material TopoChanged updates from the old geom subset materials
        for (const auto& geomSubset : _meshSharedData->_topology.GetGeomSubsets()) {
            if (!geomSubset.materialId.IsEmpty()) {
                const SdfPath materialId
                    = usdImagingDelegate->ConvertCachePathToIndexPath(geomSubset.materialId);
                HdVP2Material* material = static_cast<HdVP2Material*>(
                    renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

                if (material) {
                    material->UnsubscribeFromMaterialUpdates(id);
                }
            }
        }

        {
            MProfilingScope profilingScope(
                HdVP2RenderDelegate::sProfilerCategory,
                MProfiler::kColorC_L2,
                _rprimId.asChar(),
                "HdVP2Mesh::GetMeshTopology");
            HdMeshTopology newTopology = GetMeshTopology(delegate);

            // Test to see if the topology actually changed. If not, we don't have to do anything!
            // Don't test IsTopologyDirty anywhere below this because it is not accurate. Instead
            // using the _indexBufferValid flag on render item data.
            if (!(newTopology == _meshSharedData->_topology)) {
                _meshSharedData->_topology = newTopology;
                _meshSharedData->_adjacency.reset();
                _meshSharedData->_renderingTopology = HdMeshTopology();

                RenderItemFunc setIndexBufferDirty
                    = [](HdVP2DrawItem::RenderItemData& renderItemData) {
                          renderItemData._indexBufferValid = false;
                      };
                _ForEachRenderItem(_reprs, setIndexBufferDirty);
            }
        }

        // subscribe to material TopoChanged updates from the new geom subset materials
        for (const auto& geomSubset : _meshSharedData->_topology.GetGeomSubsets()) {
            if (!geomSubset.materialId.IsEmpty()) {
                const SdfPath materialId
                    = usdImagingDelegate->ConvertCachePathToIndexPath(geomSubset.materialId);
                HdVP2Material* material = static_cast<HdVP2Material*>(
                    renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

                if (material) {
                    material->SubscribeForMaterialUpdates(id);
                }
            }
        }
    }

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        const SdfPath materialId = _GetUpdatedMaterialId(this, delegate);
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

    // if the instancer is dirty then any streams with instance interpolation need to be updated.
    // We don't necessarily know if there ARE any streams with instance interpolation, so call
    // _UpdatePrimvarSources to check.
    bool instancerDirty
        = ((*dirtyBits
            & (HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyInstancer
               | HdChangeTracker::DirtyInstanceIndex))
           != 0);

    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)
        || HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->normals)
        || HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->primvar) || instancerDirty) {

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
                usdImagingDelegate->ConvertCachePathToIndexPath(geomSubset.materialId));
        }

        // also, we always require points
        if (!_PrimvarIsRequired(HdTokens->points))
            _meshSharedData->_allRequiredPrimvars.push_back(HdTokens->points);

        _UpdatePrimvarSources(delegate, *dirtyBits, _meshSharedData->_allRequiredPrimvars);
    }

    if (_meshSharedData->_renderingTopology == HdMeshTopology()) {
        MProfilingScope profilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorC_L2,
            _rprimId.asChar(),
            "HdVP2Mesh Create Rendering Topology");

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

#if PXR_VERSION > 2111
    const TfToken& renderTag = GetRenderTag();
#else
    const TfToken& renderTag = delegate->GetRenderTag(id);
#endif

    _SyncSharedData(_sharedData, delegate, dirtyBits, reprToken, *this, _reprs, renderTag);

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
        | HdChangeTracker::DirtyNormals | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyInstanceIndex
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

    // This support UsdSkel affecting the points position when th etransform is dirty.
    if (bits & HdChangeTracker::DirtyTransform && _pointsFromSkel) {
        bits |= HdChangeTracker::DirtyPoints;
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

    _PropagateDirtyBitsCommon(bits, _reprs);

    return bits;
}

/*! \brief  Initialize the given representation of this Rprim.

    This is called prior to syncing the prim, the first time the repr
    is used.

    \param  reprToken   the name of the repr to initialize.  HdRprim has already
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

    HdReprSharedPtr repr = _InitReprCommon(*this, reprToken, _reprs, dirtyBits, GetId());
    if (!repr)
        return;

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
                MHWRender::MRenderItem* defaultMaterialItem
                    = _CreateSmoothHullRenderItem(
#if HD_API_VERSION < 35
                          renderItemName, *drawItem, *subSceneContainer, nullptr)
#else
                          renderItemName, *drawItem.get(), *subSceneContainer, nullptr)
#endif
                          ._renderItem;
                defaultMaterialItem->setDefaultMaterialHandling(
                    MRenderItem::DrawOnlyWhenDefaultMaterialActive);
                defaultMaterialItem->setShader(_delegate->Get3dDefaultMaterialShader());
#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
                if (!GetInstancerId().IsEmpty()) {
                    defaultMaterialItem = _CreateShadedSelectedInstancesItem(
#if HD_API_VERSION < 35
                        renderItemName, *drawItem, *subSceneContainer, nullptr);
#else
                        renderItemName, *drawItem.get(), *subSceneContainer, nullptr);
#endif
                    defaultMaterialItem->setDefaultMaterialHandling(
                        MRenderItem::DrawOnlyWhenDefaultMaterialActive);
                    defaultMaterialItem->setShader(_delegate->Get3dDefaultMaterialShader());
                }
#endif
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
                renderItem = _CreateWireframeRenderItem(
                    renderItemName,
                    kOpaqueBlue,
                    MSelectionMask::kSelectMeshes,
                    MHWRender::MFrameContext::kExcludeMeshes);
                drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
            }
            // The item is used for bbox display and selection highlight.
            else if (reprToken == HdVP2ReprTokens->bbox) {
                renderItem = _CreateBoundingBoxRenderItem(
                    renderItemName,
                    kOpaqueBlue,
                    MSelectionMask::kSelectMeshes,
                    MHWRender::MFrameContext::kExcludeMeshes);
                drawItem->AddUsage(HdVP2DrawItem::kSelectionHighlight);
            }
            break;
#ifndef MAYA_NEW_POINT_SNAPPING_SUPPORT
        case HdMeshGeomStylePoints:
            renderItem = _CreatePointsRenderItem(
                renderItemName,
                MSelectionMask::kSelectMeshVerts,
                MHWRender::MFrameContext::kExcludeMeshes);
            break;
#endif
        default: TF_WARN("Unsupported geomStyle"); break;
        }

        if (renderItem) {
            _AddRenderItem(*drawItem, renderItem, *subSceneContainer);
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

void HdVP2Mesh::_CreateSmoothHullRenderItems(
    HdVP2DrawItem&      drawItem,
    MSubSceneContainer& subSceneContainer)
{
    // 2021-01-29: Changing topology is not tested
    TF_VERIFY(drawItem.GetRenderItems().size() == 0);
    drawItem.GetRenderItems().clear();

    // Need to topology to check for geom subsets.
    const HdMeshTopology& topology = _meshSharedData->_topology;
    const HdGeomSubsets&  geomSubsets = topology.GetGeomSubsets();

    // If the geom subsets do not cover all the faces in the mesh we need
    // to add an additional render item for those faces.
    int numFacesWithoutRenderItem = topology.GetNumFaces();

    // Initialize the face to subset item mapping with an invalid item.
    _meshSharedData->_faceIdToGeomSubsetId.clear();
    _meshSharedData->_faceIdToGeomSubsetId.resize(topology.GetNumFaces(), SdfPath::EmptyPath());

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
        _CreateSmoothHullRenderItem(renderItemName, drawItem, subSceneContainer, &geomSubset);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
        if (!GetInstancerId().IsEmpty()) {
            _CreateShadedSelectedInstancesItem(
                renderItemName, drawItem, subSceneContainer, &geomSubset);
        }
#endif

        // now fill in _faceIdToGeomSubsetId at geomSubset.indices with the subset item pointer
        for (auto faceId : geomSubset.indices) {
            if (faceId >= topology.GetNumFaces()) {
                MString warning("Skipping faceID(");
                warning += faceId;
                warning += ") on GeomSubset \"";
                warning += geomSubset.id.GetString().c_str();
                warning += "\": greater than the number of faces in the mesh.";
                MGlobal::displayWarning(warning);
                continue;
            }
            // we expect that material binding geom subsets will not overlap
            TF_VERIFY(SdfPath::EmptyPath() == _meshSharedData->_faceIdToGeomSubsetId[faceId]);
            _meshSharedData->_faceIdToGeomSubsetId[faceId] = geomSubset.id;
        }
        numFacesWithoutRenderItem -= geomSubset.indices.size();
    }

    TF_VERIFY(numFacesWithoutRenderItem >= 0);

    if (numFacesWithoutRenderItem > 0) {
        // create an item for the remaining faces
        _CreateSmoothHullRenderItem(
            drawItem.GetDrawItemName(), drawItem, subSceneContainer, nullptr);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
        if (!GetInstancerId().IsEmpty()) {
            _CreateShadedSelectedInstancesItem(
                drawItem.GetDrawItemName(), drawItem, subSceneContainer, nullptr);
        }
#endif

        if (numFacesWithoutRenderItem == topology.GetNumFaces()) {
            // If there are no geom subsets that are material bind geom subsets, then we don't need
            // the _faceIdToGeomSubsetId mapping, we'll just create one item and use the full
            // topology for it.
            _meshSharedData->_faceIdToGeomSubsetId.clear();
            numFacesWithoutRenderItem = 0;
        }
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

    auto* const         param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    MSubSceneContainer* subSceneContainer = param->GetContainer();
    if (ARCH_UNLIKELY(!subSceneContainer))
        return;

    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2,
        _rprimId.asChar(),
        "HdVP2Mesh::_UpdateRepr");

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
                _CreateSmoothHullRenderItems(*drawItem, *subSceneContainer);
            }
        }

        for (auto& renderItemData : drawItem->GetRenderItems()) {
            _UpdateDrawItem(sceneDelegate, drawItem, renderItemData, desc, reprToken);
        }
    }
}

/*! \brief  Update the draw item

    This call happens on worker threads and results of the change are collected
    in MayaUsdCommitState and enqueued for Commit on main-thread using CommitTasks
*/
void HdVP2Mesh::_UpdateDrawItem(
    HdSceneDelegate*               sceneDelegate,
    HdVP2DrawItem*                 drawItem,
    HdVP2DrawItem::RenderItemData& renderItemData,
    const HdMeshReprDesc&          desc,
    const TfToken&                 reprToken)
{
    HdDirtyBits itemDirtyBits = renderItemData.GetDirtyBits();

    auto* const          param = static_cast<HdVP2RenderParam*>(_delegate->GetRenderParam());
    ProxyRenderDelegate& drawScene = param->GetDrawScene();
    UsdImagingDelegate*  usdImagingDelegate = drawScene.GetUsdImagingDelegate();

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
    // We don't need to update the shaded selected instance item when the selection mode is not
    // dirty.
    const bool isShadedSelectedInstanceItem = renderItemData._shadedSelectedInstances;
    const bool usingShadedSelectedInstanceItem
        = !GetInstancerId().IsEmpty() && drawScene.SnapToPoints();
    const bool updateShadedSelectedInstanceItem = (itemDirtyBits & DirtySelectionMode) != 0;
    if (isShadedSelectedInstanceItem && !usingShadedSelectedInstanceItem
        && !updateShadedSelectedInstanceItem) {
        return;
    }
#else
    const bool isShadedSelectedInstanceItem = false;
    const bool usingShadedSelectedInstanceItem = false;
#endif

    const bool isDedicatedHighlightItem
        = drawItem->MatchesUsage(HdVP2DrawItem::kSelectionHighlight);
    const bool isHighlightItem = drawItem->ContainsUsage(HdVP2DrawItem::kSelectionHighlight);
    const bool inTemplateMode = _displayLayerModes._displayType == MayaUsdRPrim::kTemplate;
    const bool inReferenceMode = _displayLayerModes._displayType == MayaUsdRPrim::kReference;
    const bool inPureSelectionHighlightMode = isDedicatedHighlightItem && !inTemplateMode;

    // We don't need to update the selection-highlight-only item when there is no selection
    // highlight change and the mesh is not selected. Render item stores its own
    // dirty bits, so the proper update will be done when it shows in the viewport.
    if (inPureSelectionHighlightMode && ((itemDirtyBits & DirtySelectionHighlight) == 0)
        && (_selectionStatus == kUnselected)) {
        return;
    }

    MHWRender::MRenderItem*        renderItem = renderItemData._renderItem;
    MayaUsdCommitState             stateToCommit(renderItemData);
    HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;
    if (ARCH_UNLIKELY(!renderItem)) {
        return;
    }

    const SdfPath& id = GetId();

    const HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();

    // The bounding box item uses a globally-shared geometry data therefore it
    // doesn't need to extract index data from topology. Points use non-indexed
    // draw.
    const bool isBBoxItem = (renderItem->drawMode() & MHWRender::MGeometry::kBoundingBox) != 0;

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
    if (requiresIndexUpdate && !renderItemData._indexBufferValid) {
        const HdMeshTopology& topologyToUse = _meshSharedData->_renderingTopology;

        if (desc.geomStyle == HdMeshGeomStyleHull) {
            MProfilingScope profilingScope(
                HdVP2RenderDelegate::sProfilerCategory,
                MProfiler::kColorC_L2,
                _rprimId.asChar(),
                "HdVP2Mesh prepare index buffer");

            // _trianglesFaceVertexIndices has the full triangulation calculated in
            // _updateRepr. Find the triangles which represent faces in the matching
            // geom subset and add those triangles to the index buffer for renderItem.

            VtVec3iArray     trianglesFaceVertexIndices; // for this item only!
            std::vector<int> faceIds;
            if (_meshSharedData->_faceIdToGeomSubsetId.size() == 0
                || reprToken == HdVP2ReprTokens->defaultMaterial) {
                // If there is no mapping from face to render item or if this is the default
                // material item then all the faces are on this render item. VtArray has
                // copy-on-write semantics so this is fast
                trianglesFaceVertexIndices = _meshSharedData->_trianglesFaceVertexIndices;
            } else {
                for (size_t triangleId = 0; triangleId < _meshSharedData->_primitiveParam.size();
                     triangleId++) {
                    size_t faceId = HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
                        _meshSharedData->_primitiveParam[triangleId]);
                    if (_meshSharedData->_faceIdToGeomSubsetId[faceId]
                        == renderItemData._geomSubset.id) {
                        faceIds.push_back(faceId);
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
                } else if (alphaInterp == HdInterpolationUniform) {
                    if (faceIds.size() > 0) {
                        // it is a geom subset
                        for (auto& faceId : faceIds) {
                            if (alphaArray[faceId] < 0.999f) {
                                renderItemData._transparent = true;
                                break;
                            }
                        }
                    } else {
                        // no geom subsets, check every face
                        int numFaces = topologyToUse.GetNumFaces();
                        for (int faceId = 0; faceId < numFaces; faceId++) {
                            if (alphaArray[faceId] < 0.999f) {
                                renderItemData._transparent = true;
                                break;
                            }
                        }
                    }
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
        renderItemData._indexBufferValid = true;
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
                materialId = usdImagingDelegate->ConvertCachePathToIndexPath(cachePathMaterialId);
            }
            const HdVP2Material* material = static_cast<const HdVP2Material*>(
                renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

            if (material) {
                MHWRender::MShaderInstance* shader = material->GetSurfaceShader();
                if (shader != nullptr
                    && (shader != drawItemData._shader || shader != stateToCommit._shader)) {
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
        bool updateFallbackMaterial = useFallbackMaterial && drawItemData._fallbackColorDirty;

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
                const GfVec3f& clr3f = colorArray[0];
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
                drawItemData._fallbackColorDirty = false;
            }
        }
    }

    // Local bounds
    const GfRange3d& range = _sharedData.bounds.GetRange();

    _UpdateTransform(stateToCommit, _sharedData, itemDirtyBits, isBBoxItem);
    MMatrix& worldMatrix = drawItemData._worldMatrix;

    // If the mesh is instanced, create one new instance per transform.
    // The current instancer invalidation tracking makes it hard for
    // us to tell whether transforms will be dirty, so this code
    // pulls them every time something changes. Then, it compares the
    // new transforms and the old transforms. If they are the same, skip
    // updating Maya.
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
        } else {
            // The shaded instances are split into two render items: one for the
            // selected instance and one for the unselected instances. We do this so
            // that when point snapping we can snap selected instances to unselected
            // instances, without snapping to selected instances.

            // This code figures out which instances should be included in the current
            // render item, and which colors should be used to draw those instances.

            // Store info per instance
            const unsigned char dormant = 0;
            const unsigned char active = 1;
            const unsigned char lead = 2;
            const unsigned char invalid = 255;

            std::vector<unsigned char> instanceInfo;

            // depending on the type of render item we want to set different values
            // into instanceInfo;
            unsigned char modeDormant = invalid;
            unsigned char modeActive = invalid;
            unsigned char modeLead = invalid;

            if (!isHighlightItem) {
                stateToCommit._instanceColorParam = kDiffuseColorStr;
                if (!usingShadedSelectedInstanceItem) {
                    if (isShadedSelectedInstanceItem) {
                        modeDormant = invalid;
                        modeActive = invalid;
                        modeLead = invalid;
                    } else {
                        modeDormant = active;
                        modeActive = active;
                        modeLead = active;
                    }
                } else {
                    if (isShadedSelectedInstanceItem) {
                        modeDormant = invalid;
                        modeActive = active;
                        modeLead = active;
                    } else {
                        modeDormant = active;
                        modeActive = invalid;
                        modeLead = invalid;
                    }
                }
            } else if (_selectionStatus == kFullyLead || _selectionStatus == kFullyActive) {
                modeDormant = _selectionStatus == kFullyLead ? lead : active;
                stateToCommit._instanceColorParam = kSolidColorStr;
            } else {
                modeDormant = inPureSelectionHighlightMode ? invalid : dormant;
                modeActive = active;
                modeLead = lead;
                stateToCommit._instanceColorParam = kSolidColorStr;
            }

            // Assign with the dormant info by default. For non-selection
            // items the default value won't be draw, for wireframe items
            // this will correspond to drawing with the dormant wireframe color
            // or not drawing if the item is a selection highlight item.
            instanceInfo.resize(instanceCount, modeDormant);

            // Sometimes the calls to GetActiveSelectionState and GetLeadSelectionState
            // return instance indices which do not match the current selection, and that
            // causes incorrect drawing. Only call GetActiveSelectionState and GetLeadSelectionState
            // when _selectionStatus is kPartiallySelected. If the object is fully lead or active
            // then we already have the correct values in instanceInfo.
            if (_selectionStatus == kPartiallySelected) {
                // Assign with the index to the active selection highlight color.
                if (const auto state = drawScene.GetActiveSelectionState(id)) {
                    for (const auto& indexArray : state->instanceIndices) {
                        for (const auto index : indexArray) {
                            // This bounds check is necessary because of Pixar USD Issue 1516
                            // Logged as MAYA-113682
                            if (index >= 0 && index < (const int)instanceCount) {
                                instanceInfo[index] = modeActive;
                            }
                        }
                    }
                }

                // Assign with the index to the lead selection highlight color.
                if (const auto state = drawScene.GetLeadSelectionState(id)) {
                    for (const auto& indexArray : state->instanceIndices) {
                        for (const auto index : indexArray) {
                            // This bounds check is necessary because of Pixar USD Issue 1516
                            // Logged as MAYA-113682
                            if (index >= 0 && index < (const int)instanceCount) {
                                instanceInfo[index] = modeLead;
                            }
                        }
                    }
                }
            }

            // Now instanceInfo is set up correctly to tell us which instances are a part of this
            // render item.

            // Set up the source color buffers.
            const MColor wireframeColors[]
                = { drawScene.GetWireframeColor(),
                    drawScene.GetSelectionHighlightColor(HdPrimTypeTokens->mesh),
                    drawScene.GetSelectionHighlightColor() };
            bool useWireframeColors = stateToCommit._instanceColorParam == kSolidColorStr;

            MFloatArray*    shadedColors = nullptr;
            HdInterpolation colorInterpolation = HdInterpolationConstant;
            for (auto& entry : _meshSharedData->_primvarInfo) {
                const TfToken& primvarName = entry.first;
                if (primvarName == HdVP2Tokens->displayColorAndOpacity) {

                    colorInterpolation = entry.second->_source.interpolation;

                    if (colorInterpolation == HdInterpolationInstance) {
                        shadedColors = &entry.second->_extraInstanceData;
                        TF_VERIFY(shadedColors->length() == instanceCount * kNumColorChannels);
                    }
                }
            }

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
            // Create & fill the per-instance data buffers: the transform buffer, the color buffer
            // and the Maya instance id to usd instance id mapping buffer.
            InstanceIdMap mayaToUsd;
#endif
#ifdef MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT
            // Mark the Ufe Identifiers on the item dirty. The next time isolate select
            // updates the Ufe Identifiers will be updated.
            MayaUsdCustomData::ItemDataDirty(*renderItem, true);

            InstancePrimPaths& instancePrimPaths = MayaUsdCustomData::GetInstancePrimPaths(GetId());

            // The code to invalidate the instancePrimPaths is incomplete. If we had an instance
            // added and another instance removed between two calls to Sync, then the instanceCount
            // will match the cached path count, and the cache won't be invalidated. None of the
            // dirty information I get get out of the instancer seems correct, so I'll use this best
            // effort version for now, while I wait for a USD side fix.
            if (instanceCount != instancePrimPaths.size()) {
                instancePrimPaths.clear();
                instancePrimPaths.resize(instanceCount);
            }
#endif

            stateToCommit._instanceTransforms = std::make_shared<MMatrixArray>();
            stateToCommit._instanceColors = std::make_shared<MFloatArray>();
            for (unsigned int usdInstanceId = 0; usdInstanceId < instanceCount; usdInstanceId++) {
                unsigned char info = instanceInfo[usdInstanceId];
                if (info == invalid)
                    continue;
#ifndef MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT
                stateToCommit._ufeIdentifiers.append(
                    drawScene.GetScenePrimPath(GetId(), usdInstanceId).GetString().c_str());
#endif
                transforms[usdInstanceId].Get(instanceMatrix.matrix);
                stateToCommit._instanceTransforms->append(worldMatrix * instanceMatrix);
#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
                mayaToUsd.push_back(usdInstanceId);
#endif
                if (useWireframeColors) {
                    const MColor& color = wireframeColors[info];
                    for (unsigned int j = 0; j < kNumColorChannels; j++) {
                        stateToCommit._instanceColors->append(color[j]);
                    }
                } else if (shadedColors) {
                    unsigned int offset = usdInstanceId * kNumColorChannels;
                    for (unsigned int j = 0; j < kNumColorChannels; j++) {
                        stateToCommit._instanceColors->append((*shadedColors)[offset + j]);
                    }
                }
            }
#ifdef MAYA_UPDATE_UFE_IDENTIFIER_SUPPORT
            InstanceIdMap& cachedMayaToUsd = MayaUsdCustomData::Get(*renderItem);
            bool           mayaToUsdChanged = cachedMayaToUsd.size() != mayaToUsd.size();
            for (unsigned int i = 0; !mayaToUsdChanged && i < mayaToUsd.size(); i++) {
                mayaToUsdChanged = cachedMayaToUsd[i] != mayaToUsd[i];
            }

            if (mayaToUsdChanged && drawScene.ufeIdentifiersInUse()) {
                unsigned int mayaInstanceCount = mayaToUsd.size();
                for (unsigned int mayaInstanceId = 0; mayaInstanceId < mayaInstanceCount;
                     mayaInstanceId++) {
                    unsigned int usdInstanceId = mayaToUsd[mayaInstanceId];
                    // try making a cache of the USD ID to the ufeIdentifier.
                    if (instancePrimPaths[usdInstanceId] == SdfPath()) {
                        instancePrimPaths[usdInstanceId]
                            = drawScene.GetScenePrimPath(GetId(), usdInstanceId);
                    }
                    stateToCommit._ufeIdentifiers.append(
                        instancePrimPaths[usdInstanceId].GetString().c_str());
                }
            }
            cachedMayaToUsd = std::move(mayaToUsd);
#else
            TF_VERIFY(
                stateToCommit._ufeIdentifiers.length()
                == stateToCommit._instanceTransforms->length());
#endif
            if (stateToCommit._instanceTransforms->length() == 0)
                instancerWithNoInstances = true;
        }

        // compare the new _instanceTransforms on stateToCommit to
        // the existing instance transforms (if any) on drawItemData
        bool instanceTransformsChanged = static_cast<bool>(stateToCommit._instanceTransforms)
            ? !static_cast<bool>(drawItemData._instanceTransforms)
            : static_cast<bool>(drawItemData._instanceTransforms);
        if (stateToCommit._instanceTransforms && drawItemData._instanceTransforms) {
            instanceTransformsChanged
                = (stateToCommit._instanceTransforms->length()
                   != drawItemData._instanceTransforms->length());
            for (unsigned int index = 0;
                 index < stateToCommit._instanceTransforms->length() && !instanceTransformsChanged;
                 index++) {
                instanceTransformsChanged
                    = ((*stateToCommit._instanceTransforms)[index]
                       != (*drawItemData._instanceTransforms)[index]);
            }
        }
        // if the values are the same then there is nothing to do. Don't update
        // the instance transforms and keep on drawing with the current transforms
        if (!instanceTransformsChanged) {
            stateToCommit._instanceTransforms.reset();
        } else {
            drawItemData._instanceTransforms = stateToCommit._instanceTransforms;
        }

        // compate the new _instanceColors on stateToCommit to
        // the existing instance colors (if any) on drawItemData
        bool instanceColorsChanged = static_cast<bool>(stateToCommit._instanceColors)
            ? !static_cast<bool>(drawItemData._instanceColors)
            : static_cast<bool>(drawItemData._instanceColors); // XOR
        if (stateToCommit._instanceColors && drawItemData._instanceColors) {
            instanceColorsChanged
                = stateToCommit._instanceColors->length() != drawItemData._instanceColors->length();
            for (unsigned int i = 0;
                 i < drawItemData._instanceColors->length() && !instanceColorsChanged;
                 i++) {
                instanceColorsChanged
                    = (*drawItemData._instanceColors)[i] != (*stateToCommit._instanceColors)[i];
            }
        }
        // if the colors haven't changed then there is nothing to do. Don't update
        // the instance colors and keep on drawing the current colors
        if (!instanceColorsChanged) {
            stateToCommit._instanceColors.reset();
        } else {
            drawItemData._instanceColors = stateToCommit._instanceColors;
        }

    } else {
        // Non-instanced Rprims.
        if ((itemDirtyBits & DirtySelectionHighlight) && isHighlightItem) {
            MColor                      color = _GetHighlightColor(HdPrimTypeTokens->mesh);
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

        if (inPureSelectionHighlightMode) {
            enable = enable && (_selectionStatus != kUnselected);
        } else if (isPointSnappingItem) {
            enable = enable && (_selectionStatus == kUnselected);
        } else if (isBBoxItem) {
            enable = enable && !range.IsEmpty();
        }

        if (inTemplateMode) {
            enable = enable && isHighlightItem;
        } else if (inReferenceMode) {
            enable = enable && !isPointSnappingItem;
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

    // Some items may require selection mask overrides
    if (!isDedicatedHighlightItem && !isPointSnappingItem
        && (itemDirtyBits & (DirtySelectionHighlight | DirtySelectionMode))) {
        MSelectionMask selectionMask(MSelectionMask::kSelectMeshes);

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
        if (!isBBoxItem) {
            bool shadedUnselectedInstances
                = !isShadedSelectedInstanceItem && !GetInstancerId().IsEmpty();
            if (_selectionStatus == kUnselected || drawScene.SnapToSelectedObjects()
                || shadedUnselectedInstances) {
                selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
            }
            // Only unselected Rprims can be used for point snapping.
            if (_selectionStatus == kUnselected && !shadedUnselectedInstances) {
                selectionMask.addMask(MSelectionMask::kSelectPointsForGravity);
            }
        }
#endif
        // In template and reference modes, items should have no selection
        if (inTemplateMode || inReferenceMode) {
            selectionMask = MSelectionMask();
        }

        // The function is thread-safe, thus called in place to keep simple.
        renderItem->setSelectionMask(selectionMask);
    }

    // Capture buffers we need
    MHWRender::MIndexBuffer* indexBuffer = drawItemData._indexBuffer.get();
    PrimvarInfoMap*          primvarInfo = &_meshSharedData->_primvarInfo;
    TfTokenVector*           primvars = &_meshSharedData->_allRequiredPrimvars;
    const HdVP2BBoxGeom&     sharedBBoxGeom = _delegate->GetSharedBBoxGeom();
    if (isBBoxItem) {
        indexBuffer = const_cast<MHWRender::MIndexBuffer*>(sharedBBoxGeom.GetIndexBuffer());
    }

    // We can get an empty stateToCommit when viewport draw modes change. In this case every
    // rprim is marked dirty to give any stale render items a chance to update. If there are
    // no stale render items then stateToCommit can be empty!
    if (!stateToCommit.Empty()) {
        _delegate->GetVP2ResourceRegistry().EnqueueCommit([stateToCommit,
                                                           param,
                                                           primvarInfo,
                                                           primvars,
                                                           indexBuffer,
                                                           isBBoxItem,
                                                           &sharedBBoxGeom]() {
            // This code executes serially, once per mesh updated. Keep
            // performance in mind while modifying this code.
            const HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;
            MHWRender::MRenderItem*              renderItem = drawItemData._renderItem;
            if (ARCH_UNLIKELY(!renderItem))
                return;

            MStatus result;

            // If available, something changed
            if (stateToCommit._indexBufferData)
                indexBuffer->commit(stateToCommit._indexBufferData);

            // If available, something changed
            if (stateToCommit._shader != nullptr) {
                bool success = renderItem->setShader(stateToCommit._shader);
                TF_VERIFY(success);
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

                std::set<TfToken> addedPrimvars;
                auto              addPrimvar =
                    [primvarInfo, &vertexBuffers, &addedPrimvars, isBBoxItem, &sharedBBoxGeom](
                        const TfToken& p) {
                        auto entry = primvarInfo->find(p);
                        if (entry == primvarInfo->cend()) {
                            // No primvar by that name.
                            return;
                        }
                        MHWRender::MVertexBuffer* primvarBuffer = nullptr;
                        if (isBBoxItem && p == HdTokens->points) {
                            primvarBuffer = const_cast<MHWRender::MVertexBuffer*>(
                                sharedBBoxGeom.GetPositionBuffer());
                        } else {
                            primvarBuffer = entry->second->_buffer.get();
                        }
                        if (primvarBuffer) { // this filters out the separate color & alpha entries
                            MStatus result = vertexBuffers.addBuffer(p.GetText(), primvarBuffer);
                            TF_VERIFY(result == MStatus::kSuccess);
                        }
                        addedPrimvars.insert(p);
                    };

                // Points and normals always are at the beginning of vertex requirements:
                addPrimvar(HdTokens->points);
                addPrimvar(HdTokens->normals);
                // Then add required primvars *in order*:
                if (primvars) {
                    for (const TfToken& primvarName : *primvars) {
                        if (addedPrimvars.find(primvarName) == addedPrimvars.cend()) {
                            addPrimvar(primvarName);
                        }
                    }
                }
                // Then add whatever primvar is left that was not in the requirements:
                for (auto& entry : *primvarInfo) {
                    if (addedPrimvars.find(entry.first) == addedPrimvars.cend()) {
                        addPrimvar(entry.first);
                    }
                }

                // The API call does three things:
                // - Associate geometric buffers with the render item.
                // - Update bounding box.
                // - Trigger consolidation/instancing update.
                result = drawScene.setGeometryForRenderItem(
                    *renderItem, vertexBuffers, *indexBuffer, stateToCommit._boundingBox);
                TF_VERIFY(result == MStatus::kSuccess);
            }

            // Important, update instance transforms after setting geometry on render items!
            auto& oldInstanceCount = stateToCommit._renderItemData._instanceCount;
            auto  newInstanceCount = stateToCommit._instanceTransforms
                ? stateToCommit._instanceTransforms->length()
                : oldInstanceCount;

            // GPU instancing has been enabled. We cannot switch to consolidation
            // without recreating render item, so we keep using GPU instancing.
            if (stateToCommit._renderItemData._usingInstancedDraw) {
                if (stateToCommit._instanceTransforms) {
                    if (oldInstanceCount == newInstanceCount) {
                        for (unsigned int i = 0; i < newInstanceCount; i++) {
                            // VP2 defines instance ID of the first instance to be 1.
                            result = drawScene.updateInstanceTransform(
                                *renderItem, i + 1, (*stateToCommit._instanceTransforms)[i]);
                            TF_VERIFY(result == MStatus::kSuccess);
                        }
                    } else {
                        result = drawScene.setInstanceTransformArray(
                            *renderItem, *stateToCommit._instanceTransforms);
                        TF_VERIFY(result == MStatus::kSuccess);
                    }
                }

                if (stateToCommit._instanceColors && stateToCommit._instanceColors->length() > 0) {
                    TF_VERIFY(
                        newInstanceCount * kNumColorChannels
                        == stateToCommit._instanceColors->length());
                    result = drawScene.setExtraInstanceData(
                        *renderItem,
                        stateToCommit._instanceColorParam,
                        *stateToCommit._instanceColors);
                    TF_VERIFY(result == MStatus::kSuccess);
                }
            }
#if MAYA_API_VERSION >= 20210000
            else if (newInstanceCount >= 1) {
#else
            // In Maya 2020 and before, GPU instancing and consolidation are two separate
            // systems that cannot be used by a render item at the same time. In case of single
            // instance, we keep the original render item to allow consolidation with other
            // prims. In case of multiple instances, we need to disable consolidation to allow
            // GPU instancing to be used.
            else if (newInstanceCount == 1) {
                bool success = renderItem->setMatrix(&(*stateToCommit._instanceTransforms)[0]);
                TF_VERIFY(success);
            } else if (newInstanceCount > 1) {
                _SetWantConsolidation(*renderItem, false);
#endif
                if (stateToCommit._instanceTransforms) {
                    result = drawScene.setInstanceTransformArray(
                        *renderItem, *stateToCommit._instanceTransforms);
                    TF_VERIFY(result == MStatus::kSuccess);
                }

                if (stateToCommit._instanceColors && stateToCommit._instanceColors->length() > 0) {
                    TF_VERIFY(
                        newInstanceCount * kNumColorChannels
                        == stateToCommit._instanceColors->length());
                    result = drawScene.setExtraInstanceData(
                        *renderItem,
                        stateToCommit._instanceColorParam,
                        *stateToCommit._instanceColors);
                    TF_VERIFY(result == MStatus::kSuccess);
                }

                stateToCommit._renderItemData._usingInstancedDraw = true;
            } else if (stateToCommit._worldMatrix != nullptr) {
                // Regular non-instanced prims. Consolidation has been turned on by
                // default and will be kept enabled on this case.
                bool success = renderItem->setMatrix(stateToCommit._worldMatrix);
                TF_VERIFY(success);
            }

            if (stateToCommit._instanceTransforms) {
                oldInstanceCount = newInstanceCount;
            }
#ifdef MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT
            if (stateToCommit._ufeIdentifiers.length() > 0) {
                drawScene.setUfeIdentifiers(*renderItem, stateToCommit._ufeIdentifiers);
            }
#endif
        });
    }

    // Reset dirty bits because we've prepared commit state for this render item.
    renderItemData.ResetDirtyBits();
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
    MProfilingScope profilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorC_L2,
        _rprimId.asChar(),
        "HdVP2Mesh::_UpdatePrimvarSources");

    const SdfPath& id = GetId();

    ErasePrimvarInfoFunc erasePrimvarInfo
        = [this](const TfToken& name) { _meshSharedData->_primvarInfo.erase(name); };

    UpdatePrimvarInfoFunc updatePrimvarInfo
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

              // if the primvar color changes then we might need to use a different fallback
              // material
              if (interpolation == HdInterpolationConstant && name == HdTokens->displayColor) {
                  // find all the smooth hull render items and mark their _fallbackColorDirty true
                  for (const std::pair<TfToken, HdReprSharedPtr>& pair : _reprs) {

                      _MeshReprConfig::DescArray reprDescs = _GetReprDesc(pair.first);
                      // Iterate through all reprdescs for the current repr to figure out if
                      // any of them requires the fallback material
                      for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
                          const HdMeshReprDesc& desc = reprDescs[descIdx];
                          if (desc.geomStyle == HdMeshGeomStyleHull) {
                              RenderItemFunc renderItemFunc
                                  = [](HdVP2DrawItem::RenderItemData& renderItemData) {
                                        renderItemData._fallbackColorDirty = true;
                                    };

                              _ForEachRenderItemInRepr(pair.second, renderItemFunc);
                          }
                      }
                  }
              }
          };

    _UpdatePrimvarSourcesGeneric(
        sceneDelegate, dirtyBits, requiredPrimvars, *this, updatePrimvarInfo, erasePrimvarInfo);

    // At this point we've searched the primvars for the required primvars.
    // check to see if there are any HdExtComputation which should replace
    // primvar data or fill in for a missing primvar.
    HdExtComputationPrimvarDescriptorVector compPrimvars
        = sceneDelegate->GetExtComputationPrimvarDescriptors(id, HdInterpolationVertex);
    const HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();
    bool                 pointsAreComputed = false;
    for (const auto& primvarName : requiredPrimvars) {
        // The compPrimvars are a description of the link between the compute system and
        // what we need to draw.
        auto result
            = std::find_if(compPrimvars.begin(), compPrimvars.end(), [&](const auto& compPrimvar) {
                  return compPrimvar.name == primvarName;
              });
        // if there is no compute for the given required primvar then we're done!
        if (result == compPrimvars.end())
            continue;
        HdExtComputationPrimvarDescriptor compPrimvar = *result;
        // Create the HdExtCompCpuComputation objects necessary to resolve the computation
        HdExtComputation const* sourceComp
            = static_cast<HdExtComputation const*>(renderIndex.GetSprim(
                HdPrimTypeTokens->extComputation, compPrimvar.sourceComputationId));
        if (!sourceComp || sourceComp->GetElementCount() <= 0)
            continue;

        // This compPrimvar is telling me that the primvar with "name" comes from compute.
        // The compPrimvar has the Id of the compute the data comes from, and the output
        // of the compute which contains the data
        HdExtCompCpuComputationSharedPtr cpuComputation;
        HdBufferSourceSharedPtrVector    sources;
        // There is a possible data race calling CreateComputation, see
        // https://github.com/PixarAnimationStudios/USD/issues/1742
        cpuComputation
            = HdExtCompCpuComputation::CreateComputation(sceneDelegate, *sourceComp, &sources);

        // Immediately resolve the computation so we can fill _meshSharedData._primvarInfo
        for (HdBufferSourceSharedPtr& source : sources) {
            source->Resolve();
        }

        // Pull the result out of the compute and save it into our local primvar info.
        size_t outputIndex
            = cpuComputation->GetOutputIndex(compPrimvar.sourceComputationOutputName);
        // INVALID_OUTPUT_INDEX is declared static in USD, can't access here so re-declare
        constexpr size_t INVALID_OUTPUT_INDEX = std::numeric_limits<size_t>::max();
        if (INVALID_OUTPUT_INDEX != outputIndex) {
            updatePrimvarInfo(
                primvarName, cpuComputation->GetOutputByIndex(outputIndex), HdInterpolationVertex);
        }

        // Records that points primvar is computed.
        if (primvarName == HdTokens->points) {
            pointsAreComputed = true;
        }
    }

    // When points are computed then we will have to propagate that fact to the function
    // _PropagateDirtyBits() so that it can mark points dirty when the transform change.
    // This support UsdSkel affecting the points position and properly making the render
    // delegate dirty.
    _pointsFromSkel = pointsAreComputed;
}

#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
MHWRender::MRenderItem* HdVP2Mesh::_CreateShadedSelectedInstancesItem(
    const MString&      name,
    HdVP2DrawItem&      drawItem,
    MSubSceneContainer& subSceneContainer,
    const HdGeomSubset* geomSubset) const
{
    MString ssiName = name;
    ssiName += std::string(1, VP2_RENDER_DELEGATE_SEPARATOR).c_str();
    ssiName += "shadedSelectedInstances";
    HdVP2DrawItem::RenderItemData& renderItemData
        = _CreateSmoothHullRenderItem(ssiName, drawItem, subSceneContainer, geomSubset);
    renderItemData._shadedSelectedInstances = true;

    return renderItemData._renderItem;
}
#endif

/*! \brief  Create render item for smoothHull repr.
 */
HdVP2DrawItem::RenderItemData& HdVP2Mesh::_CreateSmoothHullRenderItem(
    const MString&      name,
    HdVP2DrawItem&      drawItem,
    MSubSceneContainer& subSceneContainer,
    const HdGeomSubset* geomSubset) const
{
    MString itemName = name;
    if (geomSubset) {
        itemName += std::string(1, VP2_RENDER_DELEGATE_SEPARATOR).c_str();
        itemName += geomSubset->id.GetString().c_str();
    }

    MHWRender::MRenderItem* const renderItem = MHWRender::MRenderItem::Create(
        itemName, MHWRender::MRenderItem::MaterialSceneItem, MHWRender::MGeometry::kTriangles);

    constexpr MHWRender::MGeometry::DrawMode drawMode = static_cast<MHWRender::MGeometry::DrawMode>(
        MHWRender::MGeometry::kShaded | MHWRender::MGeometry::kTextured);
    renderItem->setDrawMode(drawMode);
    renderItem->setExcludedFromPostEffects(false);
    renderItem->castsShadows(true);
    renderItem->receivesShadows(true);
    renderItem->setShader(_delegate->GetFallbackShader(kOpaqueGray));
    _InitRenderItemCommon(renderItem);

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

    return _AddRenderItem(drawItem, renderItem, subSceneContainer, geomSubset);
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
    _InitRenderItemCommon(renderItem);

#if MAYA_API_VERSION >= 20220000
    renderItem->setObjectTypeExclusionFlag(MHWRender::MFrameContext::kExcludeMeshes);
#endif

    return renderItem;
}

PXR_NAMESPACE_CLOSE_SCOPE
