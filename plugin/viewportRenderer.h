#ifndef __VIEWPORT_RENDERER_H__
#define __VIEWPORT_RENDERER_H__

#include <maya/MViewportRenderer.h>

#include <pxr/pxr.h>
#include <pxr/imaging/glf/glew.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImagingGL/hdEngine.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdViewportRenderer : public MViewportRenderer {
public:
    HdViewportRenderer();
    ~HdViewportRenderer() override;

    MStatus initialize() override;
    MStatus uninitialize() override;

    MStatus render(const MRenderingInfo& renderInfo) override;
    bool nativelySupports(MViewportRenderer::RenderingAPI api, float version) override;
    bool override(MViewportRenderer::RenderingOverride override) override;
private:
    UsdStageRefPtr stage;
    // We only care about hydra, not the reference one.
    UsdImagingGLEngine* hdEngine;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __VIEWPORT_RENDERER_H__
