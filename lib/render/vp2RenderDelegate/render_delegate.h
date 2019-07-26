#ifndef HD_VP2_RENDER_DELEGATE
#define HD_VP2_RENDER_DELEGATE

// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/resourceRegistry.h"

#include "render_param.h"
#include "resource_registry.h"

#include <maya/MString.h>
#include <maya/MShaderManager.h>

#include <mutex>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

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
class HdVP2RenderDelegate final : public HdRenderDelegate {
public:
    HdVP2RenderDelegate(ProxyRenderDelegate& proxyDraw);

    ~HdVP2RenderDelegate() override;

    HdRenderParam* GetRenderParam() const override;

    const TfTokenVector& GetSupportedRprimTypes() const override;

    const TfTokenVector& GetSupportedSprimTypes() const override;

    const TfTokenVector& GetSupportedBprimTypes() const override;

    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    HdVP2ResourceRegistry& GetVP2ResourceRegistry();

    HdRenderPassSharedPtr CreateRenderPass(HdRenderIndex* index, HdRprimCollection const& collection) override;

    HdInstancer* CreateInstancer(HdSceneDelegate* delegate, SdfPath const& id, SdfPath const& instancerId) override;    
    void DestroyInstancer(HdInstancer* instancer) override;

    HdRprim* CreateRprim(TfToken const& typeId, SdfPath const& rprimId, SdfPath const& instancerId) override;
    void DestroyRprim(HdRprim* rPrim) override;

    HdSprim* CreateSprim(TfToken const& typeId, SdfPath const& sprimId) override;
    HdSprim* CreateFallbackSprim(TfToken const& typeId) override;
    void DestroySprim(HdSprim* sPrim) override;

    HdBprim* CreateBprim(TfToken const& typeId, SdfPath const& bprimId) override;
    HdBprim* CreateFallbackBprim(TfToken const& typeId) override;
    void DestroyBprim(HdBprim* bPrim) override;

    void CommitResources(HdChangeTracker* tracker) override;

    TfToken GetMaterialBindingPurpose() const override;

    MString GetLocalNodeName(const MString& name) const;

    MHWRender::MShaderInstance* GetFallbackShader(MColor color=MColor(0.0f,0.0f,1.0f,1.0f)) const;
    MHWRender::MShaderInstance* Get3dSolidShader() const;
    MHWRender::MShaderInstance* Get3dFatPointShader() const;

private:
    HdVP2RenderDelegate(const HdVP2RenderDelegate&) = delete;
    HdVP2RenderDelegate& operator=(const HdVP2RenderDelegate&) = delete;

    static std::mutex                   _mutexResourceRegistry;     //!< Mutex protecting construction/destruction of resource registry
    static std::atomic_int              _counterResourceRegistry;   //!< Number of render delegates sharing this resource registry. Last one deletes the instance.
    static HdResourceRegistrySharedPtr  _resourceRegistry;          //!< Shared and unused by VP2 resource registry

    std::unique_ptr<HdVP2RenderParam>   _renderParam;               //!< Render param used to provided access to VP2 during prim synchronization
    SdfPath                             _id;                        //!< Render delegate IDs
    HdVP2ResourceRegistry               _resourceRegistryVP2;       //!< VP2 resource registry used for enqueue and execution of commits
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
