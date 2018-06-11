#include "viewportRenderer.h"

#include <maya/MRenderingInfo.h>
#include <pxr/base/gf/matrix4d.h>

#include <pxr/imaging/hdx/rendererPlugin.h>
#include <pxr/imaging/hdx/rendererPluginRegistry.h>

#include <memory>
#include <mutex>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    std::unique_ptr<HdViewportRenderer> _viewportRenderer = nullptr;
    std::mutex _viewportRendererLock;
}

HdViewportRenderer::HdViewportRenderer() :
    MViewportRenderer("HdViewportRenderer") {
    fUIName.set("Hydra Viewport Renderer");
    fRenderingOverride = MViewportRenderer::kOverrideAllDrawing;
}

HdViewportRenderer::~HdViewportRenderer() {

}

HdViewportRenderer* HdViewportRenderer::getInstance() {
    std::lock_guard<std::mutex> lg(_viewportRendererLock);
    if (_viewportRenderer == nullptr) { _viewportRenderer.reset(new HdViewportRenderer()); }
    return _viewportRenderer.get();
}

void HdViewportRenderer::cleanup() {
    _viewportRenderer = nullptr;
}

MStatus HdViewportRenderer::initialize() {
    GlfGlewInit();
    renderIndex.reset(HdRenderIndex::New(&renderDelegate));
    taskDelegate = std::make_shared<HdMayaDelegate>(renderIndex.get(), SdfPath("/HdViewportRenderer"));
    return MStatus::kSuccess;
}

MStatus HdViewportRenderer::uninitialize() {
    return MStatus::kSuccess;
}

TfTokenVector HdViewportRenderer::getRendererPlugins() {
    HfPluginDescVector pluginDescs;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

    TfTokenVector ret; ret.reserve(pluginDescs.size());
    for (const auto& desc: pluginDescs) {
        ret.emplace_back(desc.id);
    }
    return ret;
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

    GfVec4d viewport(originX, originY, width, height);
    taskDelegate->SetCameraState(renderInfo.viewMatrix(), renderInfo.projectionMatrix(), viewport);

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    glEnable(GL_LIGHTING);

    MayaRenderParams params; params.enableLighting = false;
    auto tasks = taskDelegate->GetRenderTasks(params, rprims);
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
