#include "viewportRenderer.h"

#include <maya/MRenderingInfo.h>
#include <pxr/base/gf/matrix4d.h>

PXR_NAMESPACE_USING_DIRECTIVE

HdViewportRenderer::HdViewportRenderer() :
    MViewportRenderer("HdViewportRenderer") {
    fUIName.set("Hydra Viewport Renderer");
    fRenderingOverride = MViewportRenderer::kOverrideAllDrawing;
}

HdViewportRenderer::~HdViewportRenderer() {
}

MStatus HdViewportRenderer::initialize() {
    if (!UsdImagingGLHdEngine::IsDefaultPluginAvailable()) { return MStatus::kFailure; }
    GlfGlewInit();
    renderIndex.reset(HdRenderIndex::New(&renderDelegate));
    taskDelegate = std::make_shared<MayaSceneDelegate>(renderIndex.get(), SdfPath("/HdViewportRenderer"));
    return MStatus::kSuccess;
}

MStatus HdViewportRenderer::uninitialize() {
    return MStatus::kSuccess;
}

MStatus HdViewportRenderer::render(const MRenderingInfo& renderInfo) {
    if (renderInfo.renderingAPI() != MViewportRenderer::kOpenGL) {
        return MS::kFailure;
    }

    float clearColor[4] = { 0.0f };
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    renderInfo.renderTarget().makeTargetCurrent();

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto originX = renderInfo.originX();
    const auto originY = renderInfo.originY();
    const auto width = renderInfo.width();
    const auto height = renderInfo.height();

    UsdImagingGLEngine::RenderParams params;
    params.drawMode = UsdImagingGLEngine::DrawMode::DRAW_GEOM_FLAT;
    params.showGuides = true;
    params.gammaCorrectColors = false;
    GfVec4d viewport(originX, originY, width, height);
    taskDelegate->SetCameraState(renderInfo.viewMatrix(), renderInfo.projectionMatrix(), viewport);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    glEnable(GL_LIGHTING);

    auto tasks = taskDelegate->GetRenderTasks();
    engine.Execute(*renderIndex, tasks);

    glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    glPopAttrib(); // GL_ENABLE_BIT | GL_CURRENT_BIT

    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    return MStatus::kSuccess;
}

bool HdViewportRenderer::nativelySupports(MViewportRenderer::RenderingAPI api, float /*version*/) {
    return MViewportRenderer::kOpenGL == api;
}

bool HdViewportRenderer::override(MViewportRenderer::RenderingOverride override) {
    return fRenderingOverride == override;
}
