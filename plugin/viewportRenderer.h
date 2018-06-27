#ifndef __HDMAYA_VIEWPORT_RENDERER_H__
#define __HDMAYA_VIEWPORT_RENDERER_H__

#include <pxr/pxr.h>
#include <pxr/imaging/glf/glew.h>

#include <pxr/usd/usd/stage.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdSt/renderDelegate.h>

#include <pxr/usdImaging/usdImagingGL/hdEngine.h>

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/rprimCollection.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/taskController.h>

#include <maya/MViewportRenderer.h>

#include "delegates/delegate.h"
#include "delegates/delegateRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaViewportRenderer : public MViewportRenderer {
public:
    HdMayaViewportRenderer();
    ~HdMayaViewportRenderer() override;

    static HdMayaViewportRenderer* GetInstance();
    static void Cleanup();

    MStatus initialize() override;
    MStatus uninitialize() override;

    static TfTokenVector GetRendererPlugins();
    static std::string GetRendererPluginDisplayName(const TfToken& id);
    static void ChangeRendererPlugin(const TfToken& id);
    static int GetFallbackShadowMapResolution();
    static void SetFallbackShadowMapResolution(int resolution);

    MStatus render(const MRenderingInfo& renderInfo) override;
    bool nativelySupports(MViewportRenderer::RenderingAPI api, float version) override;
    bool override(MViewportRenderer::RenderingOverride override) override;
    unsigned int overrideThenStandardExclusion() const override;
private:
    void InitHydraResources();
    void ClearHydraResources();

    HdEngine _engine;
    HdxRendererPlugin* _rendererPlugin = nullptr;
    HdxTaskController* _taskController = nullptr;
    HdRenderIndex* _renderIndex = nullptr;
    HdxSelectionTrackerSharedPtr _selectionTracker;

    std::vector<HdMayaDelegatePtr> _delegates;

    SdfPath _ID;
    TfToken _rendererName;

    int _fallbackShadowMapResolution = 1024;

    bool _initializedViewport = false;
    bool _isPopulated = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_VIEWPORT_RENDERER_H__
