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

#include "meshViewportCompute.h"

#include "mesh.h"
#include "render_delegate.h"

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <pxr/imaging/hd/vertexAdjacency.h>
#include <pxr/imaging/pxOsd/refinerFactory.h>
#include <pxr/imaging/pxOsd/tokens.h>

#include <maya/MProfiler.h>

#ifdef HDVP2_ENABLE_GPU_COMPUTE

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::string _GetResourcePath(const std::string& resource)
{
    static PlugPluginPtr plugin
        = PlugRegistry::GetInstance().GetPluginWithName("mayaUsd_ComputeShaders");
    if (!TF_VERIFY(plugin, "Could not get plugin\n")) {
        return std::string();
    }

    const std::string path = PlugFindPluginResource(plugin, resource);
    TF_VERIFY(!path.empty(), "Cound not find resource: %s\n", resource.c_str());

    return path;
}

template <typename F> class LambdaTask : public tbb::task
{
public:
    LambdaTask(const F& func)
        : _func(func)
    {
    }

private:
    tbb::task* execute()
    {
        _func();
        return nullptr;
    }

    F _func;
};

template <typename F> void EnqueueLambdaTask(const F& f)
{
    tbb::task::enqueue(*new (tbb::task::allocate_root()) LambdaTask<F>(f));
}
} // namespace

std::once_flag      MeshViewportCompute::_compileProgramOnce;
PxrMayaGLSLProgram* MeshViewportCompute::_computeNormalsProgram;

bool MeshViewportCompute::verifyDrawItem(const HdVP2DrawItem& drawItem) const
{
    return &drawItem == _drawItem;
}

void MeshViewportCompute::openGLErrorCheck()
{
//#define DO_OPENGL_ERROR_CHECK
#ifdef DO_OPENGL_ERROR_CHECK
    glFinish();
    // Check for errors.
    GLenum err = glGetError();
    TF_VERIFY(GL_NO_ERROR == err);
#endif
}

void MeshViewportCompute::setTopologyDirty()
{
    _topologyDirty = true;
    _executed = false;
    _vertexCount = 0;
    if (_consolidatedCompute) {
        _consolidatedCompute->_topologyDirty = true;
        _consolidatedCompute->_vertexCount = 0;
    }
}

void MeshViewportCompute::setAdjacencyBufferGPUDirty()
{
    _adjacencyBufferGPUDirty = true;
    _executed = false;
    if (_consolidatedCompute)
        _consolidatedCompute->_adjacencyBufferGPUDirty = true;
}

void MeshViewportCompute::setNormalVertexBufferGPUDirty()
{
    _normalVertexBufferGPUDirty = true;
    _executed = false;
    if (_consolidatedCompute)
        _consolidatedCompute->_normalVertexBufferGPUDirty = true;
}

void MeshViewportCompute::reset()
{
    /*  don't clear _meshSharedData, it's either an input from the external HdVP2Mesh
        or it has been created explicitly for this consolidated viewport compute.
    */
    _drawItem = nullptr;
    _executed = false;
    _sourcesExecuted = false;

    _consolidatedCompute.reset();
    _geometryIndexMapping.reset();
    _vertexCount = 0;

    _adjacencyBufferSize = 0;
    _adjacencyBufferCPU.reset();
    _adjacencyBufferGPU.reset();
    _renderingToSceneFaceVtxIdsGPU.reset();
    _sceneToRenderingFaceVtxIdsGPU.reset();

    fRenderGeom = nullptr;

    _positionVertexBufferGPU = nullptr;
    _normalVertexBufferGPU = nullptr;
    _colorVertexBufferGPU = nullptr;

    _topologyDirty = true;
    _adjacencyBufferGPUDirty = true;
    _normalVertexBufferGPUDirty = true;

#if defined(DO_CPU_OSD) || defined(DO_OPENGL_OSD)
    // OSD information
    _vertexStencils.reset();
    _varyingStencils.reset();
    _patchTable.reset();
#endif
}

bool MeshViewportCompute::hasExecuted() const { return _executed; }

void MeshViewportCompute::findConsolidationMapping(MRenderItem& renderItem)
{
    // If the item is not consolidated clear any stale consolidatedCompute information
    if (!renderItem.isConsolidated()) {
        _consolidatedCompute.reset();
        _sourcesExecuted = false;
        return;
    }

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:MGeometryIndexMapping");

    if (_geometryIndexMapping) {
        if (renderItem.isSourceIndexMappingValid(*_geometryIndexMapping.get()))
            return;
    }

    reset();
    _geometryIndexMapping.reset(new MHWRender::MGeometryIndexMapping());
    renderItem.sourceIndexMapping(*_geometryIndexMapping.get());

    for (int i = 0; i < _geometryIndexMapping->geometryCount(); i++) {
        MRenderItem*                    sourceItem = _geometryIndexMapping->sourceRenderItem(i);
        MSharedPtr<MeshViewportCompute> sourceViewportComputeItem
            = MSharedPtr<MeshViewportCompute>::dynamic_pointer_cast<>(
                sourceItem->viewportComputeItem());
        sourceViewportComputeItem->_consolidatedCompute
            = MSharedPtr<MeshViewportCompute>::dynamic_pointer_cast<>(
                renderItem.viewportComputeItem());
        TF_VERIFY(i == 0 || _sourcesExecuted == sourceViewportComputeItem->hasExecuted());
        _sourcesExecuted = sourceViewportComputeItem->hasExecuted();
    }
}

HdMeshTopology& getSceneTopology(HdVP2MeshSharedData* meshSharedData)
{
    return meshSharedData->_topology;
}

HdMeshTopology& getRenderingTopology(HdVP2MeshSharedData* meshSharedData)
{
    return meshSharedData->_renderingTopology;
}

template <typename TopologyAccessor>
void MeshViewportCompute::createConsolidatedTopology(TopologyAccessor getTopology)
{
    if (!_topologyDirty && getTopology(_meshSharedData.get()).GetNumPoints() > 0)
        return;
    _topologyDirty = false;

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:createConsolidatedTopology");

    size_t faceVertexCountsSize = 0;
    size_t faceVertexIndicesSize = 0;
    size_t holeIndicesSize = 0;
    size_t sceneToRenderingFaceVtxIdsCount = 0;
    size_t vertexCount = 0;

    // figure out the size of the consolidated mesh topology
    if (_geometryIndexMapping.get()) {
        for (int i = 0; i < _geometryIndexMapping->geometryCount(); i++) {
            MRenderItem* sourceItem = _geometryIndexMapping->sourceRenderItem(i);

            MSharedPtr<MeshViewportCompute> sourceViewportComputeItem
                = MSharedPtr<MeshViewportCompute>::dynamic_pointer_cast<>(
                    sourceItem->viewportComputeItem());
            HdVP2MeshSharedData* sourceMeshSharedData
                = sourceViewportComputeItem->_meshSharedData.get();
            TF_VERIFY(sourceMeshSharedData);
            HdMeshTopology& sourceTopology = getTopology(sourceMeshSharedData);

            faceVertexCountsSize += sourceTopology.GetNumFaces();
            faceVertexIndicesSize += sourceTopology.GetFaceVertexIndices().size();
            holeIndicesSize += sourceTopology.GetHoleIndices().size();
            // if there is a sourceMeshSharedData it should have entries for every vertex in that
            // geometry source.
            vertexCount += sourceTopology.GetNumPoints();
            sceneToRenderingFaceVtxIdsCount += sourceTopology.GetNumPoints();
        }
    }

    TF_VERIFY(faceVertexCountsSize > 0 || !_geometryIndexMapping.get());

    // check to see if it is actually a consolidated geometry that needs a consolidated adjacency
    // buffer
    if (faceVertexCountsSize > 0) {
        TfToken    scheme = PxOsdOpenSubdivTokens->catmullClark;
        TfToken    orientation = PxOsdOpenSubdivTokens->rightHanded;
        VtIntArray faceVertexCounts;
        VtIntArray faceVertexIndices;
        VtIntArray holeIndices;
        int        refineLevel = 0;

        faceVertexCounts.reserve(faceVertexCountsSize);
        faceVertexIndices.reserve(faceVertexIndicesSize);
        holeIndices.reserve(holeIndicesSize);
        _meshSharedData->_renderingToSceneFaceVtxIds.clear();
        _meshSharedData->_renderingToSceneFaceVtxIds.reserve(vertexCount);
        _meshSharedData->_sceneToRenderingFaceVtxIds.clear();
        _meshSharedData->_sceneToRenderingFaceVtxIds.reserve(sceneToRenderingFaceVtxIdsCount);

        for (int sourceIndex = 0; sourceIndex < _geometryIndexMapping->geometryCount();
             sourceIndex++) {
            MRenderItem* sourceItem = _geometryIndexMapping->sourceRenderItem(sourceIndex);
            int          vertexStart = _geometryIndexMapping->vertexStart(sourceIndex);
            TF_VERIFY(vertexStart >= 0);
            size_t                          consolidatedBufferVertexOffset = (size_t)vertexStart;
            MSharedPtr<MeshViewportCompute> sourceViewportComputeItem
                = MSharedPtr<MeshViewportCompute>::dynamic_pointer_cast<>(
                    sourceItem->viewportComputeItem());
            HdVP2MeshSharedData* sourceMeshSharedData
                = sourceViewportComputeItem->_meshSharedData.get();
            TF_VERIFY(sourceMeshSharedData);

            HdMeshTopology& sourceTopology = getTopology(sourceMeshSharedData);

            TF_VERIFY(sourceTopology.GetScheme() == scheme);
            TF_VERIFY(sourceTopology.GetOrientation() == orientation);
            TF_VERIFY(sourceTopology.GetRefineLevel() == refineLevel);

            VtIntArray const& sourceFaceVertexCounts = sourceTopology.GetFaceVertexCounts();
            for (size_t faceId = 0; faceId < sourceFaceVertexCounts.size(); faceId++) {
                faceVertexCounts.push_back(sourceFaceVertexCounts[faceId]);
            }

            VtIntArray const& sourceFaceVertexIndices = sourceTopology.GetFaceVertexIndices();
            for (size_t faceVertexId = 0; faceVertexId < sourceFaceVertexIndices.size();
                 faceVertexId++) {
                faceVertexIndices.push_back(
                    sourceFaceVertexIndices[faceVertexId] + consolidatedBufferVertexOffset);
            }

            VtIntArray const& sourceHoleIndices = sourceTopology.GetHoleIndices();
            for (size_t faceId = 0; faceId < sourceHoleIndices.size(); faceId++) {
                holeIndices.push_back(
                    sourceHoleIndices[faceId] + consolidatedBufferVertexOffset); // untested?
            }

            for (size_t idx = 0; idx < sourceMeshSharedData->_renderingToSceneFaceVtxIds.size();
                 idx++) {
                _meshSharedData->_renderingToSceneFaceVtxIds.push_back(
                    sourceMeshSharedData->_renderingToSceneFaceVtxIds[idx]
                    + consolidatedBufferVertexOffset);
            }

            // add padding to _sceneToRenderingFaceVtxIds because the scene IDs start at
            // consolidatedBufferVertexOffset
            while (consolidatedBufferVertexOffset
                   > _meshSharedData->_sceneToRenderingFaceVtxIds.size()) {
                _meshSharedData->_sceneToRenderingFaceVtxIds.push_back(-1);
            }

            for (size_t idx = 0; idx < sourceMeshSharedData->_sceneToRenderingFaceVtxIds.size();
                 idx++) {
                _meshSharedData->_sceneToRenderingFaceVtxIds.push_back(
                    sourceMeshSharedData->_sceneToRenderingFaceVtxIds[idx]
                    + consolidatedBufferVertexOffset);
            }
        }

        HdMeshTopology consolidatedTopology(
            scheme, orientation, faceVertexCounts, faceVertexIndices, holeIndices, refineLevel);
        getTopology(_meshSharedData.get()) = consolidatedTopology;
    } else {
        // It is not a consolidated draw item. There is only a single topology so we can use that
        // directly In fact it is already there, nothing to do!
        vertexCount = _meshSharedData->_renderingTopology.GetNumPoints();

        // the non-consolidated topology doesn't get updated with _sceneToRenderingFaceVtxIds, I
        // guess we have to do it in the kernel? Or have different storage for a topology we modify.

        // Can't modify _meshSharedData if we are not consolidated!
    }

    TF_VERIFY(vertexCount == 0 || vertexCount == _vertexCount);
    _vertexCount = vertexCount;
}

void MeshViewportCompute::createConsolidatedAdjacency()
{
    if (_adjacencyBufferSize > 0)
        return;

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:createConsolidatedAdjacency");

    Hd_VertexAdjacencySharedPtr adjacency(new Hd_VertexAdjacency());
    HdBufferSourceSharedPtr     adjacencyComputation
        = adjacency->GetSharedAdjacencyBuilderComputation(&_meshSharedData->_topology);
    adjacencyComputation->Resolve();

    const VtIntArray& adjacencyTable = adjacency->GetAdjacencyTable();
    size_t            adjacencyBufferSize = adjacencyTable.size();
    int*              adjCopy = new int[adjacencyBufferSize];
    memcpy(adjCopy, adjacencyTable.data(), adjacencyBufferSize * sizeof(int));
    _adjacencyBufferCPU.reset(
        adjCopy); // make sure this is really using the const version and doesn't copy the data.
    _adjacencyBufferSize = adjacencyTable.size();
}

void MeshViewportCompute::findRenderGeometry(MRenderItem& renderItem)
{
    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:findRenderGeometry");

    MGeometry* renderGeometry = renderItem.geometry();
    if (fRenderGeom && (fRenderGeom != renderGeometry)) {
        _positionVertexBufferGPU = nullptr;
        _normalVertexBufferGPU = nullptr;
        _colorVertexBufferGPU = nullptr;
        fRenderGeom = nullptr;
    }
    fRenderGeom = renderGeometry;
}

void MeshViewportCompute::createConsolidatedOSDTables(MRenderItem& renderItem)
{
#if defined(DO_CPU_OSD) || defined(DO_OPENGL_OSD)

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:consolidatedOSDTables");

    // refine
    //  and
    // create stencil/patch table
    {
        OpenSubdiv::Far::StencilTable const* consolidatedVertexStencils = nullptr;
        OpenSubdiv::Far::StencilTable const* consolidatedVaryingStencils = nullptr;
        OpenSubdiv::Far::PatchTable const*   consolidatedPatchTable = nullptr;

        // if this is a consolidated item then we won't have any stencils or tables.
        // If this is an unconsolidated item then we'll already have the tables we need.
        if (!_vertexStencils || !_varyingStencils || !_patchTable) {
            MProfilingScope subsubProfilingScope(
                HdVP2RenderDelegate::sProfilerCategory,
                MProfiler::kColorD_L2,
                "MeshViewportCompute:createConsolidatedMeshTables");

            // create topology refiner
            PxOsdTopologyRefinerSharedPtr refiner;
            // for empty topology, we don't need to refine anything.
            // but still need to return the typed buffer for codegen
            if (_meshSharedData->_renderingTopology.GetFaceVertexCounts().size() == 0) {
                // leave refiner empty
            } else {
                refiner = PxOsdRefinerFactory::Create(
                    _meshSharedData->_renderingTopology.GetPxOsdMeshTopology(),
                    TfToken(_meshSharedData->_renderTag.GetText()));
            }

            if (refiner) {
                OpenSubdiv::Far::PatchTableFactory::Options patchOptions(_level);
                if (_adaptive) {
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
                    MProfilingScope subsubsubProfilingScope(
                        HdVP2RenderDelegate::sProfilerCategory,
                        MProfiler::kColorD_L2,
                        "MeshViewportCompute:refine");
                    if (_adaptive) {
                        OpenSubdiv::Far::TopologyRefiner::AdaptiveOptions adaptiveOptions(_level);
#if OPENSUBDIV_VERSION_NUMBER >= 30400
                        adaptiveOptions = patchOptions.GetRefineAdaptiveOptions();
#endif
                        refiner->RefineAdaptive(adaptiveOptions);
                    } else {
                        refiner->RefineUniform(_level);
                    }
                }
                {
                    MProfilingScope subsubsubProfilingScope(
                        HdVP2RenderDelegate::sProfilerCategory,
                        MProfiler::kColorD_L2,
                        "MeshViewportCompute:stencilFactory");
                    OpenSubdiv::Far::StencilTableFactory::Options options;
                    options.generateOffsets = true;
                    options.generateIntermediateLevels = _adaptive;
                    options.interpolationMode
                        = OpenSubdiv::Far::StencilTableFactory::INTERPOLATE_VERTEX;
                    consolidatedVertexStencils
                        = OpenSubdiv::Far::StencilTableFactory::Create(*refiner, options);

                    options.interpolationMode
                        = OpenSubdiv::Far::StencilTableFactory::INTERPOLATE_VARYING;
                    consolidatedVaryingStencils
                        = OpenSubdiv::Far::StencilTableFactory::Create(*refiner, options);
                }
                {
                    MProfilingScope subsubsubProfilingScope(
                        HdVP2RenderDelegate::sProfilerCategory,
                        MProfiler::kColorD_L2,
                        "MeshViewportCompute:patchFactory");
                    consolidatedPatchTable
                        = OpenSubdiv::Far::PatchTableFactory::Create(*refiner, patchOptions);
                }
            }

            // merge endcap
            if (consolidatedPatchTable && consolidatedPatchTable->GetLocalPointStencilTable()) {
                // append stencils
                if (OpenSubdiv::Far::StencilTable const* vertexStencilsWithLocalPoints
                    = OpenSubdiv::Far::StencilTableFactory::AppendLocalPointStencilTable(
                        *refiner,
                        consolidatedVertexStencils,
                        consolidatedPatchTable->GetLocalPointStencilTable())) {
                    delete consolidatedVertexStencils;
                    consolidatedVertexStencils = vertexStencilsWithLocalPoints;
                }
                if (OpenSubdiv::Far::StencilTable const* varyingStencilsWithLocalPoints
                    = OpenSubdiv::Far::StencilTableFactory::AppendLocalPointStencilTable(
                        *refiner,
                        consolidatedVaryingStencils,
                        consolidatedPatchTable->GetLocalPointStencilTable())) {
                    delete consolidatedVaryingStencils;
                    consolidatedVaryingStencils = varyingStencilsWithLocalPoints;
                }
            }

            // save tables to topology
            _vertexStencils.reset(consolidatedVertexStencils);
            _varyingStencils.reset(consolidatedVaryingStencils);
            _patchTable.reset(consolidatedPatchTable);
        }
    }

    if (_geometryIndexMapping && _geometryIndexMapping->geometryCount() > 0) {
        MProfilingScope subsubProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L2,
            "MeshViewportCompute:updateIndexMapping");

        // TODO: assumes quads
        int indexStart = 0;
        int vertexStart = 0;
        for (int i = 0; i < _geometryIndexMapping->geometryCount(); i++) {
            MRenderItem* sourceItem = _geometryIndexMapping->sourceRenderItem(i);

            // we can have a nullptr sourceMeshSharedData, but if we do all the sourceItems should
            // have a nullptr
            MSharedPtr<MeshViewportCompute> sourceViewportComputeItem
                = MSharedPtr<MeshViewportCompute>::dynamic_pointer_cast<>(
                    sourceItem->viewportComputeItem());
            size_t sourcePtableSize
                = sourceViewportComputeItem->_patchTable->GetPatchControlVerticesTable().size();
            size_t sourceBaseVertexCount
                = sourceViewportComputeItem->_meshSharedData->_renderingTopology
                      .GetNumPoints(); // _geometryIndexMapping->vertexLength(i);
            size_t sourceSmoothVertexCount
                = sourceViewportComputeItem->_vertexStencils->GetNumStencils();
            _geometryIndexMapping->updateSource(
                i,
                indexStart,
                sourcePtableSize * 1.5,
                vertexStart,
                sourceBaseVertexCount + sourceSmoothVertexCount);
            indexStart += sourcePtableSize * 1.5;
            // vertexStart += sourceBaseVertexCount + sourceSmoothVertexCount; multiDrawIndirect
            // adds vertexStart to each vertexId for the draw. The index buffer I have is a fully
            // consolidated index buffer, so I don't need to use that.
        }
        renderItem.setSourceIndexMapping(*_geometryIndexMapping.get());
    }

    MProfilingScope subsubProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:triangulateSmoothPatchTable");

    // Inspired by HdSt_Osd3IndexComputation::Resolve()
    // PxOsdOpenSubdivTokens->loop -> _patchTable is triangles
    // PxOsdOpenSubdivTokens->catmullClark + _adaptive -> BSplinePatches
    // PxOsdOpenSubdivTokens->catmullClark + !_adaptive -> quads
    // HdSt draws with tessellation shaders and we do have that? try calling
    OpenSubdiv::Far::Index const* firstIndex = nullptr;
    size_t                        ptableSize = 0;
    if (_patchTable) {
        ptableSize = _patchTable->GetPatchControlVerticesTable().size();
        if (ptableSize > 0)
            firstIndex = &_patchTable->GetPatchControlVerticesTable()[0];
    }

    int indexLength = 0;

    if (!_adaptive
        && _meshSharedData->_renderingTopology.GetScheme() == PxOsdOpenSubdivTokens->catmullClark) {
        // _patchTable is quads. Convert to triangles and make an index buffer we can draw
        int patchSize
            = _patchTable ? _patchTable->GetPatchArrayDescriptor(0).GetNumControlVertices() : 0;
        TF_VERIFY(patchSize == 4);
        VtArray<int> indices(ptableSize);
        memcpy(indices.data(), firstIndex, ptableSize * sizeof(int));

        {
            MProfilingScope subsubProfilingScope(
                HdVP2RenderDelegate::sProfilerCategory,
                MProfiler::kColorD_L1,
                "MeshViewportCompute:createTriangleIndexBuffer");

            MIndexBuffer* indexBuffer = fRenderGeom->indexBuffer(0);
            if (!indexBuffer) {
                indexBuffer = fRenderGeom->createIndexBuffer(MGeometry::kInt32);
            }
            // The new size of the index buffer needs to be 50% larger than the patch table size
            // when the patch table is quads.
            void* indexData = indexBuffer->acquire(
                ptableSize * 1.5,
                true); // we are not going to use any of the old indexing so this is write only
            int* indexWriteLocation = (int*)indexData;
            for (size_t faceStart = 0; faceStart < ptableSize; faceStart += 4) {
                *(indexWriteLocation++) = indices[faceStart];
                *(indexWriteLocation++) = indices[faceStart + 1];
                *(indexWriteLocation++) = indices[faceStart + 2];

                *(indexWriteLocation++) = indices[faceStart];
                *(indexWriteLocation++) = indices[faceStart + 2];
                *(indexWriteLocation++) = indices[faceStart + 3];
            }
            indexBuffer->commit(indexData);
            indexLength = ptableSize;
        }
    } else if (_meshSharedData->_renderingTopology.GetScheme() == PxOsdOpenSubdivTokens->loop) {
        int patchSize
            = _patchTable ? _patchTable->GetPatchArrayDescriptor(0).GetNumControlVertices() : 0;
        TF_VERIFY(patchSize == 3);
        MIndexBuffer* indexBuffer = fRenderGeom->indexBuffer(0);
        void*         indexData = indexBuffer->acquire(ptableSize, true);
        memcpy(indexData, firstIndex, ptableSize * sizeof(int));
        indexBuffer->commit(indexData);
    } else {
        // I can't handle it. Need to use patch drawing with
        // MRenderItem::setPrimitive(MGeometry::kPatch, stride)
        // But I don't have a shader set up with Tessellation.
    }
#endif
}

void MeshViewportCompute::findVertexBuffers(MRenderItem& renderItem)
{
    if (_positionVertexBufferGPU) {
        return;
    }

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:findVertexBuffers");

    for (int bufferIndex = 0; bufferIndex < fRenderGeom->vertexBufferCount(); bufferIndex++) {
        MVertexBuffer* renderBuffer = fRenderGeom->vertexBuffer(bufferIndex);
        TF_VERIFY(renderBuffer->resourceHandle());
        const MVertexBufferDescriptor& descriptor = renderBuffer->descriptor();

        if (MGeometry::kPosition == descriptor.semantic()) {
            MProfilingScope subsubProfilingScope(
                HdVP2RenderDelegate::sProfilerCategory,
                MProfiler::kColorD_L2,
                "MeshViewportCompute:positionBufferResourceHandle");
            TF_VERIFY(renderBuffer->vertexCount() == _vertexCount);
            _positionVertexBufferGPU = renderBuffer;
        } else if (MGeometry::kNormal == descriptor.semantic()) {
            MProfilingScope subsubProfilingScope(
                HdVP2RenderDelegate::sProfilerCategory,
                MProfiler::kColorD_L2,
                "MeshViewportCompute:normalBufferResourceHandle");
            _normalVertexBufferGPU = renderBuffer;
        } else if (MGeometry::kColor == descriptor.semantic()) {
            _colorVertexBufferGPU = renderBuffer;
        } else {
            fprintf(stderr, "Unsupported buffer type.\n");
            continue;
        }
    }

    if (nullptr == _normalVertexBufferGPU) {
        MProfilingScope subsubProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L1,
            "MeshViewportCompute:createNormalBuffer");

        const MHWRender::MVertexBufferDescriptor vbDesc(
            "", MHWRender::MGeometry::kNormal, MHWRender::MGeometry::kFloat, 3);

        _normalVertexBufferGPU = fRenderGeom->createVertexBuffer(vbDesc);
    }

    GLuint* normalBufferResourceHandle = (GLuint*)_normalVertexBufferGPU->resourceHandle();
    if (!normalBufferResourceHandle) {
        // tell the buffer what size it is
        void* normalsBufferData = _normalVertexBufferGPU->acquire(_vertexCount, true);
        memset(normalsBufferData, 0, _vertexCount * sizeof(float) * 3);
        _normalVertexBufferGPU->commit(normalsBufferData);
    }
}

void MeshViewportCompute::prepareAdjacencyBuffer()
{
#if defined(HDVP2_OPENGL_NORMALS)
    if (!_adjacencyBufferGPUDirty)
        return;
    _adjacencyBufferGPUDirty = false;

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:prepareAdjacencyBuffer");

    // we compute the number of normals as required by the topology.

    // https://www.khronos.org/assets/uploads/developers/library/2014-siggraph-bof/KITE-BOF_Aug14.pdf
    // https://github.com/PixarAnimationStudios/USD/blob/be1a80f8cb91133ac75e1fc2a2e1832cd10d91c8/pxr/imaging/hdSt/smoothNormals.cpp

    // We need additional padding in the header of adjacencyData because glsl compute seems to still
    // execute all branch not taken code, and just not write anything. If we don't have padding then
    // some gpu threads will see a valance in the thousands and take forever to run and/or crash
    // accessing memory out of bounds.

    const int* adjacencyData = _adjacencyBufferCPU.get();
    int        numVertex = adjacencyData[0] / 2; // two entries per vertex.
    size_t     localWorkSize = 256;
    size_t     paddingSize = (localWorkSize - numVertex % localWorkSize) * 2;

    size_t adjacencyBufferSize = _adjacencyBufferSize + paddingSize;
    size_t vertexDataSize = (numVertex * 2);
    int*   vertexDataStart = const_cast<int*>(adjacencyData);
    size_t vertexIdSize = _adjacencyBufferSize - vertexDataSize;
    int*   vertexIdStart = vertexDataStart + vertexDataSize;

    const MHWRender::MVertexBufferDescriptor intArrayDesc(
        "", MHWRender::MGeometry::kColor, MHWRender::MGeometry::kInt32, 1);
    _adjacencyBufferGPU.reset(new MHWRender::MVertexBuffer(intArrayDesc));
    void* bufferData = _adjacencyBufferGPU->acquire(adjacencyBufferSize, true);

    for (size_t i = 0; i < vertexDataSize; i += 2) {
        vertexDataStart[i] += paddingSize;
    }

    // copy the vertex data information into the new padded buffer
    int* destination = ((int*)bufferData);
    memcpy(bufferData, vertexDataStart, vertexDataSize * sizeof(int));
    // set the padding space to be zeros
    destination = destination + vertexDataSize;
    memset(destination, 0, paddingSize * sizeof(int));
    // copy the adjacency information for each vertex into the upper part of the buffer.
    destination = destination + paddingSize;
    memcpy(destination, vertexIdStart, vertexIdSize * sizeof(int));

    // commit the adjacency information
    _adjacencyBufferGPU->commit(bufferData);

    // prepare the remapping array from the regular topology to the render topology
    _renderingToSceneFaceVtxIdsGPU.reset(new MHWRender::MVertexBuffer(intArrayDesc));
    bufferData = _renderingToSceneFaceVtxIdsGPU->acquire(
        _meshSharedData->_renderingToSceneFaceVtxIds.size(), true);
    memcpy(
        bufferData,
        _meshSharedData->_renderingToSceneFaceVtxIds.data(),
        _meshSharedData->_renderingToSceneFaceVtxIds.size() * sizeof(int));
    _renderingToSceneFaceVtxIdsGPU->commit(bufferData);

    _sceneToRenderingFaceVtxIdsGPU.reset(new MHWRender::MVertexBuffer(intArrayDesc));
    bufferData = _sceneToRenderingFaceVtxIdsGPU->acquire(
        _meshSharedData->_sceneToRenderingFaceVtxIds.size(), true);
    memcpy(
        bufferData,
        _meshSharedData->_sceneToRenderingFaceVtxIds.data(),
        _meshSharedData->_sceneToRenderingFaceVtxIds.size() * sizeof(int));
    _sceneToRenderingFaceVtxIdsGPU->commit(bufferData);
#endif
}

void MeshViewportCompute::prepareUniformBufferForNormals()
{
    if (0 != _uboResourceHandle)
        return;

    if (hasOpenGL()) {
        glGenBuffers(1, &_uboResourceHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, _uboResourceHandle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(unsigned int), &_vertexCount, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

bool MeshViewportCompute::hasOpenGL()
{
    // test an arbitrary OpenGL function pointer and make sure it is not nullptr
    return nullptr != glBindBufferBase;
}

void MeshViewportCompute::initializeOpenGL()
{
#if USD_VERSION_NUM < 2102
    GlfGlewInit();
#else
    GarchGLApiLoad();
#endif
}

void MeshViewportCompute::compileNormalsProgram()
{
#if defined(HDVP2_OPENGL_NORMALS)
    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:compileNormalsProgram");

    std::string   computeShaderSource = _GetResourcePath("computeNormals.glsl");
    std::ifstream glslFile(computeShaderSource.c_str());
    std::string   glslString;
    glslFile.seekg(0, std::ios::end);
    glslString.reserve(glslFile.tellg());
    glslFile.seekg(0, std::ios::beg);

    glslString.assign((std::istreambuf_iterator<char>(glslFile)), std::istreambuf_iterator<char>());

    if (hasOpenGL()) {
        initializeOpenGL();
    }
    TF_VERIFY(hasOpenGL());

    _computeNormalsProgram = new PxrMayaGLSLProgram;
    _computeNormalsProgram->CompileShader(GL_COMPUTE_SHADER, glslString);
    _computeNormalsProgram->Link();
    _computeNormalsProgram->Validate();
    openGLErrorCheck();
#endif
}

void MeshViewportCompute::computeNormals()
{
#if defined(HDVP2_OPENGL_NORMALS)

    if (!_normalVertexBufferGPUDirty)
        return;
    _normalVertexBufferGPUDirty = false;

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:computeNormals");
    GLuint* adjacencyBufferResourceHandle = (GLuint*)_adjacencyBufferGPU->resourceHandle();

    std::call_once(_compileProgramOnce, MeshViewportCompute::compileNormalsProgram);

    GLuint programId = _computeNormalsProgram->GetProgramId();

    // We already did another lambda task that did the commit for _positionsBuffer, so we should be
    // able to get the resource handle.
    GLuint* positionBufferResourceHandle = (GLuint*)_positionVertexBufferGPU->resourceHandle();

    // normal buffer needs to be locked because we are modifying it. We don't want the CPU version
    // and GPU version of the buffer to hold different data. Locking the buffer deletes the CPU
    // version of the buffer.
    _normalVertexBufferGPU->lockResourceHandle();
    GLuint* normalBufferResourceHandle = (GLuint*)_normalVertexBufferGPU->resourceHandle();

    // remapping buffers
    GLuint* renderingToSceneFaceVtxIdsResourceHandle
        = (GLuint*)_renderingToSceneFaceVtxIdsGPU->resourceHandle();
    GLuint* sceneToRenderingFaceVtxIdsResourceHandle
        = (GLuint*)_sceneToRenderingFaceVtxIdsGPU->resourceHandle();

    if (hasOpenGL()) {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, _uboResourceHandle);
        openGLErrorCheck();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *positionBufferResourceHandle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, *adjacencyBufferResourceHandle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, *renderingToSceneFaceVtxIdsResourceHandle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, *sceneToRenderingFaceVtxIdsResourceHandle);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, *normalBufferResourceHandle);
        openGLErrorCheck();

        size_t localWorkSize = 256;
        size_t globalWorkSize = (localWorkSize - _vertexCount % localWorkSize) + _vertexCount;
        size_t num_groups = globalWorkSize / localWorkSize;

        glUseProgram(programId);
        glDispatchCompute(num_groups, 1, 1);
        glUseProgram(0);
        openGLErrorCheck();

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
        openGLErrorCheck();
    }
    _normalVertexBufferGPU->unlockResourceHandle();
#elif defined(HDVP2_OPENCL_NORMALS)

    // Make shared buffers
    std::vector<cl_mem> sharedBuffers;
    cl_int              err;

    {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L2,
            "MeshViewportCompute:copyAdjacencyToOpenCL");
        for (unsigned int i = 0; i < consolidatedItems.size(); i++) {
            _renderItemData& _renderItemData = _renderItemDatas[i];
            MRenderItem*     renderItem = consolidatedItems[i].get();

            {
                _meshSharedData->_openCLMeshInfo._adjacencyBufferCL.reset(new MOpenCLBuffer(
                    MOpenCLInfo::getOpenCLContext(),
                    CL_MEM_READ_ONLY, // the READ and WRITE flags are from the point of view of an
                                      // OpenCL Kernel.
                    _adjacencyBufferSize * sizeof(int),
                    (void*)_adjacencyBufferCPU,
                    &err));
                MOpenCLInfo::checkCLErrorStatus(err);
            }
        }
    }

    {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L2,
            "MeshViewportCompute:attachToGLBuffers");
        for (unsigned int i = 0; i < consolidatedItems.size(); i++) {
            _renderItemData& _renderItemData = _renderItemDatas[i];
            MRenderItem*     renderItem = consolidatedItems[i].get();
            {

                _meshSharedData->_openCLMeshInfo._positionsBufferShared.attach(clCreateFromGLBuffer(
                    MOpenCLInfo::getOpenCLContext(),
                    CL_MEM_READ_ONLY,
                    *(cl_GLuint*)_positionVertexBufferGPU->resourceHandle(),
                    &err));
                MOpenCLInfo::checkCLErrorStatus(err);
                sharedBuffers.push_back(
                    _meshSharedData->_openCLMeshInfo._positionsBufferShared.get());

                _meshSharedData->_openCLMeshInfo._normalsBufferShared.attach(clCreateFromGLBuffer(
                    MOpenCLInfo::getOpenCLContext(),
                    CL_MEM_WRITE_ONLY,
                    *(cl_GLuint*)_normalVertexBufferGPU->resourceHandle(),
                    &err)); // from the point of view of OpenCL!!!
                MOpenCLInfo::checkCLErrorStatus(err);
                sharedBuffers.push_back(
                    _meshSharedData->_openCLMeshInfo._normalsBufferShared.get());
            }
        }
    }

    // acquire the shared buffers
    MAutoCLEvent acquireEvent;
    {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L2,
            "MeshViewportCompute:acquireSharedBuffers");
#ifdef MAYA_BLOCKING_OPENCL
        err = clEnqueueAcquire3DObjects_blocking_api(
            MOpenCLInfo::getMayaDefaultOpenCLCommandQueue(),
            sharedBuffers.size(),
            sharedBuffers.data(),
            0,
            nullptr,
            acquireEvent.getReferenceForAssignment());
#else
        err = clEnqueueAcquireGLObjects(
            MOpenCLInfo::getMayaDefaultOpenCLCommandQueue(),
            sharedBuffers.size(),
            sharedBuffers.data(),
            0,
            nullptr,
            acquireEvent.getReferenceForAssignment());
#endif
    }
    MOpenCLInfo::checkCLErrorStatus(err);

    // get the kernel
    MString kernelFile("C:/dev/usd/ecg-maya-usd/maya-usd/lib/mayaUsd/render/vp2RenderDelegate/"
                       "mesh.cl"); // needs to get installed somewhere
    MString kernelName("computeNormals");
    MAutoCLKernel computeNormalsKernel(MOpenCLInfo::getOpenCLKernel(kernelFile, kernelName));

    // compute the work group size and global work size
    size_t workGroupSize;
    size_t retSize;
    err = clGetKernelWorkGroupInfo(
        computeNormalsKernel.get(),
        MOpenCLInfo::getOpenCLDeviceId(),
        CL_KERNEL_WORK_GROUP_SIZE,
        sizeof(size_t),
        &workGroupSize,
        &retSize);
    MOpenCLInfo::checkCLErrorStatus(err);
    size_t localWorkSize = 256;
    if (retSize > 0) {
        localWorkSize = workGroupSize;
    }

    cl_event* events = new cl_event[consolidatedItems.size()];
    {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L2,
            "MeshViewportCompute:enqueueKernels");
        for (unsigned int i = 0; i < consolidatedItems.size(); i++) {
            _renderItemData& _renderItemData = _renderItemDatas[i];
            MRenderItem*     renderItem = consolidatedItems[i].get();

            size_t remain = _vertexCount % localWorkSize;
            size_t globalWorkSize = _vertexCount;
            if (remain) {
                globalWorkSize = _vertexCount + localWorkSize - remain;
            }

            // set up the compute kernel!  This can be mostly copied from
            // CommitOpenCLPositions::operator()(), just integrate the new data handles we got
            // above.

            // set kernel args
            err = clSetKernelArg(
                computeNormalsKernel.get(),
                0,
                sizeof(cl_mem),
                (const void*)
                    _meshSharedData->_openCLMeshInfo._positionsBufferShared.getReadOnlyRef());
            MOpenCLInfo::checkCLErrorStatus(err);
            err = clSetKernelArg(
                computeNormalsKernel.get(), 1, sizeof(cl_uint), (const void*)&_vertexCount);
            MOpenCLInfo::checkCLErrorStatus(err);
            err = clSetKernelArg(
                computeNormalsKernel.get(),
                2,
                sizeof(cl_mem),
                (const void*)_meshSharedData->_openCLMeshInfo._adjacencyBufferCL->buffer()
                    .getReadOnlyRef());
            MOpenCLInfo::checkCLErrorStatus(err);
            err = clSetKernelArg(
                computeNormalsKernel.get(),
                3,
                sizeof(cl_mem),
                (const void*)
                    _meshSharedData->_openCLMeshInfo._normalsBufferShared.getReadOnlyRef());
            MOpenCLInfo::checkCLErrorStatus(err);

            // run kernel
            err = clEnqueueNDRangeKernel(
                MOpenCLInfo::getMayaDefaultOpenCLCommandQueue(),
                computeNormalsKernel.get(),
                1,
                nullptr,
                &globalWorkSize,
                &localWorkSize,
                1,
                acquireEvent.getReadOnlyRef(),
                _meshSharedData->_openCLMeshInfo._normalsBufferReady.getReferenceForAssignment());
            MOpenCLInfo::checkCLErrorStatus(err);

            // build a list of all the kernel complete events
            events[i] = _meshSharedData->_openCLMeshInfo._normalsBufferReady.get();
        }
    }

    // release the shared buffers
    MAutoCLEvent releaseEvent;
    {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L2,
            "MeshViewportCompute:releaseSharedBuffers");
#ifdef MAYA_BLOCKING_OPENCL
        clEnqueueRelease3DObjects_blocking_api(
            MOpenCLInfo::getMayaDefaultOpenCLCommandQueue(),
            sharedBuffers.size(),
            sharedBuffers.data(),
            1,
            _meshSharedData->_openCLMeshInfo._normalsBufferReady.getReadOnlyRef(),
            releaseEvent.getReferenceForAssignment());
#else
        clEnqueueReleaseGLObjects(
            MOpenCLInfo::getMayaDefaultOpenCLCommandQueue(),
            sharedBuffers.size(),
            sharedBuffers.data(),
            consolidatedItems.size(),
            events,
            releaseEvent.getReferenceForAssignment());
#endif
    }
    {
        MProfilingScope subProfilingScope(
            HdVP2RenderDelegate::sProfilerCategory,
            MProfiler::kColorD_L3,
            "MeshViewportCompute:syncOpenCL");
        DoGLWaitSync(releaseEvent.get());
    }
    delete[] events;

    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L2,
        "MeshViewportCompute:releaseOpenCLBuffers");

    for (unsigned int i = 0; i < consolidatedItems.size(); i++) {
        _renderItemData& _renderItemData = _renderItemDatas[i];
        MRenderItem*     renderItem = consolidatedItems[i].get();

        _meshSharedData->_openCLMeshInfo._positionsBufferShared.reset();
        _meshSharedData->_openCLMeshInfo._normalsBufferShared.reset();
        _meshSharedData->_openCLMeshInfo._adjacencyBufferCL->reset();
        _meshSharedData->_openCLMeshInfo._normalsBufferReady.reset();
    }

    // clFinish(MOpenCLInfo::getMayaDefaultOpenCLCommandQueue());

#elif defined(DO_CPU_NORMALS)

#endif
}

void MeshViewportCompute::computeOSD()
{
#if defined(DO_CPU_OSD) || defined(DO_OPENGL_OSD)
    MProfilingScope subProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory, MProfiler::kColorD_L2, "MeshViewportCompute:doOSD");
    // Inspired by HdSt_Osd3TopologyComputation::Resolve()

    // refine
    //  and
    // create stencil/patch table
    OpenSubdiv::Far::StencilTable const* consolidatedVertexStencils = _vertexStencils.get();
    OpenSubdiv::Far::StencilTable const* consolidatedVaryingStencils = _varyingStencils.get();
    OpenSubdiv::Far::PatchTable const*   consolidatedPatchTable = _patchTable.get();
#endif
#if defined(DO_CPU_OSD)

    class OsdCPUBuffer
    {
    public:
        OsdCPUBuffer(float* buffer)
            : _buffer(buffer)
        {
        }

        float* BindCpuBuffer() { return _buffer; }

    private:
        float* _buffer { nullptr };
    };

    // smooth the normals
    const MVertexBufferDescriptor& normalsDescriptor = _normalVertexBufferGPU->descriptor();
    int                            dimension = normalsDescriptor.dimension();
    void*                          normalsBufferData = _normalVertexBufferGPU->acquire(
        _vertexCount + consolidatedVertexStencils->GetNumStencils(), false);
    OpenSubdiv::Osd::BufferDescriptor normalSrcDesc(0, dimension, dimension);
    OpenSubdiv::Osd::BufferDescriptor normalDstDesc(_vertexCount * dimension, dimension, dimension);
    OsdCPUBuffer                      osdNormalVertexBuffer((float*)normalsBufferData);
    OpenSubdiv::Osd::CpuEvaluator::
        EvalStencils<OsdCPUBuffer, OsdCPUBuffer, OpenSubdiv::Far::StencilTable>(
            &osdNormalVertexBuffer,
            normalSrcDesc,
            &osdNormalVertexBuffer,
            normalDstDesc,
            consolidatedVertexStencils);
    _normalVertexBufferGPU->commit(normalsBufferData);

    // smooth the positions
    const MVertexBufferDescriptor& positionsDescriptor = _positionVertexBufferGPU->descriptor();
    dimension = positionsDescriptor.dimension();
    void* positionsBufferData = _positionVertexBufferGPU->acquire(
        _vertexCount + consolidatedVertexStencils->GetNumStencils(), false);
    OpenSubdiv::Osd::BufferDescriptor positionSrcDesc(0, dimension, dimension);
    OpenSubdiv::Osd::BufferDescriptor positionDstDesc(
        _vertexCount * dimension, dimension, dimension);
    OsdCPUBuffer osdPositionVertexBuffer((float*)positionsBufferData);
    OpenSubdiv::Osd::CpuEvaluator::
        EvalStencils<OsdCPUBuffer, OsdCPUBuffer, OpenSubdiv::Far::StencilTable>(
            &osdPositionVertexBuffer,
            positionSrcDesc,
            &osdPositionVertexBuffer,
            positionDstDesc,
            consolidatedVertexStencils);
    _positionVertexBufferGPU->commit(positionsBufferData);
#elif defined(DO_OPENGL_OSD)

    class OsdGLBuffer
    {
    public:
        OsdGLBuffer(GLuint resourceId, size_t dimension)
            : _dimension(dimension)
            , _resourceId(resourceId)
        {
        }

        size_t GetNumElements() const { return _dimension; }
        GLuint BindVBO() { return _resourceId; }

    private:
        size_t _dimension { 0 };
        GLuint _resourceId;
    };

    // Smooth the positions
    // ensure the position buffer is large enough to hold the smoothed result
    const MVertexBufferDescriptor& positionsDescriptor = _positionVertexBufferGPU->descriptor();
    int                            dimension = positionsDescriptor.dimension();
    void*                          positionsBufferData = _positionVertexBufferGPU->acquire(
        _vertexCount + consolidatedVertexStencils->GetNumStencils(), false);
    _positionVertexBufferGPU->commit(positionsBufferData);

    // BufferDescriptor is meant to describe an interleaved buffer, but we are not interleaved, so
    // it looks dumb
    OpenSubdiv::Osd::BufferDescriptor positionSrcDesc(0, dimension, dimension);
    OpenSubdiv::Osd::BufferDescriptor positionDstDesc(
        _vertexCount * dimension, dimension, dimension);

    static OpenSubdiv::Osd::EvaluatorCacheT<OpenSubdiv::Osd::GLComputeEvaluator> evaluatorCache;
    OpenSubdiv::Osd::GLComputeEvaluator const*                                   positionInstance
        = OpenSubdiv::Osd::GetEvaluator<OpenSubdiv::Osd::GLComputeEvaluator>(
            &evaluatorCache, positionSrcDesc, positionDstDesc, (void*)NULL);

    OsdGLBuffer osdPositionBuffer(
        *((GLuint*)(_positionVertexBufferGPU->resourceHandle())), dimension);
    OpenSubdiv::Osd::GLStencilTableSSBO* gpuStencilTable
        = OpenSubdiv::Osd::GLStencilTableSSBO::Create(consolidatedVertexStencils, nullptr);
    positionInstance->EvalStencils(
        &osdPositionBuffer, positionSrcDesc, &osdPositionBuffer, positionDstDesc, gpuStencilTable);

    // ensure the normal buffer is large enough to hold the smoothed result
    const MVertexBufferDescriptor& normalsDescriptor = _normalVertexBufferGPU->descriptor();
    dimension = normalsDescriptor.dimension();
    void* normalsBufferData = _normalVertexBufferGPU->acquire(
        _vertexCount + consolidatedVertexStencils->GetNumStencils(), false);
    _normalVertexBufferGPU->commit(normalsBufferData);

    // BufferDescriptor is meant to describe an interleaved buffer, but we are not interleaved, so
    // it looks dumb
    OpenSubdiv::Osd::BufferDescriptor normalSrcDesc(0, dimension, dimension);
    OpenSubdiv::Osd::BufferDescriptor normalDstDesc(_vertexCount * dimension, dimension, dimension);

    OpenSubdiv::Osd::GLComputeEvaluator const* normalInstance
        = OpenSubdiv::Osd::GetEvaluator<OpenSubdiv::Osd::GLComputeEvaluator>(
            &evaluatorCache, normalSrcDesc, normalDstDesc, (void*)NULL);

    OsdGLBuffer osdNormalBuffer(*((GLuint*)(_normalVertexBufferGPU->resourceHandle())), dimension);
    normalInstance->EvalStencils(
        &osdNormalBuffer, normalSrcDesc, &osdNormalBuffer, normalDstDesc, gpuStencilTable);

    if (_colorVertexBufferGPU) {
        // ensure the color buffer is large enough to hold the smoothed result
        const MVertexBufferDescriptor& colorsDescriptor = _colorVertexBufferGPU->descriptor();
        dimension = colorsDescriptor.dimension();
        void* ColorsBufferData = _colorVertexBufferGPU->acquire(
            _vertexCount + consolidatedVertexStencils->GetNumStencils(), false);
        _colorVertexBufferGPU->commit(ColorsBufferData);

        // BufferDescriptor is meant to describe an interleaved buffer, but we are not interleaved,
        // so it looks dumb
        OpenSubdiv::Osd::BufferDescriptor ColorSrcDesc(0, dimension, dimension);
        OpenSubdiv::Osd::BufferDescriptor ColorDstDesc(
            _vertexCount * dimension, dimension, dimension);

        OpenSubdiv::Osd::GLComputeEvaluator const* colorInstance
            = OpenSubdiv::Osd::GetEvaluator<OpenSubdiv::Osd::GLComputeEvaluator>(
                &evaluatorCache, ColorSrcDesc, ColorDstDesc, (void*)NULL);

        OsdGLBuffer osdColorBuffer(
            *((GLuint*)(_colorVertexBufferGPU->resourceHandle())), dimension);
        colorInstance->EvalStencils(
            &osdColorBuffer, ColorSrcDesc, &osdColorBuffer, ColorDstDesc, gpuStencilTable);
    }
#endif
}

void MeshViewportCompute::setClean()
{
    // when we perform consolidated compute the dirty flags for the source items remain dirty.
    // Each individual source item isn't capable of drawing unconsolidated, so the flags must
    // remain dirty to guard against potentially drawing unconsolidated & requiring the compute
    // to execute.

    _topologyDirty = false;
    _adjacencyBufferGPUDirty = false;
    _normalVertexBufferGPUDirty = false;
    _executed = true;
}

bool MeshViewportCompute::execute(
    const MPxViewportComputeItem::Actions& availableActions,
    MRenderItem&                           renderItem)
{
    if (!_normalVertexBufferGPUDirty)
        return true;

    if (_adjacencyTaskInProgress)
        return false;

    MProfilingScope mainProfilingScope(
        HdVP2RenderDelegate::sProfilerCategory,
        MProfiler::kColorD_L1,
        "MeshViewportCompute::execute");

    findConsolidationMapping(renderItem);

    if (_topologyDirty || _adjacencyBufferSize == 0) {
        TF_VERIFY(!_adjacencyTaskInProgress);
        _adjacencyTaskInProgress = true;
        EnqueueLambdaTask([this] {
            this->createConsolidatedTopology(getSceneTopology);
            this->createConsolidatedTopology(getRenderingTopology);
            this->createConsolidatedAdjacency();
            this->_adjacencyTaskInProgress = false;
        });
        return false;
    }

    findRenderGeometry(renderItem);

    createConsolidatedOSDTables(renderItem); // disabled by preprocessor macros

    findVertexBuffers(renderItem);

    prepareAdjacencyBuffer();

    prepareUniformBufferForNormals();

    computeNormals();

    computeOSD(); // disabled by preprocessor macros

    setClean();

    return true;
}

bool MeshViewportCompute::canConsolidate(const MPxViewportComputeItem& other) const
{
    const MeshViewportCompute* otherMeshViewportCompute
        = dynamic_cast<const MeshViewportCompute*>(&other);
    if (nullptr == otherMeshViewportCompute)
        return false;

    // If the compute has executed then the data to be consolidated will already
    // be smoothed. Smoothed items can only consolidate with other smoothed items.
    return hasExecuted() == otherMeshViewportCompute->hasExecuted()
#if defined(DO_CPU_OSD) || defined(DO_OPENGL_OSD)
        && _adaptive == otherMeshViewportCompute->_adaptive
        && _level == otherMeshViewportCompute->_level
        && _meshSharedData->_renderingTopology.GetScheme()
        == otherMeshViewportCompute->_meshSharedData->_renderingTopology.GetScheme()
#endif
        ;
}

MSharedPtr<MPxViewportComputeItem> MeshViewportCompute::cloneForConsolidation() const
{
    MSharedPtr<MeshViewportCompute> clone
        = MSharedPtr<MeshViewportCompute>::make<>(std::make_shared<HdVP2MeshSharedData>(), nullptr);
    return clone;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
