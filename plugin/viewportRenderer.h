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
    HdMayaDelegate* _delegate = nullptr;
    HdxSelectionTrackerSharedPtr _selectionTracker;

    SdfPath _delegateID;

    bool _initializedViewport = false;
    bool _isPopulated = false;

    TfToken _rendererName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_VIEWPORT_RENDERER_H__
