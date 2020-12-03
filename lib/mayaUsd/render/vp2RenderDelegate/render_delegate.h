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
#ifndef HD_VP2_RENDER_DELEGATE
#define HD_VP2_RENDER_DELEGATE

#include "render_param.h"
#include "resource_registry.h"
#include "shader.h"

#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/pxr.h>

#include <maya/MShaderManager.h>
#include <maya/MString.h>

#include <atomic>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdVP2BBoxGeom;
class ProxyRenderDelegate;

/*! \brief    VP2 render delegate
    \class    HdVP2RenderDelegate

    Render delegates provide renderer-specific functionality to the render
    index, the main hydra state management structure. The render index uses
    the render delegate to create and delete scene primitives, which include
    geometry and also non-drawable objects.

    Primitives in Hydra are split into Rprims (drawables), Sprims (state
    objects like cameras and materials), and Bprims (buffer objects like
    textures). The minimum set of primitives a renderer needs to support is
    one Rprim (so the scene's not empty) and the "camera" Sprim, which is
    required by HdxRenderTask, the task implementing basic hydra drawing.

    VP2 render delegate reports which prim types it supports via
    GetSupportedRprimTypes() (and Sprim, Bprim).

    VP2 Rprims create MRenderItem geometry objects in the MPxSubSceneOverride.
    Render delegate renderpasses are not utilized, since subscene is only
    a subset of what's being draw in the viewport and overall control is left
    to the application.

    The render delegate also has a hook for the main hydra execution algorithm
    (HdEngine::Execute()): between HdRenderIndex::SyncAll(), which pulls new
    scene data, and execution of tasks, the engine calls back to
    CommitResources(). This commit is perfoming execution which must happen
    on the main-thread. In the future we will further split engine execution,
    levering evaluation time to do HdRenderIndex::SyncAll together with
    parallel DG computation and perform commit from reserved thread
    via main-thread tasks.
*/
class HdVP2RenderDelegate final : public HdRenderDelegate
{
public:
    HdVP2RenderDelegate(ProxyRenderDelegate& proxyDraw);

    ~HdVP2RenderDelegate() override;

    HdRenderParam* GetRenderParam() const override;

    const TfTokenVector& GetSupportedRprimTypes() const override;

    const TfTokenVector& GetSupportedSprimTypes() const override;

    const TfTokenVector& GetSupportedBprimTypes() const override;

    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    HdVP2ResourceRegistry& GetVP2ResourceRegistry();

    HdRenderPassSharedPtr
    CreateRenderPass(HdRenderIndex* index, HdRprimCollection const& collection) override;

    HdInstancer* CreateInstancer(
        HdSceneDelegate* delegate,
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
        SdfPath const& id) override;
#else
        SdfPath const& id,
        SdfPath const& instancerId) override;
#endif
    void DestroyInstancer(HdInstancer* instancer) override;

    HdRprim* CreateRprim(
        TfToken const& typeId,
#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
        SdfPath const& rprimId) override;
#else
        SdfPath const& rprimId,
        SdfPath const& instancerId) override;
#endif

    void DestroyRprim(HdRprim* rPrim) override;

    HdSprim* CreateSprim(TfToken const& typeId, SdfPath const& sprimId) override;
    HdSprim* CreateFallbackSprim(TfToken const& typeId) override;
    void     DestroySprim(HdSprim* sPrim) override;

    HdBprim* CreateBprim(TfToken const& typeId, SdfPath const& bprimId) override;
    HdBprim* CreateFallbackBprim(TfToken const& typeId) override;
    void     DestroyBprim(HdBprim* bPrim) override;

    void CommitResources(HdChangeTracker* tracker) override;

    TfToken GetMaterialBindingPurpose() const override;

    MString GetLocalNodeName(const MString& name) const;

    MHWRender::MShaderInstance* GetFallbackShader(const MColor& color) const;
    MHWRender::MShaderInstance* GetFallbackCPVShader() const;
    MHWRender::MShaderInstance* Get3dSolidShader(const MColor& color) const;
    MHWRender::MShaderInstance* Get3dCPVSolidShader() const;
    MHWRender::MShaderInstance* Get3dFatPointShader() const;

    MHWRender::MShaderInstance* GetBasisCurvesFallbackShader(
        const TfToken& curveType,
        const TfToken& curveBasis,
        const MColor&  color) const;

    MHWRender::MShaderInstance*
    GetBasisCurvesCPVShader(const TfToken& curveType, const TfToken& curveBasis) const;

    MHWRender::MShaderInstance* GetShaderFromCache(const TfToken& id);
    bool AddShaderToCache(const TfToken& id, const MHWRender::MShaderInstance& shader);

    const MHWRender::MSamplerState* GetSamplerState(const MHWRender::MSamplerStateDesc& desc) const;

    const HdVP2BBoxGeom& GetSharedBBoxGeom() const;

    static const int sProfilerCategory; //!< Profiler category

private:
    HdVP2RenderDelegate(const HdVP2RenderDelegate&) = delete;
    HdVP2RenderDelegate& operator=(const HdVP2RenderDelegate&) = delete;

    static std::atomic_int
        _renderDelegateCounter; //!< Number of render delegates. First one creates shared resources
                                //!< and last one deletes them.
    static std::mutex
        _renderDelegateMutex; //!< Mutex protecting construction/destruction of render delegate
    static HdResourceRegistrySharedPtr
        _resourceRegistry; //!< Shared and unused by VP2 resource registry

    std::unique_ptr<HdVP2RenderParam>
            _renderParam; //!< Render param used to provided access to VP2 during prim synchronization
    SdfPath _id;          //!< Render delegate ID
    HdVP2ResourceRegistry
        _resourceRegistryVP2; //!< VP2 resource registry used for enqueue and execution of commits

    HdVP2ShaderCache _shaderCache; //!< A thread-safe cache of named shaders.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
