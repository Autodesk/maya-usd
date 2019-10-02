// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "render_delegate.h"

#include "material.h"
#include "mesh.h"

#include "render_pass.h"
#include "instancer.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/tokens.h"

#include <maya/MProfiler.h>

#include <unordered_map>
#include <unordered_set>

#include <tbb/reader_writer_lock.h>
#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
    /*! \brief List of supported Rprims by VP2 render delegate
    */
    inline const TfTokenVector& _SupportedRprimTypes() {
        static const TfTokenVector r{ HdPrimTypeTokens->mesh };
        return r;
    }

    /*! \brief List of supported Sprims by VP2 render delegate
    */
    inline const TfTokenVector& _SupportedSprimTypes() {
        static const TfTokenVector r{
            HdPrimTypeTokens->material,        HdPrimTypeTokens->camera };
        return r;
    }

    /*! \brief List of supported Bprims by VP2 render delegate
    */
    inline const TfTokenVector& _SupportedBprimTypes() {
        static const TfTokenVector r{ HdPrimTypeTokens->texture };
        return r;
    }

    /*! \brief  Color hash helper class, used by shader registry
    */
    struct MColorHash
    {
        std::size_t operator()(const MColor& color) const
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, color.r);
            boost::hash_combine(seed, color.g);
            boost::hash_combine(seed, color.b);
            boost::hash_combine(seed, color.a);
            return seed;
        }
    };

    std::unordered_map<MColor, MHWRender::MShaderInstance*, MColorHash>    _fallbackShaders;        //!< Shader registry used by fallback shader method
    tbb::reader_writer_lock                                                _mutexFallbackShaders;   //!< Synchronization used to protect concurrent read from serial writes

    /*! \brief  Sampler state desc hash helper class, used by sampler state cache.
    */
    struct MSamplerStateDescHash
    {
        std::size_t operator()(const MHWRender::MSamplerStateDesc& desc) const
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, desc.filter);
            boost::hash_combine(seed, desc.comparisonFn);
            boost::hash_combine(seed, desc.addressU);
            boost::hash_combine(seed, desc.addressV);
            boost::hash_combine(seed, desc.addressW);
            boost::hash_combine(seed, desc.borderColor[0]);
            boost::hash_combine(seed, desc.borderColor[1]);
            boost::hash_combine(seed, desc.borderColor[2]);
            boost::hash_combine(seed, desc.borderColor[3]);
            boost::hash_combine(seed, desc.mipLODBias);
            boost::hash_combine(seed, desc.minLOD);
            boost::hash_combine(seed, desc.maxLOD);
            boost::hash_combine(seed, desc.maxAnisotropy);
            boost::hash_combine(seed, desc.coordCount);
            boost::hash_combine(seed, desc.elementIndex);
            return seed;
        }
    };

    /*! Sampler state desc equality helper class, used by sampler state cache.
    */
    struct MSamplerStateDescEquality
    {
        bool operator() (
            const MHWRender::MSamplerStateDesc& a,
            const MHWRender::MSamplerStateDesc& b) const
        {
            return (
                a.filter == b.filter &&
                a.comparisonFn == b.comparisonFn &&
                a.addressU == b.addressU &&
                a.addressV == b.addressV &&
                a.addressW == b.addressW &&
                a.borderColor[0] == b.borderColor[0] &&
                a.borderColor[1] == b.borderColor[1] &&
                a.borderColor[2] == b.borderColor[2] &&
                a.borderColor[3] == b.borderColor[3] &&
                a.mipLODBias == b.mipLODBias &&
                a.minLOD == b.minLOD &&
                a.maxLOD == b.maxLOD &&
                a.maxAnisotropy == b.maxAnisotropy &&
                a.coordCount == b.coordCount &&
                a.elementIndex == b.elementIndex
            );
        }
    };

    using MSamplerStateCache = std::unordered_map<
        MHWRender::MSamplerStateDesc,
        const MHWRender::MSamplerState*,
        MSamplerStateDescHash,
        MSamplerStateDescEquality
    >;

    MSamplerStateCache _samplerStates;                           //!< Sampler state cache

    const MString _diffuseColorParameterName = "diffuseColor";   //!< Shader parameter name
    const MString _solidColorParameterName   = "solidColor";     //!< Shader parameter name
    const MString _pointSizeParameterName    = "pointSize";      //!< Shader parameter name
} // namespace

const int HdVP2RenderDelegate::sProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "HdVP2RenderDelegate", "HdVP2RenderDelegate"
#else
    "HdVP2RenderDelegate"
#endif
);

std::mutex HdVP2RenderDelegate::_mutexResourceRegistry;
std::atomic_int HdVP2RenderDelegate::_counterResourceRegistry;
HdResourceRegistrySharedPtr HdVP2RenderDelegate::_resourceRegistry;

/*! \brief  Constructor.
*/
HdVP2RenderDelegate::HdVP2RenderDelegate(ProxyRenderDelegate& drawScene) {
    _id = SdfPath(TfToken(TfStringPrintf("/HdVP2RenderDelegate_%p", this)));

    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
    if (_counterResourceRegistry.fetch_add(1) == 0) {
        _resourceRegistry.reset(new HdResourceRegistry());
    }

    _renderParam.reset(new HdVP2RenderParam(drawScene));
}

/*! \brief  Destructor.
*/
HdVP2RenderDelegate::~HdVP2RenderDelegate() {
    std::lock_guard<std::mutex> guard(_mutexResourceRegistry);
    
    if (_counterResourceRegistry.fetch_sub(1) == 1) {
        _resourceRegistry.reset();
    }
}

/*! \brief  Return delegate's HdVP2RenderParam, giving access to things like MPxSubSceneOverride.
*/
HdRenderParam* HdVP2RenderDelegate::GetRenderParam() const {
    return _renderParam.get();
}

/*! \brief  Notification to commit resources to GPU & compute before rendering

    This notification sent by HdEngine happens after parallel synchronization of data,
    prim's via VP2 resource registry are inserting work to commit. Now is the time
    on main thread to commit resources and compute missing streams.

    In future we will better leverage evaluation time to perform synchronization
    of data and allow main-thread task execution during the compute as it's done
    for the rest of VP2 synchronization with DG data.
*/
void HdVP2RenderDelegate::CommitResources(HdChangeTracker* tracker) {
    TF_UNUSED(tracker);

    MProfilingScope profilingScope(sProfilerCategory, MProfiler::kColorC_L2, "Commit resources");

    // --------------------------------------------------------------------- //
    // RESOLVE, COMPUTE & COMMIT PHASE
    // --------------------------------------------------------------------- //
    // All the required input data is now resident in memory, next we must:
    //
    //     1) Execute compute as needed for normals, tessellation, etc.
    //     2) Commit resources to the GPU.
    //     3) Update any scene-level acceleration structures.

    _resourceRegistryVP2.Commit();
}

/*! \brief  Return a list of which Rprim types can be created by this class's.
*/
const TfTokenVector& HdVP2RenderDelegate::GetSupportedRprimTypes() const {
    return _SupportedRprimTypes();
}

/*! \brief  Return a list of which Sprim types can be created by this class's.
*/
const TfTokenVector& HdVP2RenderDelegate::GetSupportedSprimTypes() const {
    return _SupportedSprimTypes();
}

/*! \brief  Return a list of which Bprim types can be created by this class's.
*/
const TfTokenVector& HdVP2RenderDelegate::GetSupportedBprimTypes() const {
    return _SupportedBprimTypes();
}

/*! \brief  Return unused global resource registry.
*/
HdResourceRegistrySharedPtr HdVP2RenderDelegate::GetResourceRegistry() const {
    return _resourceRegistry;
}

/*! \brief  Return VP2 resource registry, holding access to commit execution enqueue.
*/
HdVP2ResourceRegistry& HdVP2RenderDelegate::GetVP2ResourceRegistry() {
    return _resourceRegistryVP2;
}

/*! \brief  Create a renderpass for rendering a given collection.
*/
HdRenderPassSharedPtr HdVP2RenderDelegate::CreateRenderPass(HdRenderIndex* index, const HdRprimCollection& collection) {
    return HdRenderPassSharedPtr(new HdVP2RenderPass(this, index, collection));
}

/*! \brief  Request to create a new VP2 instancer.

    \param id           The unique identifier of this instancer.
    \param instancerId  The unique identifier for the parent instancer that
                        uses this instancer as a prototype (may be empty).

    \return A pointer to the new instancer or nullptr on error.
*/
HdInstancer* HdVP2RenderDelegate::CreateInstancer(
    HdSceneDelegate* delegate, const SdfPath& id, const SdfPath& instancerId) {
    
    return new HdVP2Instancer(delegate, id, instancerId);
}

/*! \brief  Destroy instancer instance
*/
void HdVP2RenderDelegate::DestroyInstancer(HdInstancer* instancer) {
    delete instancer;
}

/*! \brief  Request to Allocate and Construct a new, VP2 specialized Rprim.

    \param typeId       the type identifier of the prim to allocate
    \param rprimId      a unique identifier for the prim
    \param instancerId  the unique identifier for the instancer that uses
                        the prim (optional: May be empty).

    \return A pointer to the new prim or nullptr on error.
*/
HdRprim* HdVP2RenderDelegate::CreateRprim(
    const TfToken& typeId, const SdfPath& rprimId, const SdfPath& instancerId) {
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdVP2Mesh(this, rprimId, instancerId);
    }
    //if (typeId == HdPrimTypeTokens->volume) {
    //    return new HdVP2Volume(this, rprimId, instancerId);
    //}
    TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    return nullptr;
}

/*! \brief  Destroy & deallocate Rprim instance
*/
void HdVP2RenderDelegate::DestroyRprim(HdRprim* rPrim) {
    delete rPrim;
}

/*! \brief  Request to Allocate and Construct a new, VP2 specialized Sprim.

    \param typeId   the type identifier of the prim to allocate
    \param sprimId  a unique identifier for the prim

    \return A pointer to the new prim or nullptr on error.
*/
HdSprim* HdVP2RenderDelegate::CreateSprim(
    const TfToken& typeId, const SdfPath& sprimId) {
    if (typeId == HdPrimTypeTokens->material) {
        return new HdVP2Material(this, sprimId);
    }
    if (typeId == HdPrimTypeTokens->camera) { 
        return new HdCamera(sprimId); 
    }
    /*
    if (typeId == HdPrimTypeTokens->sphereLight) {
        return HdVP2Light::CreatePointLight(this, sprimId);
    }
    if (typeId == HdPrimTypeTokens->distantLight) {
        return HdVP2Light::CreateDistantLight(this, sprimId);
    }
    if (typeId == HdPrimTypeTokens->diskLight) {
        return HdVP2Light::CreateDiskLight(this, sprimId);
    }
    if (typeId == HdPrimTypeTokens->rectLight) {
        return HdVP2Light::CreateRectLight(this, sprimId);
    }
    if (typeId == HdPrimTypeTokens->cylinderLight) {
        return HdVP2Light::CreateCylinderLight(this, sprimId);
    }
    if (typeId == HdPrimTypeTokens->domeLight) {
        return HdVP2Light::CreateDomeLight(this, sprimId);
    }
    */
    TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    return nullptr;
}

/*! \brief  Request to Allocate and Construct an Sprim to use as a standin, if there
            if an error with another another Sprim of the same type.For example,
            if another prim references a non - exisiting Sprim, the fallback could
            be used.

    \param typeId   the type identifier of the prim to allocate

    \return A pointer to the new prim or nullptr on error.
*/
HdSprim* HdVP2RenderDelegate::CreateFallbackSprim(const TfToken& typeId) {
    if (typeId == HdPrimTypeTokens->material) {
        return new HdVP2Material(this, SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCamera(SdfPath::EmptyPath());
    }
    /*
    if (typeId == HdPrimTypeTokens->sphereLight) {
        return HdVP2Light::CreatePointLight(this, SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->distantLight) {
        return HdVP2Light::CreateDistantLight(this, SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->diskLight) {
        return HdVP2Light::CreateDiskLight(this, SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->rectLight) {
        return HdVP2Light::CreateRectLight(this, SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->cylinderLight) {
        return HdVP2Light::CreateCylinderLight(this, SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->domeLight) {
        return HdVP2Light::CreateDomeLight(this, SdfPath::EmptyPath());
    }
    */
    TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    return nullptr;
}

/*! \brief  Destroy & deallocate Sprim instance
*/
void HdVP2RenderDelegate::DestroySprim(HdSprim* sPrim) {
    delete sPrim;
}

/*! \brief  Request to Allocate and Construct a new, VP2 specialized Bprim.
    
    \param typeId the type identifier of the prim to allocate
    \param sprimId a unique identifier for the prim

    \return A pointer to the new prim or nullptr on error.
*/
HdBprim* HdVP2RenderDelegate::CreateBprim(
    const TfToken& typeId, const SdfPath& bprimId) {
    /*
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdVP2Texture(this, bprimId);
    }
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdVP2RenderBuffer(bprimId);
    }
    if (typeId == _tokens->openvdbAsset) {
        return new HdVP2OpenvdbAsset(this, bprimId);
    }
    */
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    return nullptr;
}

/*! \brief  Request to Allocate and Construct a Bprim to use as a standin, if there
            if an error with another another Bprim of the same type.  For example,
            if another prim references a non-exisiting Bprim, the fallback could
            be used.

    \param typeId the type identifier of the prim to allocate

    \return A pointer to the new prim or nullptr on error.
*/
HdBprim* HdVP2RenderDelegate::CreateFallbackBprim(const TfToken& typeId) {
    /*
    if (typeId == HdPrimTypeTokens->texture) {
        return new HdVP2Texture(this, SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdVP2RenderBuffer(SdfPath::EmptyPath());
    }
    if (typeId == _tokens->openvdbAsset) {
        return new HdVP2OpenvdbAsset(this, SdfPath::EmptyPath());
    }
    */
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    return nullptr;
}

/*! \brief  Destroy & deallocate Bprim instance
*/
void HdVP2RenderDelegate::DestroyBprim(HdBprim* bPrim) { 
    delete bPrim; 
}

/*! \brief  Returns a token that indicates material bindings purpose.

    The full material purpose is suggested according to
      https://github.com/PixarAnimationStudios/USD/pull/853
*/
TfToken HdVP2RenderDelegate::GetMaterialBindingPurpose() const {
    return HdTokens->full;
}

/*! \brief  Returns a node name made as a child of delegate's id.
*/
MString HdVP2RenderDelegate::GetLocalNodeName(const MString& name) const {
    return MString(_id.AppendChild(TfToken(name.asChar())).GetText());
}

/*! \brief  Returns a fallback shader instance when no material is found.
            
    This method is keeping registry of all fallback shaders generated, allowing only
    one instance per color which enables consolidation of render calls with same shader
    instance.

    \param color    Color to set on given shader instance

    \return A new or existing copy of shader instance with given color parameter set
*/
MHWRender::MShaderInstance* HdVP2RenderDelegate::GetFallbackShader(MColor color) const {
    // Look for it first with reader lock
    {
        tbb::reader_writer_lock::scoped_lock_read lockThisForRead(_mutexFallbackShaders);
    
        auto it = _fallbackShaders.find(color);
        if (it != _fallbackShaders.end())
            return it->second;
    }

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    const MHWRender::MShaderManager* shaderMgr = renderer ? renderer->getShaderManager() : nullptr;
    MHWRender::MShaderInstance* fallbackShader = nullptr;

    if (shaderMgr) {
        fallbackShader = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dBlinnShader);
        float solidColor[] = { color.r, color.g, color.b, color.a };
        fallbackShader->setParameter(_diffuseColorParameterName, solidColor);
    }
    else
        return nullptr;

    // Time to lock for write
    tbb::reader_writer_lock::scoped_lock lockThisForWrite(_mutexFallbackShaders);

    // Double check that it wasn't inserted by another thread
    auto it = _fallbackShaders.find(color);
    if (it != _fallbackShaders.end())
        return it->second;

    // Insert instance we just created
    _fallbackShaders[color] = fallbackShader;
    
    return fallbackShader;
}

/*! \brief  Returns a fallback shader instance when no material is found to support color per vertex.

    \return A new copy of shader instance
*/
MHWRender::MShaderInstance* HdVP2RenderDelegate::GetColorPerVertexShader() const
{
    static MHWRender::MShaderInstance* sCPVShader = nullptr;

    if (sCPVShader == nullptr) {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer) {
            const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
            if (shaderMgr) {
                sCPVShader = shaderMgr->getStockShader(
                    MHWRender::MShaderManager::k3dBlinnShader);

                sCPVShader->addInputFragment(
                    "mayaCPVPassing", "C_4F", "diffuseColor", "colorIn");
            }
        }
    }

    return sCPVShader;
}

/*! \brief  Returns a 3d green shader that can be used for selection highlight.
*/
MHWRender::MShaderInstance* HdVP2RenderDelegate::Get3dSolidShader() const
{
    static MHWRender::MShaderInstance* s3dSolidShader = nullptr;

    if (!s3dSolidShader) {
        MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
        if (renderer) {
            const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager();
            if (shaderMgr) {
                s3dSolidShader = shaderMgr->getStockShader(
                    MHWRender::MShaderManager::k3dSolidShader);
                if (s3dSolidShader)
                {
                    constexpr float color[] = { 0.056f, 1.0f, 0.366f, 1.0f };
                    s3dSolidShader->setParameter(_solidColorParameterName, color);
                }
            }
        }
    }

    return s3dSolidShader;
}

/*! \brief  Returns a white 3d fat point shader.
*/
MHWRender::MShaderInstance* HdVP2RenderDelegate::Get3dFatPointShader() const
{
    static MHWRender::MShaderInstance* s3dFatPointShader = nullptr;

    if (!s3dFatPointShader)
    {
        if (MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer())
        {
            if (const MHWRender::MShaderManager* shaderMgr = renderer->getShaderManager())
            {
                s3dFatPointShader = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dFatPointShader);

                if (s3dFatPointShader)
                {
                    const float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                    const float point[] = { 5.0, 5.0 };

                    s3dFatPointShader->setParameter(_solidColorParameterName, white);
                    s3dFatPointShader->setParameter(_pointSizeParameterName, point);
                }
            }
        }
    }

    return s3dFatPointShader;
}

/*! \brief  Returns a sampler state as required.
*/
const MHWRender::MSamplerState* HdVP2RenderDelegate::GetSamplerState(
    const MHWRender::MSamplerStateDesc& desc) const
{
    const MHWRender::MSamplerState* samplerState;

    auto it = _samplerStates.find(desc);
    if (it != _samplerStates.end()) {
        samplerState = it->second;
    }
    else {
        samplerState = MHWRender::MStateManager::acquireSamplerState(desc);
        _samplerStates[desc] = samplerState;
    }
 
    return samplerState;
}

PXR_NAMESPACE_CLOSE_SCOPE
