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
#include "render_delegate.h"

#include "basisCurves.h"
#include "bboxGeom.h"
#include "instancer.h"
#include "material.h"
#include "mesh.h"
#include "render_pass.h"

#include <mayaUsd/render/vp2ShaderFragments/shaderFragments.h>

#include <pxr/imaging/hd/bprim.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/instancer.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/tokens.h>

#include <maya/MProfiler.h>

#include <boost/functional/hash.hpp>
#include <tbb/reader_writer_lock.h>
#include <tbb/spin_rw_mutex.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
/*! \brief List of supported Rprims by VP2 render delegate
 */
inline const TfTokenVector& _SupportedRprimTypes()
{
    static const TfTokenVector SUPPORTED_RPRIM_TYPES
        = { HdPrimTypeTokens->basisCurves, HdPrimTypeTokens->mesh };
    return SUPPORTED_RPRIM_TYPES;
}

/*! \brief List of supported Sprims by VP2 render delegate
 */
inline const TfTokenVector& _SupportedSprimTypes()
{
    static const TfTokenVector r { HdPrimTypeTokens->material, HdPrimTypeTokens->camera };
    return r;
}

/*! \brief List of supported Bprims by VP2 render delegate
 */
inline const TfTokenVector& _SupportedBprimTypes()
{
    static const TfTokenVector r {};
    return r;
}

const MString _diffuseColorParameterName = "diffuseColor"; //!< Shader parameter name
const MString _solidColorParameterName = "solidColor";     //!< Shader parameter name
const MString _pointSizeParameterName = "pointSize";       //!< Shader parameter name
const MString _curveBasisParameterName = "curveBasis";     //!< Shader parameter name
const MString _structOutputName = "outSurfaceFinal"; //!< Output struct name of the fallback shader

//! Enum class for fallback shader types
enum class FallbackShaderType
{
    kCommon = 0,
    kBasisCurvesLinear,
    kBasisCurvesCubicBezier,
    kBasisCurvesCubicBSpline,
    kBasisCurvesCubicCatmullRom,
    kCount
};

//! Total number of fallback shader types
constexpr size_t FallbackShaderTypeCount = static_cast<size_t>(FallbackShaderType::kCount);

//! Array of constant-color shader fragment names indexed by FallbackShaderType
const MString _fallbackShaderNames[] = { "FallbackShader",
                                         "BasisCurvesLinearFallbackShader",
                                         "BasisCurvesCubicFallbackShader",
                                         "BasisCurvesCubicFallbackShader",
                                         "BasisCurvesCubicFallbackShader" };

//! Array of varying-color shader fragment names indexed by FallbackShaderType
const MString _cpvFallbackShaderNames[] = { "FallbackCPVShader",
                                            "BasisCurvesLinearCPVShader",
                                            "BasisCurvesCubicCPVShader",
                                            "BasisCurvesCubicCPVShader",
                                            "BasisCurvesCubicCPVShader" };

//! "curveBasis" parameter values for three different cubic curves
const std::unordered_map<FallbackShaderType, int> _curveBasisParameterValueMapping
    = { { FallbackShaderType::kBasisCurvesCubicBezier, 0 },
        { FallbackShaderType::kBasisCurvesCubicBSpline, 1 },
        { FallbackShaderType::kBasisCurvesCubicCatmullRom, 2 } };

//! Get the shader type needed by the given curveType and curveBasis
FallbackShaderType GetBasisCurvesShaderType(const TfToken& curveType, const TfToken& curveBasis)
{
    FallbackShaderType type = FallbackShaderType::kCount;

    if (curveType == HdTokens->linear) {
        type = FallbackShaderType::kBasisCurvesLinear;
    } else if (curveType == HdTokens->cubic) {
        if (curveBasis == HdTokens->bezier) {
            type = FallbackShaderType::kBasisCurvesCubicBezier;
        } else if (curveBasis == HdTokens->bSpline) {
            type = FallbackShaderType::kBasisCurvesCubicBSpline;
        } else if (curveBasis == HdTokens->catmullRom) {
            type = FallbackShaderType::kBasisCurvesCubicCatmullRom;
        }
    }

    return type;
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

/*! \brief  Color-indexed shader map.
 */
struct MShaderMap
{
    //! Shader registry
    std::unordered_map<MColor, MHWRender::MShaderInstance*, MColorHash> _map;

    //! Synchronization used to protect concurrent read from serial writes
    tbb::spin_rw_mutex _mutex;
};

/*! \brief  Shader cache.
 */
class MShaderCache final
{
public:
    /*! \brief  Initialize shaders.
     */
    void Initialize()
    {
        if (_isInitialized)
            return;

        MHWRender::MRenderer*            renderer = MHWRender::MRenderer::theRenderer();
        const MHWRender::MShaderManager* shaderMgr
            = renderer ? renderer->getShaderManager() : nullptr;
        if (!TF_VERIFY(shaderMgr))
            return;

        _3dCPVSolidShader = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dCPVSolidShader);

        TF_VERIFY(_3dCPVSolidShader);

        _3dFatPointShader = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dFatPointShader);

        if (TF_VERIFY(_3dFatPointShader)) {
            constexpr float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            constexpr float size[] = { 5.0, 5.0 };

            _3dFatPointShader->setParameter(_solidColorParameterName, white);
            _3dFatPointShader->setParameter(_pointSizeParameterName, size);
        }

        for (size_t i = 0; i < FallbackShaderTypeCount; i++) {
            MHWRender::MShaderInstance* shader
                = shaderMgr->getFragmentShader(_cpvFallbackShaderNames[i], _structOutputName, true);

            if (TF_VERIFY(shader)) {
                FallbackShaderType type = static_cast<FallbackShaderType>(i);
                const auto         it = _curveBasisParameterValueMapping.find(type);
                if (it != _curveBasisParameterValueMapping.end()) {
                    shader->setParameter(_curveBasisParameterName, it->second);
                }
            }

            _fallbackCPVShaders[i] = shader;
        }

        _isInitialized = true;
    }

    /*! \brief  Returns a fallback CPV shader instance when no material is bound.
     */
    MHWRender::MShaderInstance* GetFallbackCPVShader(FallbackShaderType type) const
    {
        if (type >= FallbackShaderType::kCount) {
            return nullptr;
        }

        const size_t index = static_cast<size_t>(type);
        return _fallbackCPVShaders[index];
    }

    /*! \brief  Returns a white 3d fat point shader.
     */
    MHWRender::MShaderInstance* Get3dFatPointShader() const { return _3dFatPointShader; }

    /*! \brief  Returns a 3d CPV solid-color shader instance.
     */
    MHWRender::MShaderInstance* Get3dCPVSolidShader() const { return _3dCPVSolidShader; }

    /*! \brief  Returns a 3d solid shader with the specified color.
     */
    MHWRender::MShaderInstance* Get3dSolidShader(const MColor& color)
    {
        // Look for it first with reader lock
        tbb::spin_rw_mutex::scoped_lock lock(_3dSolidShaders._mutex, false /*write*/);
        auto                            it = _3dSolidShaders._map.find(color);
        if (it != _3dSolidShaders._map.end()) {
            return it->second;
        }

        // Upgrade to writer lock.
        lock.upgrade_to_writer();

        // Double check that it wasn't inserted by another thread
        it = _3dSolidShaders._map.find(color);
        if (it != _3dSolidShaders._map.end()) {
            return it->second;
        }

        MHWRender::MShaderInstance* shader = nullptr;

        MHWRender::MRenderer*            renderer = MHWRender::MRenderer::theRenderer();
        const MHWRender::MShaderManager* shaderMgr
            = renderer ? renderer->getShaderManager() : nullptr;
        if (TF_VERIFY(shaderMgr)) {
            shader = shaderMgr->getStockShader(MHWRender::MShaderManager::k3dSolidShader);

            if (TF_VERIFY(shader)) {
                const float solidColor[] = { color.r, color.g, color.b, color.a };
                shader->setParameter(_solidColorParameterName, solidColor);

                // Insert instance we just created
                _3dSolidShaders._map[color] = shader;
            }
        }

        return shader;
    }

    /*! \brief  Returns a fallback shader instance when no material is bound.

        This method is keeping registry of all fallback shaders generated,
        allowing only one instance per color which enables consolidation of
        draw calls with same shader instance.

        \param color   Color to set on given shader instance
        \param index   Index to the required shader name in _fallbackShaderNames

        \return A new or existing copy of shader instance with given color
    */
    MHWRender::MShaderInstance* GetFallbackShader(const MColor& color, FallbackShaderType type)
    {
        if (type >= FallbackShaderType::kCount) {
            return nullptr;
        }

        const size_t index = static_cast<size_t>(type);
        auto&        shaderMap = _fallbackShaders[index];

        // Look for it first with reader lock
        tbb::spin_rw_mutex::scoped_lock lock(shaderMap._mutex, false /*write*/);
        auto                            it = shaderMap._map.find(color);
        if (it != shaderMap._map.end()) {
            return it->second;
        }

        // Upgrade to writer lock.
        lock.upgrade_to_writer();

        // Double check that it wasn't inserted by another thread
        it = shaderMap._map.find(color);
        if (it != shaderMap._map.end()) {
            return it->second;
        }

        MHWRender::MShaderInstance* shader = nullptr;

        // If the map is not empty, clone any existing shader instance in the
        // map instead of acquiring via MShaderManager::getFragmentShader(),
        // which creates new shader fragment graph for each shader instance
        // and causes expensive shader compilation and rebinding.
        it = shaderMap._map.begin();
        if (it != shaderMap._map.end()) {
            shader = it->second->clone();
        } else {
            MHWRender::MRenderer*            renderer = MHWRender::MRenderer::theRenderer();
            const MHWRender::MShaderManager* shaderMgr
                = renderer ? renderer->getShaderManager() : nullptr;
            if (TF_VERIFY(shaderMgr)) {
                shader = shaderMgr->getFragmentShader(
                    _fallbackShaderNames[index], _structOutputName, true);

                if (TF_VERIFY(shader)) {
                    const auto it = _curveBasisParameterValueMapping.find(type);
                    if (it != _curveBasisParameterValueMapping.end()) {
                        shader->setParameter(_curveBasisParameterName, it->second);
                    }
                }
            }
        }

        // Insert the new shader instance
        if (TF_VERIFY(shader)) {
            float diffuseColor[] = { color.r, color.g, color.b, color.a };
            shader->setParameter(_diffuseColorParameterName, diffuseColor);
            shaderMap._map[color] = shader;
        }

        return shader;
    }

private:
    bool _isInitialized { false }; //!< Whether the shader cache is initialized

    //! Shader registry used by fallback shaders
    MShaderMap _fallbackShaders[FallbackShaderTypeCount];
    MShaderMap _3dSolidShaders;

    //!< Fallback shaders with CPV support
    MHWRender::MShaderInstance* _fallbackCPVShaders[FallbackShaderTypeCount] { nullptr };

    MHWRender::MShaderInstance* _3dFatPointShader { nullptr }; //!< 3d shader for points
    MHWRender::MShaderInstance* _3dCPVSolidShader { nullptr }; //!< 3d CPV solid-color shader
};

MShaderCache sShaderCache; //!< Global shader cache to minimize the number of unique shaders.

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
    bool
    operator()(const MHWRender::MSamplerStateDesc& a, const MHWRender::MSamplerStateDesc& b) const
    {
        return (
            a.filter == b.filter && a.comparisonFn == b.comparisonFn && a.addressU == b.addressU
            && a.addressV == b.addressV && a.addressW == b.addressW
            && a.borderColor[0] == b.borderColor[0] && a.borderColor[1] == b.borderColor[1]
            && a.borderColor[2] == b.borderColor[2] && a.borderColor[3] == b.borderColor[3]
            && a.mipLODBias == b.mipLODBias && a.minLOD == b.minLOD && a.maxLOD == b.maxLOD
            && a.maxAnisotropy == b.maxAnisotropy && a.coordCount == b.coordCount
            && a.elementIndex == b.elementIndex);
    }
};

using MSamplerStateCache = std::unordered_map<
    MHWRender::MSamplerStateDesc,
    const MHWRender::MSamplerState*,
    MSamplerStateDescHash,
    MSamplerStateDescEquality>;

MSamplerStateCache sSamplerStates; //!< Sampler state cache
tbb::spin_rw_mutex
    sSamplerRWMutex; //!< Synchronization used to protect concurrent read from serial writes

const HdVP2BBoxGeom* sSharedBBoxGeom
    = nullptr; //!< Shared geometry for all Rprims to display bounding box

} // namespace

const int HdVP2RenderDelegate::sProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "HdVP2RenderDelegate",
    "HdVP2RenderDelegate"
#else
    "HdVP2RenderDelegate"
#endif
);

std::mutex                  HdVP2RenderDelegate::_renderDelegateMutex;
std::atomic_int             HdVP2RenderDelegate::_renderDelegateCounter;
HdResourceRegistrySharedPtr HdVP2RenderDelegate::_resourceRegistry;

/*! \brief  Constructor.
 */
HdVP2RenderDelegate::HdVP2RenderDelegate(ProxyRenderDelegate& drawScene)
{
    _id = SdfPath(TfToken(TfStringPrintf("/HdVP2RenderDelegate_%p", this)));

    std::lock_guard<std::mutex> guard(_renderDelegateMutex);
    if (_renderDelegateCounter.fetch_add(1) == 0) {
        _resourceRegistry.reset(new HdResourceRegistry());

        // HdVP2BBoxGeom can only be instantiated during the lifetime of VP2
        // renderer from main thread. HdVP2RenderDelegate is created from main
        // thread currently, if we need to make its creation parallel in the
        // future we should move this code out.
        if (TF_VERIFY(sSharedBBoxGeom == nullptr)) {
            sSharedBBoxGeom = new HdVP2BBoxGeom();
        }
    }

    _renderParam.reset(new HdVP2RenderParam(drawScene));

    // Shader fragments can only be registered after VP2 initialization, thus the function cannot
    // be called when loading plugin (which can happen before VP2 initialization in the case of
    // command-line rendering). The fragments will be deregistered when plugin is unloaded.
    HdVP2ShaderFragments::registerFragments();

    // The shader cache should be initialized after registration of shader fragments.
    sShaderCache.Initialize();
}

/*! \brief  Destructor.
 */
HdVP2RenderDelegate::~HdVP2RenderDelegate()
{
    std::lock_guard<std::mutex> guard(_renderDelegateMutex);
    if (_renderDelegateCounter.fetch_sub(1) == 1) {
        _resourceRegistry.reset();

        if (TF_VERIFY(sSharedBBoxGeom)) {
            delete sSharedBBoxGeom;
            sSharedBBoxGeom = nullptr;
        }
    }
}

/*! \brief  Return delegate's HdVP2RenderParam, giving access to things like MPxSubSceneOverride.
 */
HdRenderParam* HdVP2RenderDelegate::GetRenderParam() const { return _renderParam.get(); }

/*! \brief  Notification to commit resources to GPU & compute before rendering

    This notification sent by HdEngine happens after parallel synchronization of data,
    prim's via VP2 resource registry are inserting work to commit. Now is the time
    on main thread to commit resources and compute missing streams.

    In future we will better leverage evaluation time to perform synchronization
    of data and allow main-thread task execution during the compute as it's done
    for the rest of VP2 synchronization with DG data.
*/
void HdVP2RenderDelegate::CommitResources(HdChangeTracker* tracker)
{
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
const TfTokenVector& HdVP2RenderDelegate::GetSupportedRprimTypes() const
{
    return _SupportedRprimTypes();
}

/*! \brief  Return a list of which Sprim types can be created by this class's.
 */
const TfTokenVector& HdVP2RenderDelegate::GetSupportedSprimTypes() const
{
    return _SupportedSprimTypes();
}

/*! \brief  Return a list of which Bprim types can be created by this class's.
 */
const TfTokenVector& HdVP2RenderDelegate::GetSupportedBprimTypes() const
{
    return _SupportedBprimTypes();
}

/*! \brief  Return unused global resource registry.
 */
HdResourceRegistrySharedPtr HdVP2RenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

/*! \brief  Return VP2 resource registry, holding access to commit execution enqueue.
 */
HdVP2ResourceRegistry& HdVP2RenderDelegate::GetVP2ResourceRegistry()
{
    return _resourceRegistryVP2;
}

/*! \brief  Create a renderpass for rendering a given collection.
 */
HdRenderPassSharedPtr
HdVP2RenderDelegate::CreateRenderPass(HdRenderIndex* index, const HdRprimCollection& collection)
{
    return HdRenderPassSharedPtr(new HdVP2RenderPass(this, index, collection));
}

/*! \brief  Request to create a new VP2 instancer.

    \param id           The unique identifier of this instancer.
    \param instancerId  The unique identifier for the parent instancer that
                        uses this instancer as a prototype (may be empty).

    \return A pointer to the new instancer or nullptr on error.
*/
HdInstancer* HdVP2RenderDelegate::CreateInstancer(
    HdSceneDelegate* delegate,
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    const SdfPath& id)
{
    return new HdVP2Instancer(delegate, id);
#else
    const SdfPath& id,
    const SdfPath& instancerId)
{
    return new HdVP2Instancer(delegate, id, instancerId);
#endif
}

/*! \brief  Destroy instancer instance
 */
void HdVP2RenderDelegate::DestroyInstancer(HdInstancer* instancer) { delete instancer; }

/*! \brief  Request to Allocate and Construct a new, VP2 specialized Rprim.

    \param typeId       the type identifier of the prim to allocate
    \param rprimId      a unique identifier for the prim
    \param instancerId  the unique identifier for the instancer that uses
                        the prim (optional: May be empty).

    \return A pointer to the new prim or nullptr on error.
*/
HdRprim* HdVP2RenderDelegate::CreateRprim(
    const TfToken& typeId,
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    const SdfPath& rprimId)
#else
    const SdfPath& rprimId,
    const SdfPath& instancerId)
#endif
{
    if (typeId == HdPrimTypeTokens->mesh) {
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
        return new HdVP2Mesh(this, rprimId);
#else
        return new HdVP2Mesh(this, rprimId, instancerId);
#endif
    }
    if (typeId == HdPrimTypeTokens->basisCurves) {
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
        return new HdVP2BasisCurves(this, rprimId);
#else
        return new HdVP2BasisCurves(this, rprimId, instancerId);
#endif
    }
    // if (typeId == HdPrimTypeTokens->volume) {
    // #if defined(HD_API_VERSION) && HD_API_VERSION >= 36
    //    return new HdVP2Volume(this, rprimId);
    // #else
    //    return new HdVP2Volume(this, rprimId, instancerId);
    // #endif
    //}
    TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    return nullptr;
}

/*! \brief  Destroy & deallocate Rprim instance
 */
void HdVP2RenderDelegate::DestroyRprim(HdRprim* rPrim) { delete rPrim; }

/*! \brief  Request to Allocate and Construct a new, VP2 specialized Sprim.

    \param typeId   the type identifier of the prim to allocate
    \param sprimId  a unique identifier for the prim

    \return A pointer to the new prim or nullptr on error.
*/
HdSprim* HdVP2RenderDelegate::CreateSprim(const TfToken& typeId, const SdfPath& sprimId)
{
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
HdSprim* HdVP2RenderDelegate::CreateFallbackSprim(const TfToken& typeId)
{
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
void HdVP2RenderDelegate::DestroySprim(HdSprim* sPrim) { delete sPrim; }

/*! \brief  Request to Allocate and Construct a new, VP2 specialized Bprim.

    \param typeId the type identifier of the prim to allocate
    \param sprimId a unique identifier for the prim

    \return A pointer to the new prim or nullptr on error.
*/
HdBprim* HdVP2RenderDelegate::CreateBprim(const TfToken& typeId, const SdfPath& bprimId)
{
    /*
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdVP2RenderBuffer(bprimId);
    }
    if (typeId == _tokens->openvdbAsset) {
        return new HdVP2OpenvdbAsset(this, bprimId);
    }
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    */
    return nullptr;
}

/*! \brief  Request to Allocate and Construct a Bprim to use as a standin, if there
            if an error with another another Bprim of the same type.  For example,
            if another prim references a non-exisiting Bprim, the fallback could
            be used.

    \param typeId the type identifier of the prim to allocate

    \return A pointer to the new prim or nullptr on error.
*/
HdBprim* HdVP2RenderDelegate::CreateFallbackBprim(const TfToken& typeId)
{
    /*
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdVP2RenderBuffer(SdfPath::EmptyPath());
    }
    if (typeId == _tokens->openvdbAsset) {
        return new HdVP2OpenvdbAsset(this, SdfPath::EmptyPath());
    }
    TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    */
    return nullptr;
}

/*! \brief  Destroy & deallocate Bprim instance
 */
void HdVP2RenderDelegate::DestroyBprim(HdBprim* bPrim) { delete bPrim; }

/*! \brief  Returns a token that indicates material bindings purpose.

    The full material purpose is suggested according to
      https://github.com/PixarAnimationStudios/USD/pull/853
*/
TfToken HdVP2RenderDelegate::GetMaterialBindingPurpose() const { return HdTokens->full; }

/*! \brief  Returns a node name made as a child of delegate's id.
 */
MString HdVP2RenderDelegate::GetLocalNodeName(const MString& name) const
{
    return MString(_id.AppendChild(TfToken(name.asChar())).GetText());
}

/*! \brief  Returns a clone of the shader entry stored in the cache with the specified id.
 */
MHWRender::MShaderInstance* HdVP2RenderDelegate::GetShaderFromCache(const TfToken& id)
{
    tbb::spin_rw_mutex::scoped_lock lock(_shaderCache._mutex, false /*write*/);

    const auto it = _shaderCache._map.find(id);

    const MHWRender::MShaderInstance* shader
        = (it != _shaderCache._map.cend() ? it->second.get() : nullptr);
    return (shader ? shader->clone() : nullptr);
}

/*! \brief  Adds a clone of the shader to the cache with the specified id if it doesn't exist.
 */
bool HdVP2RenderDelegate::AddShaderToCache(
    const TfToken&                    id,
    const MHWRender::MShaderInstance& shader)
{
    tbb::spin_rw_mutex::scoped_lock lock(_shaderCache._mutex, false /*write*/);

    const auto it = _shaderCache._map.find(id);
    if (it != _shaderCache._map.cend()) {
        return false;
    }

    lock.upgrade_to_writer();
    _shaderCache._map[id].reset(shader.clone());
    return true;
}

/*! \brief  Returns a fallback shader instance when no material is bound.

    This method is keeping registry of all fallback shaders generated, allowing only
    one instance per color which enables consolidation of render calls with same shader
    instance.

    \param color    Color to set on given shader instance

    \return A new or existing copy of shader instance with given color parameter set
*/
MHWRender::MShaderInstance* HdVP2RenderDelegate::GetFallbackShader(const MColor& color) const
{
    return sShaderCache.GetFallbackShader(color, FallbackShaderType::kCommon);
}

/*! \brief  Returns a constant-color fallback shader instance for basisCurves when no material is
   bound.

    This method is keeping registry of all fallback shaders generated, keeping minimal number of
    shader instances.

    \param curveType    The curve type
    \param curveBasis   The curve basis
    \param color        Color to set on given shader instance

    \return A new or existing copy of shader instance with given color parameter set
*/
MHWRender::MShaderInstance* HdVP2RenderDelegate::GetBasisCurvesFallbackShader(
    const TfToken& curveType,
    const TfToken& curveBasis,
    const MColor&  color) const
{
    FallbackShaderType type = GetBasisCurvesShaderType(curveType, curveBasis);
    return sShaderCache.GetFallbackShader(color, type);
}

/*! \brief  Returns a varying-color fallback shader instance for basisCurves when no material is
   bound.

    This method is keeping registry of all fallback shaders generated, keeping minimal number of
    shader instances.

    \param curveType    The curve type
    \param curveBasis   The curve basis

    \return A new or existing copy of shader instance with given color parameter set
*/
MHWRender::MShaderInstance* HdVP2RenderDelegate::GetBasisCurvesCPVShader(
    const TfToken& curveType,
    const TfToken& curveBasis) const
{
    FallbackShaderType type = GetBasisCurvesShaderType(curveType, curveBasis);
    return sShaderCache.GetFallbackCPVShader(type);
}

/*! \brief  Returns a fallback CPV shader instance when no material is bound.
 */
MHWRender::MShaderInstance* HdVP2RenderDelegate::GetFallbackCPVShader() const
{
    return sShaderCache.GetFallbackCPVShader(FallbackShaderType::kCommon);
}

/*! \brief  Returns a 3d solid-color shader.
 */
MHWRender::MShaderInstance* HdVP2RenderDelegate::Get3dSolidShader(const MColor& color) const
{
    return sShaderCache.Get3dSolidShader(color);
}

/*! \brief  Returns a 3d CPV solid-color shader.
 */
MHWRender::MShaderInstance* HdVP2RenderDelegate::Get3dCPVSolidShader() const
{
    return sShaderCache.Get3dCPVSolidShader();
}

/*! \brief  Returns a white 3d fat point shader.
 */
MHWRender::MShaderInstance* HdVP2RenderDelegate::Get3dFatPointShader() const
{
    return sShaderCache.Get3dFatPointShader();
}

/*! \brief  Returns a sampler state as specified by the description.
 */
const MHWRender::MSamplerState*
HdVP2RenderDelegate::GetSamplerState(const MHWRender::MSamplerStateDesc& desc) const
{
    // Look for it first with reader lock
    tbb::spin_rw_mutex::scoped_lock lock(sSamplerRWMutex, false /*write*/);

    auto it = sSamplerStates.find(desc);
    if (it != sSamplerStates.end()) {
        return it->second;
    }

    // Upgrade to writer lock.
    lock.upgrade_to_writer();

    // Double check that it wasn't inserted by another thread
    it = sSamplerStates.find(desc);
    if (it != sSamplerStates.end()) {
        return it->second;
    }

    // Create and cache.
    const MHWRender::MSamplerState* samplerState
        = MHWRender::MStateManager::acquireSamplerState(desc);
    sSamplerStates[desc] = samplerState;
    return samplerState;
}

/*! \brief  Returns the shared bbox geometry.
 */
const HdVP2BBoxGeom& HdVP2RenderDelegate::GetSharedBBoxGeom() const { return *sSharedBBoxGeom; }

PXR_NAMESPACE_CLOSE_SCOPE
