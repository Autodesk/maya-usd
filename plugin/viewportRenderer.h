#ifndef __HDMAYA_VIEWPORT_RENDERER_H__
#define __HDMAYA_VIEWPORT_RENDERER_H__

#include <maya/MViewportRenderer.h>

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

#include "delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaViewportRenderer : public MViewportRenderer {
public:
    HdMayaViewportRenderer();
    ~HdMayaViewportRenderer() override;

    static HdMayaViewportRenderer* getInstance();
    static void cleanup();

    MStatus initialize() override;
    MStatus uninitialize() override;

    static TfTokenVector getRendererPlugins();
    static std::string getRendererPluginDisplayName(const TfToken& id);
    static void changeRendererPlugin(const TfToken& id);

    MStatus render(const MRenderingInfo& renderInfo) override;
    bool nativelySupports(MViewportRenderer::RenderingAPI api, float version) override;
    bool override(MViewportRenderer::RenderingOverride override) override;
private:
    void initHydraResources();
    void clearHydraResources();

    HdEngine engine;
    HdxRendererPlugin* rendererPlugin = nullptr;
    HdxTaskController* taskController = nullptr;
    HdRenderIndex* renderIndex = nullptr;
    HdMayaDelegate* delegate = nullptr;
    HdxSelectionTrackerSharedPtr selectionTracker;

    SdfPath delegateID;

    bool initializedViewport = false;

    TfToken rendererName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_VIEWPORT_RENDERER_H__
