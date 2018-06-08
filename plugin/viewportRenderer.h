#ifndef __VIEWPORT_RENDERER_H__
#define __VIEWPORT_RENDERER_H__

#include <maya/MViewportRenderer.h>

class HdViewportRenderer : public MViewportRenderer {
public:
    HdViewportRenderer();
    ~HdViewportRenderer();

    MStatus initialize() override;
    MStatus uninitialize() override;

    MStatus render(const MRenderingInfo& renderInfo) override;
    bool nativelySupports(MViewportRenderer::RenderingAPI api, float version) override;
    bool override(MViewportRenderer::RenderingOverride override) override;
};

#endif // __VIEWPORT_RENDERER_H__
