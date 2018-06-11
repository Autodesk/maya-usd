#ifndef __VIEWPORT_RENDERER_H__
#define __VIEWPORT_RENDERER_H__

#include <maya/MViewportRenderer.h>

#include <pxr/pxr.h>
#include <pxr/imaging/glf/glew.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hdSt/renderDelegate.h>
#include <pxr/usdImaging/usdImagingGL/hdEngine.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/rprimCollection.h>

#include "delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdViewportRenderer : public MViewportRenderer {
public:
    HdViewportRenderer();
    ~HdViewportRenderer() override;

    static HdViewportRenderer* getInstance();
    static void cleanup();

    MStatus initialize() override;
    MStatus uninitialize() override;

    TfTokenVector getRendererPlugins();

    MStatus render(const MRenderingInfo& renderInfo) override;
    bool nativelySupports(MViewportRenderer::RenderingAPI api, float version) override;
    bool override(MViewportRenderer::RenderingOverride override) override;
private:
    UsdStageRefPtr stage;
    // We only care about hydra, not the reference one.
    HdEngine engine;
    HdStRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex;
    MayaSceneDelegateSharedPtr taskDelegate;
    HdRprimCollection rprims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __VIEWPORT_RENDERER_H__
