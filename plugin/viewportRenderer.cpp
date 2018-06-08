#include "viewportRenderer.h"

HdViewportRenderer::HdViewportRenderer() :
    MViewportRenderer("HdViewportRenderer") {
    fUIName.set("Hydra Viewport Renderer");
    fRenderingOverride = MViewportRenderer::kOverrideAllDrawing;
}

HdViewportRenderer::~HdViewportRenderer() {

}

MStatus HdViewportRenderer::initialize() {
    return MStatus::kSuccess;
}

MStatus HdViewportRenderer::uninitialize() {
    return MStatus::kSuccess;
}

MStatus HdViewportRenderer::render(const MRenderingInfo& renderInfo) {
    return MStatus::kSuccess;
}

bool HdViewportRenderer::nativelySupports(MViewportRenderer::RenderingAPI api, float version) {
    return MViewportRenderer::kOpenGL == api;
}

bool HdViewportRenderer::override(MViewportRenderer::RenderingOverride override) {
    return fRenderingOverride == override;
}
