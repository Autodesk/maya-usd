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
#ifndef HD_VP2_MESHVIEWPORTCOMPUTE
#define HD_VP2_MESHVIEWPORTCOMPUTE

#include <pxr/pxr.h> // for PXR_VERSION

#include <maya/MTypes.h> // for MAYA_API_VERSION

/*
    GPU Compute Prototype

    The GPU evaluation prototype is two separate parts, the normal calculation code and the OSD
    code.

    The normal calculation code is enabled by setting HDVP2_USE_GPU_NORMAL_COMPUTATION=1 at runtime.
    The normal calculation code is close to being stable enough for general use, but hasn't had
    enough polish to enable by default.

    The OSD code requires the normal calculation code to be enabled to use. OSD is enabled
    by compiling with HDVP2_ENABLE_GPU_OSD. The OSD code is much less stable then the normals
    calculation code and comes with a number of huge

    OSD Limitations:
     * No OSD adaptive support
     * scenes with animation behave poorly
     * selection in the viewport is very slow
     * toggling VP2 consolidation world off and on will cause crashes
     * some objects draw with incorrect indexing
*/

// Maya 2020 is missing API necessary for compute support
// OSX doesn't have OpenGL 4.3 support necessary for compute
// USD before 20.08 doesn't include some OSD commits we rely on
#if MAYA_API_VERSION >= 20210000 && !defined(OSMac_) && PXR_VERSION > 2002
//#define HDVP2_ENABLE_GPU_COMPUTE
#endif

#ifdef HDVP2_ENABLE_GPU_COMPUTE
/*
    GPU OSD computation implementation is experimental.
*/
//#define HDVP2_ENABLE_GPU_OSD
#ifdef HDVP2_ENABLE_GPU_OSD
#define DO_OPENGL_OSD
//#define DO_CPU_OSD
#endif

#include <pxr/imaging/hd/mesh.h>

#include <maya/MHWGeometry.h>
#include <maya/MSharedPtr.h>

#ifdef HDVP2_ENABLE_GPU_OSD
#include <opensubdiv/far/_patchTable.h>
#include <opensubdiv/far/stencilTable.h>
#endif

#include <mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.h>

#include <climits>

#define HDVP2_OPENGL_NORMALS
/*
    OpenCL Normals calculation is experimental
*/
//#define HDVP2_OPENCL_NORMALS
#ifdef HDVP2_OPENCL_NORMALS
#include <clew/clew.h>
#endif
#ifdef HDVP2_OPENGL_NORMALS
//#define DO_OPENGL_ERROR_CHECK
// clang-format wants to re-order these two includes but they must be done in this order or
// the code will not compile.
// clang-format off
#if PXR_VERSION < 2102
#include <pxr/imaging/glf/glew.h> // needs to be included before anything else includes gl.h
#else
#include <pxr/imaging/garch/glApi.h>
#endif
#include <mayaUsd/render/px_vp20/glslProgram.h> // this includes gl.h and no GL loader.
//clang-format on
#endif

#include <maya/MPxViewportComputeItem.h>

#ifdef HDVP2_OPENCL_NORMALS
#include <maya/MOpenCLAutoPtr.h>
#include <maya/MOpenCLBuffer.h>
#endif

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/thisPlugin.h>
#include <pxr/base/tf/diagnostic.h>

#ifdef HDVP2_ENABLE_GPU_OSD
#include <opensubdiv/far/_patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>
#include <opensubdiv/far/stencilTable.h>
#include <opensubdiv/far/stencilTableFactory.h>
#include <opensubdiv/osd/cpuEvaluator.h>
#include <opensubdiv/osd/cpuVertexBuffer.h>
#include <opensubdiv/osd/glComputeEvaluator.h>
#include <opensubdiv/osd/glVertexBuffer.h>
#include <opensubdiv/osd/mesh.h>
#include <opensubdiv/version.h>
#endif

#include <tbb/task.h>

#include <fstream>
#include <streambuf>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct HdVP2MeshSharedData;
class HdVP2DrawItem;
class PxrMayaGLSLProgram;

/*! \brief  HdVP2Mesh-specific compute class for evaluating geometry streams and OSD
    \class  MeshViewportCompute

    A mesh can have shader stream requires (such as normals, tangents etc) and OSD
    requirements. This class executes GPU compute kernels to fulfill the geometry
    requirements of an HdVP2Mesh.

    A key performance feature of MeshViewportCompute is MRenderItems from different
    HdVP2Mesh objects which have the same compute requirements can be consolidated
    together in consolidated world and have their compute executed in a single
    consolidated compute kernel, rather than issuing a compute kernel per unconsolidated
    render item.
*/
class MeshViewportCompute : public MPxViewportComputeItem
{
private:
    std::shared_ptr<HdVP2MeshSharedData> _meshSharedData;
    const void* _drawItem { nullptr }; // only set for a consolidation source, never dereferenced
    bool        _executed { false };   // Has this compute been executed
    bool        _sourcesExecuted {
        false
    }; // Have the source compute's executed. only valid for a consolidated compute

    MSharedPtr<MeshViewportCompute> _consolidatedCompute;

    std::unique_ptr<MHWRender::MGeometryIndexMapping> _geometryIndexMapping;
    unsigned int                                      _vertexCount { 0 };
    GLuint                                            _uboResourceHandle { 0 };

    // adjacency information for normals
    size_t                                    _adjacencyBufferSize { 0 };
    std::unique_ptr<const int>                _adjacencyBufferCPU;
    std::unique_ptr<MHWRender::MVertexBuffer> _adjacencyBufferGPU;
    std::unique_ptr<MHWRender::MVertexBuffer> _renderingToSceneFaceVtxIdsGPU;
    std::unique_ptr<MHWRender::MVertexBuffer> _sceneToRenderingFaceVtxIdsGPU;

    // Geometry information
    MGeometry* fRenderGeom { nullptr };
    // Buffers in MGeometry.
    MVertexBuffer* _positionVertexBufferGPU { nullptr }; // not owned by *this, owned by fRenderGeom
    MVertexBuffer* _normalVertexBufferGPU { nullptr };   // not owned by *this, owned by fRenderGeom
    MVertexBuffer* _colorVertexBufferGPU { nullptr };    // not owned by *this, owned by fRenderGeom

    bool _adjacencyTaskInProgress { false };
    bool _topologyDirty { true };           // sourceMeshSharedData->_renderingTopology has changed
    bool _adjacencyBufferGPUDirty { true }; //_adjacencyBufferGPU is dirty
    bool _normalVertexBufferGPUDirty { true }; //_normalVertexBufferGPU is dirty

#if defined(DO_CPU_OSD) || defined(DO_OPENGL_OSD)
    // OSD information
    std::unique_ptr<OpenSubdiv::Far::StencilTable const> _vertexStencils;
    std::unique_ptr<OpenSubdiv::Far::StencilTable const> _varyingStencils;
    std::unique_ptr<OpenSubdiv::Far::PatchTable const>   _patchTable;
    bool                                                 _adaptive { false };
    int                                                  _level { 1 };
#endif

#if defined(HDVP2_OPENGL_NORMALS)
    static std::once_flag      _compileProgramOnce;
    static PxrMayaGLSLProgram* _computeNormalsProgram;
#endif

#if defined(HDVP2_OPENCL_NORMALS)
    //!< pure OpenCL version of the positions buffer
    std::unique_ptr<MOpenCLBuffer> _positionsBufferCL;

    //!< A shared CL-GL version of _positionsBuffer
    MAutoCLMem _positionsBufferShared;

    // data to compute smooth normals
    std::unique_ptr<MOpenCLBuffer> _adjacencyBufferCL;
    MAutoCLMem                     _normalsBufferShared;
    MAutoCLEvent                   _normalsBufferReady;
#endif

    static void openGLErrorCheck();
    static bool hasOpenGL();
    static void initializeOpenGL();
    bool        hasExecuted() const;
    void        reset();
    void        findConsolidationMapping(MRenderItem& renderItem);
    template <typename TopologyAccessor>
    void        createConsolidatedTopology(TopologyAccessor getTopology);
    void        createConsolidatedAdjacency();
    void        findRenderGeometry(MRenderItem& renderItem);
    void        createConsolidatedOSDTables(MRenderItem& renderItem);
    void        findVertexBuffers(MRenderItem& renderItem);
    void        prepareAdjacencyBuffer();
    void        prepareUniformBufferForNormals();
    static void compileNormalsProgram();
    void        computeNormals();
    void        computeOSD();
    void        setClean();

public:
    MeshViewportCompute(std::shared_ptr<HdVP2MeshSharedData> meshSharedData, const void* drawItem)
        : MPxViewportComputeItem(false)
        , _meshSharedData(meshSharedData)
        , _drawItem(drawItem)
    {
        setRequiredAction(MPxViewportComputeItem::kAccessVirtualDevice, true);
        setRequiredAction(MPxViewportComputeItem::kAccessConsolidation, true);
        setRequiredAction(MPxViewportComputeItem::kModifyVertexBufferData, true);
#if defined(DO_CPU_OSD) || defined(DO_OPENGL_OSD)
        setRequiredAction(MPxViewportComputeItem::kModifyVertexBufferSize, true);
        setRequiredAction(MPxViewportComputeItem::kModifyConsolidation, true);
#endif
    }

    virtual ~MeshViewportCompute()
    {
        if (0 != _uboResourceHandle)
            glDeleteBuffers(1, &_uboResourceHandle);
    }

    bool execute(const MPxViewportComputeItem::Actions& availableActions, MRenderItem& renderItem)
        override;
    bool canConsolidate(const MPxViewportComputeItem& other) const override;
    MSharedPtr<MPxViewportComputeItem> cloneForConsolidation() const override;
    bool                               verifyDrawItem(const HdVP2DrawItem& drawItem) const;

    void setTopologyDirty();
    void setAdjacencyBufferGPUDirty();
    void setNormalVertexBufferGPUDirty();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

#endif
